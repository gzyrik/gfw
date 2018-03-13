#include "config.h"
#include "xtype.h"
#include "xthread.h"
#include "xmsg.h"
#include "xmem.h"
#include "xgc.h"
#include "xtest.h"
#include "xutil.h"
#include "xprintf.h"
#include "xstring.h"
#include "xslice.h"
#include <string.h>
#include <stdlib.h>
#define DFT_FINALIZE        NULL
#define	CACHE_SIZE          1021
#define WEAK_CONTAINER      -1
#define FREED_POINTER       -1
#define UNSET_MASK          0x80000000
#define UNSET_MASK_BIT(a)   ((unsigned)(a)>>31)

struct link {
    xint                number;
    xint                children[1];
};
struct node {
    xint                mark;
    union   {
        struct {
            xptr        address;
            struct link *children;
            xFreeFunc   finalize;
        } n; /* endpointer */
        struct {
            intptr_t    address;
            struct link *children;
            intptr_t    weak;
        } c; /* container */
        xint            free;   /* next free node index */
    } u;
};
union stack_node {
    xint                stack;
    xint                number;
    xint                id;
};
struct stack {
    struct stack        *next;
    union stack_node    *data;
    xint                top;
    xint                bottom;
    xint                current;
};
struct hash_node {
    struct hash_node    *next;
    xint                id;
};
struct hash_map {
    struct hash_node    **table;
    xint                size;
    struct hash_node    *free;
    xint                number;
};

struct cache_node {
    xint                count;
    xint                parent;
    xint                child;
};
struct _xGC {
    xint                size;     /* total number of nodes in 'pool list' */
    xint                free;     /* free node index at 'pool list' */
    xint                mark;

    struct stack        *stack;
    struct node         *pool;    /* pool list */
    struct hash_map     map;      /* ptr -> id */
    struct cache_node   cache[CACHE_SIZE];
    xint                dirty;
    xMutex              mutex;
    xPrivate            local;
};
struct weak_table {
    xint                node_id;
};

/* alloc node to recored address 'p', 0 is reserved for root node */
static xint
node_alloc_L            (xGC            *gc,
                         xptr           p,
                         xFreeFunc      finalize) 
{
    xint            i;
    struct node     *ret;
    if (gc->free == -1 || !gc->pool) {
        struct node *pool;
        xint sz = gc->size * 2;

        if (sz < 0)
            x_error ("out of gc size");
        else if (gc->size  == 0)
            sz = 1024;

        pool = (struct node*) x_realloc (gc->pool, sz*sizeof(struct node));
        gc->pool = pool;
        if (gc->size == 0 && p != NULL) {
            /* 0 is reserved for root node */
            pool->u.n.children = NULL;
            pool->u.n.address = NULL;
            pool->u.n.finalize = NULL;
            pool->mark = gc->mark++;
            gc->size++;
        }

        ret = pool + gc->size;
        ret->u.n.children = NULL;

        /* link free nodes */
        for (i = gc->size + 1; i < sz; ++i) {
            pool[i].u.free = i + 1;
            pool[i].u.n.children = NULL;
            pool[i].mark = -1;
        }
        /* last node */
        pool[sz-1].u.free = -1;
        gc->free = gc->size + 1;
        gc->size = sz;
    }
    else {
        ret = gc->pool + gc->free;
        gc->free = gc->pool[gc->free].u.free;
    }
    ret->u.n.address = p;
    ret->u.n.finalize = finalize;
    ret->mark = 0;
    if (ret->u.n.children)
        ret->u.n.children->number = 0;

    /* 0 is reserved for root node */
    x_assert (p == NULL ? ret == gc->pool : TRUE);

    return ret - gc->pool;
}
static void
node_free_L             (xGC            *gc,
                         xint           id)
{
    struct node *n = gc->pool + id;
    n->mark = -1;
    if (n->u.n.children)
        n->u.n.children->number = 0;
    n->u.free = gc->free;
    gc->free = id;
}
static struct link *
link_expand_L           (struct link    *old,
                         xint           sz)
{
    struct link *ret;
    if (old!=0) {
        sz+=old->number;
        if ((sz ^ old->number) <= old->number) {
            return old;
        }
    }

    sz=sz*2-1;

    ret=(struct link *)x_realloc(old,sizeof(struct link)+(sz-1)*sizeof(xint));
    if (old==0) {
        ret->number=0;
    }
    return ret;
}
/* Insert a parent/child id pair into cache	*/
static xbool
cache_insert_L          (xGC            *gc,
                         xint           parent,
                         xint           child)
{
    xint hash ;
    struct cache_node *cn;

    x_assert (parent < gc->size);
    hash = (parent ^ (child & ~ UNSET_MASK)) % CACHE_SIZE;
    cn = &gc->cache[hash];

    if (cn->count == 0) {
        cn->count  = 1;
        cn->parent = parent;
        cn->child  = child;
        gc->dirty++;
        return TRUE;
    }
    else if (cn->parent == parent) {
        if (cn->child == child) {
            cn->count++; /* reference count */
            return TRUE;
        }
        else if ((cn->child ^ child) == UNSET_MASK) {
            x_assert (cn->count > 0); /* unref link */
            --cn->count;
            if (cn->count==0){
                cn->child = 0;
                gc->dirty--;
            }
            return TRUE;
        }
    }
    /* hash值 冲突,无法解决 */
    return FALSE;
}

/* cmp function for cache sort */
X_INLINE_FUNC xbool
cache_cmp               (struct cache_node* ca,
                         struct cache_node *cb)
{
    if (ca->parent != cb->parent) {
        return cb->parent >= ca->parent;
    }
    return (ca->child & ~ UNSET_MASK) >= (cb->child & ~UNSET_MASK);
}
/* commit cache to the node graph */
static void
cache_flush_L           (xGC            *gc)
{
    xint i,j,k,sz;
    struct cache_node   *cache;
    if (!gc->dirty)
        return;

    cache = gc->cache;
    /* move dirty cache to head part */
    j  = 0;
    k  = CACHE_SIZE-1;
    sz = gc->dirty;
    do {
        while (cache[j].count && j<k)
            ++j;
        while (!cache[k].count && j<k)
            --k;
        if (j >= k)
            break;
        cache[j++] = cache[k];
        cache[k--].count = 0;
    } while (k > sz - 1);
    /* shell sort */
    for (k= sz / 2; k> 0; k/= 2) {
        for (i = k; i < sz; ++i) {
            struct cache_node key = cache[i];
            for( j = i -k;
                 j >= 0 && cache_cmp(cache+j, &key);
                 j -=k) {
                cache[j+k] = cache[j];
            }  
            cache[j+k] = key;
        }
    }
    i=0;
    while (i<CACHE_SIZE) {
        struct node *node;
        struct cache_node *head;
        struct cache_node *next;

        struct link *children;
        xint parent, child;

        /* cache is sorted, so not need to check next */
        if (!cache[i].count)
            break;

        head=&cache[i];
        next=head;
        sz=0;
        parent=cache[i].parent;
        /* parent in [head, next) is the same,
           'sz' is delta number of children. */ 
        while (i<CACHE_SIZE && next->count && next->parent == parent) {
            sz += (1 - UNSET_MASK_BIT(next->child))*next->count;
            ++next;
            ++i;
        }

        node = &gc->pool[parent];
        children = node->u.n.children;
        child = head->child;/* dirty child */
        k=j=0;

        if (children) {
            while (j<children->number) {
                if (head>=next) {
                    goto copy_next;
                }
                if (k != j)
                    children->children[k]=children->children[j];
                x_assert (child == head->child);
                if (child == (xint)((children->children[j] | UNSET_MASK))) {
                    /* remove link, new number is 'k'*/
                    --k;
                    --sz;
                    if(!(--head->count))
                        child = (++head)->child;
                }
                else if ((xint)(child & ~UNSET_MASK) < children->children[j]) {
                    /* children is sorted, so insert at index 'j' */
                    break;
                }
                ++j;
                ++k;
            }
        }
        if (sz>0) {
            /* shift empty space to insert new children */
            node->u.n.children = link_expand_L (node->u.n.children,sz);
            children = node->u.n.children;
            x_assert(children);
            memmove(children->children + j + sz,
                    children->children +j ,
                    (children->number - j) * sizeof(xint));
            j+=sz;
            children->number+=sz;
        }

        while(j<children->number) {
            if (head>=next) {
                goto copy_next;
            }
            x_assert (child == head->child);
            if (child == (xint)(children->children[j] | UNSET_MASK)) {
                --k;
                if(!(--head->count))
                    child = (++head)->child;
            }
            else if ((xint)(child & ~UNSET_MASK) < children->children[j]) {
                x_assert(child >= 0 );
                children->children[k]=child;
                --j;
                if(!(--head->count))
                    child = (++head)->child;
            }
            else if (k != j) {
                children->children[k]=children->children[j];
            }
            ++j;
            ++k;
        }

        while(head<next) {
            /* insert linked child */
            x_assert (k< children->number);
            x_assert (child == head->child);
            x_assert ((child & UNSET_MASK) == 0);
            children->children[k]=child;
            ++k;
            if(!(--head->count))
                child = (++head)->child;
        }
        children->number=k;
        continue;
copy_next:
        if (k!=j) {
            for (;j<children->number;j++,k++) {
                children->children[k]=children->children[j];
            }
            children->number=k;
        }
    }

    gc->dirty=0;
}
static void
node_add_L              (xGC            *gc,
                         xint           parent,
                         xint           child)
{
    while (!cache_insert_L (gc, parent, child)) {
        cache_flush_L (gc);
    }
}
X_INLINE_FUNC xint
hash(void *p)
{
    intptr_t t=(intptr_t)p;
    return (xint)((t>>2) ^ (t>>16));
}
static void
map_expand_L            (xGC            *gc)
{
    struct hash_node **table;
    xint i;
    struct hash_map *map = &gc->map;
    xint sz = map->size*2;
    if (sz == 0)
        sz = 1024;
    else if (sz < 0)
        x_error ("out of gc size");

    table=(struct hash_node	**)x_malloc(sz*sizeof(struct hash_node *));
    memset(table,0,sz*sizeof(struct hash_node *));
    for (i=0;i<map->size;i++) {
        struct hash_node *t=map->table[i];
        while (t) {
            struct hash_node *tmp=t;
            void *p=gc->pool[tmp->id].u.n.address;
            int new_hash=hash(p) & (sz-1);
            t=t->next;

            tmp->next=table[new_hash];
            table[new_hash]=tmp;
        }
    }
    x_free(map->table);
    map->table=table;
    map->size=sz;
}
static xint
map_id_L                (xGC            *gc,
                         xptr           p,
                         xFreeFunc      finalize)
{
    struct hash_node *node;
    xint    h;
    struct hash_map *map = &gc->map;
    h = hash(p);
    if (map->size) {
        node=map->table[h & (map->size -1)];
        while (node) {
            if (gc->pool[node->id].u.n.address == p) {
                return node->id;
            }
            node=node->next;
        }
    }
    if (++map->number >= map->size)
        map_expand_L (gc);

    if (map->free) {
        node=map->free;
        map->free=node->next;
    }
    else {
        node = x_slice_new(struct hash_node);
    }
    node->id   = node_alloc_L (gc, p,finalize);
    node->next = map->table[h & (map->size -1)];
    map->table[h & (map->size -1)]=node;

    return node->id;
}
static xint
map_try_id_L            (xGC            *gc,
                         xptr           p)
{
    struct hash_node *node;
    xint    h;
    struct hash_map *map = &gc->map;
    h = hash(p);

    node=map->table[h & (map->size -1)];
    while (node) {
        if (gc->pool[node->id].u.n.address == p) {
            return node->id;
        }
        node=node->next;
    }

    return 0;
}

static void
map_erase_L             (xGC            *gc,
                         xint           id)
{
    xptr p=gc->pool[id].u.n.address;
    struct hash_map *map = &gc->map;

    if (p) {
        xint h=hash(p);
        struct hash_node **node= &map->table[h & (map->size -1)];
        struct hash_node *find;
        while ((*node)->id!=id) {
            node=&(*node)->next;
        }
        find=*node;
        *node=find->next;
        find->next=map->free;
        map->free=find;
        --map->number;
    }
}
static struct stack*
stack_expand_L          (xGC            *gc)
{
    struct stack *s = x_private_get (&gc->local);

    if (!s) {
        s = x_slice_new (struct stack);
        s->data = x_malloc (sizeof(union stack_node)*3);
        s->data[0].stack=0; /* root node id */
        s->current=0;
        s->top = 1;
        s->bottom=1;
        x_private_set (&gc->local, s);
        s->next = gc->stack;
        gc->stack = s;
    }
    else if (((s->top + 1) ^ s->top) > s->top){
        s->data = x_realloc(s->data,
                            (s->top*2+1) * sizeof(union stack_node));
    }
    return s;
}
/*	   stack data
       +----------+
       0 | level 0  |                ----> level 0 / root ( node pack )
       1 | level 1  |                --+-> level 1 ( 1 node ref + node pack )
       2 | node ref | <-bottom       --+
       3 | 2 (lv.2) |
       4 | node ref |                --+-> level 2 ( 3 node ref )
       5 | node ref |                  |
       6 | node ref |                --+
       7 | 4 (lv.3) | <-current
       8 | node ref |                --+-> level 3 ( 2 node ref )
       9 | node ref |                --+
       10|   nil    | <-top
       11|   nil    |
       +----------+
       */
static void
stack_push_L            (xGC            *gc,
                         xint           id)
{
    xint i;
    struct stack *s = stack_expand_L (gc);
    for(i=s->top-1; i>=s->current;--i)
        if (s->data[i].id == id)
            return;
    s->data[s->top++].id = id;
}
static xint
stack_pack_range_L      (xGC            *gc,
                         struct stack   *s,
                         xint           to,
                         xint           top)
{
    if (to < s->bottom) {
        /* tmp objects [s->bottom, top) is linked to parent stack */
        xint i;
        xint parent = s->data[to].stack;
        for (i=s->bottom;i < top; ++i)
            node_add_L (gc, parent,s->data[i].id);
        return to+1;
    }
    else {
        /* `bottom' is the index of the new local stack, alloc node for it */
        xint bottom=stack_pack_range_L(gc,s,to-s->data[to].number,to);
        xint level=node_alloc_L (gc, x_malloc(0),DFT_FINALIZE);
        s->data[bottom].stack=level;
        /* tmp objects (to, top) is linked to the local stack */
        for (++to; to < top; ++to)
            node_add_L (gc, level,s->data[to].id);

        /* local stack `level' is linked to parent stack */
        node_add_L (gc, s->data[bottom-1].stack,level);

        return bottom+1;
    }
}
/* link all tmp objects to the stack */
static void
stack_pack_L            (xGC            *gc)
{
    xint bottom;

    struct stack *s = gc->stack;
    while (s != NULL) {
        bottom = stack_pack_range_L (gc, s, s->current, s->top);
        s->top = s->bottom = bottom;
        s->current = bottom - 1;
        s = s->next;
    }
}
X_INLINE_FUNC void
mark_weak_L             (xGC            *gc,
                         xint           id)
{
    if (gc->pool[id].mark <  gc->mark) {
        gc->pool[id].mark=gc->mark;
    }
}
static void
mark_L                  (xGC            *gc,
                         xint           root)
{
    if (gc->pool && gc->pool[root].mark <  gc->mark+1) {
        struct link *children=gc->pool[root].u.n.children;
        gc->pool[root].mark=gc->mark+1;
        if (children) {
            xint i;
            if (gc->pool[root].u.c.weak==WEAK_CONTAINER) {
                xbool merge=FALSE;
                for (i=children->number-1;i>=0;i--) {
                    xint child=children->children[i];
                    if ((intptr_t)gc->pool[child].u.n.address == FREED_POINTER) {
                        children->children[i]=0;
                        merge=TRUE;
                    }
                    else
                        mark_weak_L (gc, child);
                }

                if (merge) {
                    xint j;
                    for (i=0;i<children->number;i++) {
                        if (children->children[i]==0) {
                            break;
                        }
                    }

                    for (j=i,++i;i<children->number;i++) {
                        if (children->children[i]!=0) {
                            children->children[j++]=children->children[i];
                        }
                    }

                    children->number=j;
                }
            }
            else {
                for (i=children->number-1;i>=0;i--) {
                    mark_L (gc, children->children[i]);
                }
            }
        }
    }
}
/* link or unlink two pointer */
static void
link_set_L              (xGC            *gc,
                         xptr           parent,
                         xptr           prev,
                         xptr           now)
{
    xint parent_id;

    if (parent ==0 ) {
        parent_id = 0;
    }
    else {
        parent_id = map_id_L (gc, parent, DFT_FINALIZE);
    }
    if (prev) {
        xint prev_id = map_id_L (gc, prev, DFT_FINALIZE);
        stack_push_L (gc, prev_id);
        node_add_L (gc, parent_id,prev_id | UNSET_MASK);
    }
    if (now) {
        node_add_L (gc, parent_id,map_id_L(gc, now,DFT_FINALIZE));
    }
}

static xptr
malloc_U                (xGC            *gc,
                         xsize          size,
                         xptr           parent,
                         xFreeFunc      finalize)
{
    xptr    ret;
    xint    id;
    x_return_val_if_fail (size != 0, NULL);

    ret = x_malloc (size);

    x_mutex_lock (&gc->mutex);

    id = map_id_L (gc, ret, finalize);

    if (parent)
        link_set_L (gc, parent, 0, ret);
    else
        stack_push_L (gc, id);

    x_mutex_unlock (&gc->mutex);

    return ret;
}

xGC*
x_gc_new                (void)
{
    xGC *gc;

    gc = x_new0 (xGC, 1);
    x_mutex_init (&gc->mutex);

    return gc;
}

void
x_gc_delete             (xGC            *gc)
{
    xint i;
    struct hash_map *map;

    x_return_if_fail (gc != NULL);

    map = &gc->map;
    x_mutex_lock (&gc->mutex);

    for (i=0;i<gc->size;i++) {
        struct node *node = gc->pool+i;
        x_free (node->u.n.children);
        if (node->mark >= 0) {
            void *p=node->u.n.address;
            if (node->u.n.finalize && node->u.c.weak!=WEAK_CONTAINER) {
                node->u.n.finalize(p);
            }
            if ((intptr_t)p != FREED_POINTER) {
                x_free(p);
            }
        }
    }
    x_free (gc->pool);
    for	(i=0;i<map->size;i++) {
        x_slice_free_chain (struct hash_node, map->table[i], next);
    }
    x_slice_free_chain (struct hash_node, map->free, next);
    x_free (map->table);
    while (gc->stack) {
        struct stack *p= gc->stack->next;
        x_free (gc->stack->data);
        x_slice_free (struct stack, gc->stack);
        gc->stack = p;
    }

    x_mutex_unlock (&gc->mutex);
    x_mutex_clear (&gc->mutex);
    x_free (gc);
}

xptr
x_gc_wtable_new         (xGC            *gc,
                         xptr           parent)
{
    struct weak_table *ret;

    x_return_val_if_fail (gc != NULL, NULL);

    ret = x_slice_new (struct weak_table);
    x_mutex_lock (&gc->mutex);

    ret->node_id = map_id_L (gc, ret, DFT_FINALIZE);
    gc->pool[ret->node_id].u.c.weak=WEAK_CONTAINER;


    if (parent)
        link_set_L (gc, parent, 0, ret);
    else
        stack_push_L (gc, ret->node_id);

    x_mutex_unlock (&gc->mutex);
    return ret;
}

xptr
x_gc_wtable_next        (xGC            *gc,
                         xptr           wtable,
                         xint           *iter)
{
    xint i,j;
    xptr ret;
    struct link *children;
    struct weak_table *weak;

    x_return_val_if_fail (gc != NULL, NULL);
    x_return_val_if_fail (wtable != NULL, NULL);
    x_return_val_if_fail (iter != NULL, NULL);

    weak = wtable;
    x_mutex_lock (&gc->mutex);

    cache_flush_L (gc);
    children = gc->pool[weak->node_id].u.n.children;
    if (children==0) {
        x_mutex_unlock (&gc->mutex);
        return NULL;
    }

    for (i = *iter;i < children->number; i++) {
        int id=children->children[i];
        if (id) {
            if (gc->pool[id].u.c.address == FREED_POINTER) {
                children->children[i] = 0;
            }
            else {
                    *iter=i+1;
                stack_push_L (gc, id);
                ret = gc->pool[id].u.n.address;

                x_mutex_unlock (&gc->mutex);
                return ret;
            }
        }
    }

    for (i=0;i<children->number;i++) {
        if (children->children[i]==0) {
            break;
        }
    }

    for (j=i,++i;i<children->number;i++) {
        if (children->children[i]!=0) {
            children->children[j++]=children->children[i];
        }
    }

    children->number=j;

    x_mutex_unlock (&gc->mutex);
    return NULL;
}

xptr
x_gc_alloc              (xGC            *gc,
                         xsize          size,
                         xptr           parent,
                         xFreeFunc      finalize)
{
    x_return_val_if_fail (gc != NULL, NULL);

    return malloc_U (gc, size, parent, finalize);
}

void
x_gc_enter              (xGC            *gc)
{
    struct stack *s;

    x_return_if_fail (gc != NULL);

    x_mutex_lock (&gc->mutex);

    s = stack_expand_L (gc);
    s->data[s->top].number = s->top - s->current;
    s->current = s->top++;

    x_mutex_unlock (&gc->mutex);
}

void
x_gc_leave              (xGC            *gc,
                         xptr           p,
                         ...)
{
    va_list args;
    xint    id;
    struct stack *s;

    x_return_if_fail (gc != NULL);

    s = x_private_get (&gc->local);
    x_return_if_fail (s != NULL);

    va_start (args, p);

    x_mutex_lock (&gc->mutex);

    if (s->current >= s->bottom) {
        s->top = s->current;
        s->current -= s->data[s->current].number;
    }
    else {
        xint parent, child;
        --s->bottom;
        parent = s->data[s->bottom-1].stack;
        child = s->data[s->bottom].stack;
        node_add_L (gc, parent, child | UNSET_MASK);
        node_free_L (gc, child);
        s->current = s->bottom-1;
        s->top = s->current + 1;
    }


    while (p){
        id =  map_try_id_L (gc, p);
        if (id != 0)
            stack_push_L (gc, id);
        else
            x_warning ("%s:invalid gc ptr %p", X_STRLOC, p);

        p = va_arg (args, xptr);
    }

    x_mutex_unlock (&gc->mutex);
    va_end (args);
}
void
x_gc_collect            (xGC            *gc)
{
    xint i;

    x_return_if_fail (gc != NULL);

    x_mutex_lock (&gc->mutex);

    stack_pack_L (gc);
    cache_flush_L (gc);
    mark_L (gc, 0);
    for (i = 0; i< gc->size; ++i) {
        struct node *node = gc->pool+i;
        if (node->mark < gc->mark){
            if (node->mark >= 0) {
                xptr p = node->u.n.address;
                if (node->u.n.finalize
                    && node->u.c.weak != WEAK_CONTAINER)
                    node->u.n.finalize (p);
                if ((intptr_t)p != FREED_POINTER) {
                    map_erase_L (gc, i);
                    x_free (p);
                }
                node_free_L (gc, i);
            }
        }
        else if (node->mark == gc->mark) {
            xptr p = node->u.n.address;
            if (node->u.n.finalize
                && node->u.c.weak != WEAK_CONTAINER) {
                node->u.n.finalize (p);
                node->u.n.finalize = NULL;
            }
            map_erase_L (gc, i);
            node->u.c.address = FREED_POINTER;
            x_free (p);
        }
    }
    gc->mark += 2;

    x_mutex_unlock (&gc->mutex);
}
void
x_gc_link               (xGC            *gc,
                         xptr           parent,
                         xptr           prev,
                         xptr           now)
{
    x_return_if_fail (gc != NULL);
    x_return_if_fail (parent || prev || now);

    x_mutex_lock (&gc->mutex);
    link_set_L (gc, parent, prev, now);
    x_mutex_unlock (&gc->mutex);
}
xptr
x_gc_realloc            (xGC            *gc,
                         xptr           p,
                         xsize          sz,
                         xptr           parent)
{
    void *ret;

    x_return_val_if_fail (gc !=NULL, NULL);
    x_return_val_if_fail (sz >0, NULL);

    if (p==NULL) {
        return malloc_U (gc,sz,parent,NULL);
    }

    ret = x_realloc(p,sz);
    if (ret==p) {
        return ret;
    }
    else {
        xint new_id, old_id;
        struct link *tmp;

        x_mutex_lock (&gc->mutex);

        new_id=map_id_L(gc, ret, DFT_FINALIZE);
        old_id=map_id_L(gc, p, DFT_FINALIZE);

        tmp=gc->pool[new_id].u.n.children;
        gc->pool[new_id].u.n.children=gc->pool[old_id].u.n.children;
        gc->pool[old_id].u.n.children=tmp;

        gc->pool[new_id].u.n.finalize =gc->pool[old_id].u.n.finalize;

        if (parent) {
            link_set_L (gc,parent,p,ret);
        }
        else {
            stack_push_L (gc,new_id);
        }

        map_erase_L(gc,old_id);
        gc->pool[old_id].u.c.address=FREED_POINTER;

        x_mutex_unlock (&gc->mutex);
    }

    return ret;
}

xptr
x_gc_clone              (xGC            *gc,
                         xptr           from,
                         xsize          sz)
{
    xint from_id, to_id;
    xptr to;
    struct link *from_children;

    x_return_val_if_fail (gc != NULL, NULL);
    x_return_val_if_fail (from != NULL, NULL);
    x_return_val_if_fail (sz >0, NULL);

    to = x_malloc (sz);

    x_mutex_lock (&gc->mutex);

    from_id = map_id_L (gc,from, DFT_FINALIZE);
    to_id = map_id_L(gc,to, DFT_FINALIZE);

    from_children = gc->pool[from_id].u.n.children;

    stack_push_L (gc,to_id);

    cache_flush_L (gc);

    gc->pool[to_id].u.n.finalize = gc->pool[from_id].u.n.finalize;
    gc->pool[to_id].u.n.children = link_expand_L (gc->pool[to_id].u.n.children,
                                                  from_children->number);
    memcpy(gc->pool[to_id].u.n.children,
           from_children,
           sizeof(*from_children) + (from_children->number-1)*sizeof(int));

    x_mutex_unlock (&gc->mutex);

    memcpy(to,from,sz);

    return to;
}

xstr
x_gc_dryrun             (xGC            *gc,
                         xcstr          format,
                         ...)
{
    xint        i;
    xString     *string;

    x_return_val_if_fail (gc != NULL, NULL);

    string = x_string_new (NULL, 0);
    if (format) {
        va_list     args;
        va_start (args, format);
        x_string_append_vprintf (string, format, args);
        va_end (args);
    }

    x_mutex_lock (&gc->mutex);

    stack_pack_L (gc);
    cache_flush_L (gc);
    mark_L (gc, 0);
    for (i=0;i<gc->size;i++) {
        struct link *link=gc->pool[i].u.n.children;

        if (gc->pool[i].mark >= gc->mark+1) {
            if (gc->pool[i].u.c.weak == WEAK_CONTAINER) {
                x_string_append_printf (string, "%d[weak] -> ",i);
            }
            else {
                x_string_append_printf (string, "%d(%p) -> ",
                                        i,gc->pool[i].u.n.address);
            }
        }
        else if (gc->pool[i].mark == gc->mark) {
            x_string_append_printf (string, "w %d(%p): ",
                                    i,gc->pool[i].u.n.address);
        }
        else if (gc->pool[i].mark >= 0 ) {
            x_string_append_printf (string, "x %d(%p): ",
                                    i,gc->pool[i].u.n.address);
        }
        else {
            continue;
        }

        if (link) {
            int j;
            for (j=0;j<link->number;j++) {
                x_string_append_printf (string, "%d,",link->children[j]);
            }
        }

        x_string_append_char(string, '\n');
    }
    gc->mark++;

    x_mutex_unlock (&gc->mutex);

    return x_string_delete (string, FALSE);
}
