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
#define FREED_POINTER       (xptr)-1
#define WEAK_CONTAINER      -1

#define	CACHE_SIZE          1021
#define UNSET_MASK          0x80000000
#define UNSET_MASK_BIT(a)   ((unsigned)(a)>>31)

struct link {
    xsize               number;
    xsize               children[1];
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
        xsize           free;   /* next free node index, tail is -1 */
    } u;
};
union stack_node {
    xsize               stack;
    xsize               number;
    xsize               id;
};
struct stack {
    struct stack        *next;
    union stack_node    *data;
    xsize               top;
    xsize               bottom;
    xsize               current;
};
struct hash_node {
    struct hash_node    *next;
    xsize               id;
};
struct hash_map {
    struct hash_node    **table;
    xsize               size;
    struct hash_node    *free;
    xsize               number;
};

struct cache_node {
    xsize               child;
    xsize               parent;
};
struct _xGC {
    xsize               size;     /* total number of nodes in 'pool list' */
    xsize               free;     /* free node index at 'pool list' */
    xint                mark;

    struct stack        *stack;
    struct node         *pool;    /* pool list */
    struct hash_map     map;      /* ptr -> id */
    struct cache_node   cache[CACHE_SIZE];
    xsize               cache_dirty;
    xMutex              mutex;
    xPrivate            local;
};
struct weak_table {
    xsize               node_id;
};

/* alloc node to recored address 'p', 0 is reserved for root node */
static xsize
node_alloc_L            (xGC            *gc,
                         xptr           p,
                         xFreeFunc      finalize) 
{
    xsize           i;
    struct node     *ret;
    if (gc->free == (xsize)-1 || !gc->pool) {
        struct node *pool;
        xsize sz;

        sz = gc->size ? 2*gc->size : 1024;
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
        pool[sz-1].u.free = (xsize)-1;
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
                         xsize          id)
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
                         xsize          sz)
{
    struct link *ret;
    if (old!=0) {
        sz+=old->number;
        if ((sz ^ old->number) <= old->number) {
            return old;
        }
    }

    sz=sz*2-1;

    ret=(struct link *)x_realloc(old,sizeof(struct link)+(sz-1)*sizeof(xsize));
    if (old==0) {
        ret->number=0;
    }
    return ret;
}
/* Insert a parent/child id pair into cache	*/
static xbool
cache_insert_L          (xGC            *gc,
                         xsize          parent,
                         xsize          child)
{
    xsize hash ;
    struct cache_node *cn;

    x_assert (parent < gc->size);
    x_assert (child  > 0); /* 0 is root, can't be linked to other */

    hash = (((parent ^ 5381) << 5) ^ (child & ~ UNSET_MASK)) % CACHE_SIZE;
    cn = &gc->cache[hash];

    if (cn->child == 0) {
        cn->parent = parent;
        cn->child  = child;
        gc->cache_dirty++;
        return TRUE;
    }
    else if (cn->parent == parent) {
        if (cn->child == child)
            return TRUE;
        else if ((cn->child ^ child) == UNSET_MASK) {
            cn->child = 0;
            gc->cache_dirty--;
            return TRUE;
        }
    }
    /* hash值 冲突,无法解决 */
    x_assert(gc->cache_dirty > 0);
    return FALSE;
}

/* cmp function for cache sort */
X_INLINE_FUNC xssize
cache_node_cmp          (const struct cache_node* ca,
                         const struct cache_node *cb)
{
    if (ca->parent != cb->parent) {
        return ca->parent - cb->parent;
    }
    return (ca->child & ~ UNSET_MASK) - (cb->child & ~UNSET_MASK);
}
/* commit cache to the node graph */
static void
cache_flush_L           (xGC            *gc)
{
    xsize i,j,k,sz;
    struct cache_node   *cache;
    if (!gc->cache_dirty)
        return;

    cache = gc->cache;
    /* move dirty cache to head part and sort */
    j  = 0;
    k  = CACHE_SIZE-1;
    sz = gc->cache_dirty;
    do {
        while (cache[j].child != 0 && j<k)
            ++j;
        while (cache[k].child == 0 && j<k)
            --k;
        if (j >= k)
            break;
        cache[j++] = cache[k];
        cache[k--].child = 0;
    } while (k > sz - 1);
    /* shell sort */
    for (k= sz / 2; k> 0; k /= 2) {
        for (i = k; i < sz; ++i) {
            xssize j;
            for(j = i - k;j >= 0;j -=k) {
                if (cache_node_cmp(cache+j, cache+j+k) > 0){
                    /* swap j, j+k */
                    struct cache_node tmp = cache[j];
                    cache[j] = cache[j+k];
                    cache[j+k] = tmp;
                }
            }  
        }
    }

    i=0;
    while (i<CACHE_SIZE && cache[i].child != 0) {
        struct cache_node *head = cache+i;
        struct cache_node *next = head;
        const xsize parent=cache[i].parent;
        struct node * const node = &gc->pool[parent];
        struct link * children = node->u.n.children;

        sz=0;
        /* parent in [head, next) is the same, 'sz' is the number of children to insert. */ 
        while (i<CACHE_SIZE && next->child != 0 && next->parent == parent) {
            sz += 1 - UNSET_MASK_BIT(next->child);
            ++next;
            ++i;
        }
        if (!children || children->number == 0) {
            /* only allow to insert */
            if (sz > 0) {
                /* shift empty space to insert new children */
                children = node->u.n.children = link_expand_L (children,sz);
                sz = 0;
            }
            else
                continue; /* no need to insert to empty children */
        }
        k=j=0; /* k is write index, j is read index */
        while (head < next) {
            xssize delta = -1;
            if (j < children->number) {
                /* binary find the child position */
                const xsize child = head->child & ~UNSET_MASK;
                xsize pos, high = children->number;
                do {
                    pos = (j+high)/2;
                    delta = children->children[pos] - child;
                    if (delta <= 0) {
                        if (delta < 0) ++pos;
                        if (k < j) {
                            memmove (children->children + k,
                                     children->children + j,
                                     (pos - j)* sizeof (xsize));
                        }
                        k += pos - j;
                        j = pos;
                    }
                    else
                        high = pos;
                } while (delta != 0 && j < high);
            }
            if ((head->child & UNSET_MASK) != 0) {
                if (delta == 0) {
                    /* find this child at j, only allow to remove */
                    ++j; /* back the read index, will remove it */
                    if (sz > 0) /* if no allocate, need smaller */
                        --sz;
                }
                else {
                    /* no the child, no need to remove */
                }
            }
            else if (delta != 0) {
                /* no this child, only allow to insert at k */
                if (sz > 0) {
                    x_assert (k <= j);
                    /* shift empty space to insert new children */
                    children = node->u.n.children = link_expand_L (children,sz);
                    if (j < children->number) {
                        memmove(children->children + j + sz,
                                children->children + j ,
                                (children->number - j) * sizeof(xsize));
                    }

                    j += sz;
                    children->number+=sz;
                    sz = 0;
                }
                children->children[k++] = head->child;
            }
            else {
                /* already has the child, no need to insert */
            }
            head->child = 0;
            ++head;
        }
        /* copy remain and correct number */
        if (k!=j) {
            for (;j<children->number;j++,k++) {
                children->children[k]=children->children[j];
            }
            children->number=k;
        }
    }

    gc->cache_dirty=0;
}
static void
node_add_L              (xGC            *gc,
                         xsize          parent,
                         xsize          child)
{
    while (!cache_insert_L (gc, parent, child)) {
        cache_flush_L (gc);
    }
}
X_INLINE_FUNC xsize
hash(void *p)
{
    xsize t=(xsize)p;
    return ((t>>2) ^ (t>>16));
}
static void
map_expand_L            (xGC            *gc)
{
    struct hash_node **table;
    xsize i;
    struct hash_map *map = &gc->map;
    xsize sz = map->size ? map->size*2 : 1024;

    table=(struct hash_node	**)x_malloc(sz*sizeof(struct hash_node *));
    memset(table,0,sz*sizeof(struct hash_node *));
    for (i=0;i<map->size;i++) {
        struct hash_node *t=map->table[i];
        while (t) {
            struct hash_node *tmp=t;
            void *p=gc->pool[tmp->id].u.n.address;
            xsize new_hash=hash(p) & (sz-1);
            t=t->next;

            tmp->next=table[new_hash];
            table[new_hash]=tmp;
        }
    }
    x_free(map->table);
    map->table=table;
    map->size=sz;
}
static xsize
add_map_L               (xGC            *gc,
                         xsize          h,
                         xptr           p,
                         xFreeFunc      finalize)
{
    struct hash_map *map = &gc->map;
    struct hash_node *node;

    if (++map->number >= map->size)
        map_expand_L (gc);

    if (map->free) {
        node=map->free;
        map->free=node->next;
    }
    else {
        node = x_slice_new(struct hash_node);
    }
    node->id   = node_alloc_L (gc, p, finalize);
    node->next = map->table[h & (map->size -1)];
    map->table[h & (map->size -1)]=node;

    return node->id;
}
static xsize
map_try_id_L            (xGC            *gc,
                         xptr           p,
                         xsize          *ph)
{
    struct hash_map *map = &gc->map;
    xsize h = hash(p);

    if (map->size > 0) {
        struct hash_node *node;
        node=map->table[h & (map->size -1)];
        while (node) {
            if (gc->pool[node->id].u.n.address == p) {
                return node->id;
            }
            node=node->next;
        }
    }
    if (ph) *ph = h;
    return 0;
}

static void
map_erase_L             (xGC            *gc,
                         xsize          id)
{
    xptr p=gc->pool[id].u.n.address;
    struct hash_map *map = &gc->map;

    if (p) {
        xsize h=hash(p);
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
                         xsize          id)
{
    xsize i;
    struct stack *s = stack_expand_L (gc);
    for(i=s->current+1; i<s->top;++i) {
        if (s->data[i].id == id)
            return;
    }
    s->data[s->top++].id = id;
}
static xsize
stack_pack_range_L      (xGC            *gc,
                         struct stack   *s,
                         xsize          to,
                         xsize          top)
{
    if (to < s->bottom) {
        /* tmp objects [s->bottom, top) is linked to parent stack */
        xsize i;
        xsize parent = s->data[to].stack;
        for (i=s->bottom;i < top; ++i)
            node_add_L (gc, parent,s->data[i].id);
        return to+1;
    }
    else {
        /* `bottom' is the index of the new local stack, alloc node for it */
        xsize bottom=stack_pack_range_L(gc,s,to-s->data[to].number,to);
        xsize level=node_alloc_L (gc, FREED_POINTER, NULL);
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
    xsize bottom;

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
                         xsize          id)
{
    if (gc->pool[id].mark <  gc->mark) {
        gc->pool[id].mark=gc->mark;
    }
}
static void
mark_L                  (xGC            *gc,
                         xsize          root)
{
    if (gc->pool[root].mark <  gc->mark+1) {
        struct link *children=gc->pool[root].u.n.children;
        gc->pool[root].mark=gc->mark+1;
        if (children) {
            xsize i;
            if (gc->pool[root].u.c.weak==WEAK_CONTAINER) {
                xbool merge=FALSE;
                i=children->number;
                while(i>0) {
                    const xsize child =children->children[--i];
                    if (gc->pool[child].u.n.address == FREED_POINTER) {
                        children->children[i]=0;
                        merge=TRUE;
                    }
                    else
                        mark_weak_L (gc, child);
                }

                if (merge) {
                    xsize j;
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
                i=children->number;
                while ( i> 0) {
                    mark_L (gc, children->children[--i]);
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
    xsize parent_id;

    if (parent ==0)
        parent_id = 0;
    else {
        parent_id = map_try_id_L(gc, parent, NULL);
        x_return_if_fail(parent_id != 0);
    }
    if (prev) {
        xsize prev_id = map_try_id_L (gc, prev, NULL);
        x_return_if_fail(prev_id != 0);
        stack_push_L (gc, prev_id);
        node_add_L (gc, parent_id,prev_id | UNSET_MASK);
    }
    if (now) {
        xsize now_id = map_try_id_L (gc, now, NULL);
        x_return_if_fail(now_id != 0);
        node_add_L (gc, parent_id, now_id);
    }
}

static xptr
malloc_U                (xGC            *gc,
                         xsize          size,
                         xptr           parent,
                         xFreeFunc      finalize)
{
    xptr    p;
    xsize   h;
    xsize   id;
    x_return_val_if_fail (size != 0, NULL);

    p = x_malloc (size);

    x_mutex_lock (&gc->mutex);

    id = map_try_id_L(gc, p, &h);
    x_assert(id == 0);
    id = add_map_L (gc, h, p, finalize);

    if (parent)
        link_set_L (gc, parent, NULL, p);
    else
        stack_push_L (gc, id);

    x_mutex_unlock (&gc->mutex);
    return p;
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
    xsize i;
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
            if (p != FREED_POINTER) {
                x_free(p);
                node->u.n.address = FREED_POINTER;
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
    xsize h;

    x_return_val_if_fail (gc != NULL, NULL);

    ret = x_malloc (sizeof(struct weak_table));
    x_mutex_lock (&gc->mutex);

    ret->node_id = map_try_id_L (gc, ret, &h);
    x_assert(ret->node_id == 0);

    ret->node_id = add_map_L(gc, h, ret,NULL);
    gc->pool[ret->node_id].u.c.weak=WEAK_CONTAINER;


    if (parent)
        link_set_L (gc, parent, NULL, ret);
    else
        stack_push_L (gc, ret->node_id);

    x_mutex_unlock (&gc->mutex);
    return ret;
}

xptr
x_gc_wtable_next        (xGC            *gc,
                         xptr           wtable,
                         xsize          *iter)
{
    xsize i,j;
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
        xsize id=children->children[i];
        if (id) {
            if (gc->pool[id].u.c.address == (intptr_t)FREED_POINTER) {
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
    xsize   id;
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
        xsize parent, child;
        --s->bottom;
        parent = s->data[s->bottom-1].stack;
        child = s->data[s->bottom].stack;
        node_add_L (gc, parent, child | UNSET_MASK);
        node_free_L (gc, child);
        s->current = s->bottom-1;
        s->top = s->current + 1;
    }

    while (p){
        id =  map_try_id_L (gc, p, NULL);
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
    xsize i;

    x_return_if_fail (gc != NULL);

    x_mutex_lock (&gc->mutex);

    stack_pack_L (gc);
    cache_flush_L (gc);
    if (gc->pool) mark_L (gc, 0);
    for (i = 0; i< gc->size; ++i) {
        struct node *node = gc->pool+i;
        if (node->mark < gc->mark){
            if (node->mark >= 0) {
                xptr p = node->u.n.address;
                if (node->u.n.finalize
                    && node->u.c.weak != WEAK_CONTAINER)
                    node->u.n.finalize (p);
                if (p != FREED_POINTER) {
                    map_erase_L (gc, i);
                    node->u.n.address = FREED_POINTER;
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
            node->u.n.address = FREED_POINTER;
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
        xsize new_id, old_id;
        xsize h;
        struct link *tmp;

        x_mutex_lock (&gc->mutex);

        old_id = map_try_id_L(gc, p, &h);
        if (old_id == 0) {
            x_mutex_unlock (&gc->mutex);
            x_free(ret);
            x_return_val_if_fail(old_id == 0, NULL);
        }

        new_id = map_try_id_L(gc, ret, &h);
        x_assert(new_id == 0);
        new_id = add_map_L (gc, h, ret, NULL);

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
        gc->pool[old_id].u.c.address = (intptr_t)FREED_POINTER;

        x_mutex_unlock (&gc->mutex);
    }

    return ret;
}

xptr
x_gc_clone              (xGC            *gc,
                         xptr           from,
                         xsize          sz)
{
    xsize from_id, to_id;
    xptr  to;
    xsize h;
    struct link *from_children;

    x_return_val_if_fail (gc != NULL, NULL);
    x_return_val_if_fail (from != NULL, NULL);
    x_return_val_if_fail (sz >0, NULL);

    to = x_malloc (sz);

    x_mutex_lock (&gc->mutex);

    from_id = map_try_id_L(gc, from, &h);
    if (from_id == 0) {
        x_mutex_unlock (&gc->mutex);
        x_free(to);
        x_return_val_if_fail(from_id == 0, NULL);
    }
    to_id = map_try_id_L(gc, to, &h);
    x_assert(to_id == 0);
    to_id = add_map_L(gc,h, to, NULL);

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
    xsize       i;
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
    if (gc->pool) mark_L (gc, 0);
    for (i=0;i<gc->size;i++) {
        const struct link *link=gc->pool[i].u.n.children;
        const xptr p = gc->pool[i].u.n.address;
        xchar status;
        const char* format;


        if (gc->pool[i].mark >= gc->mark+1)
            status = ' ';
        else if (gc->pool[i].mark == gc->mark)
            status = 'W';
        else if (gc->pool[i].mark >= 0)
            status = 'X';
        else {
            continue;
        }
        if (gc->pool[i].u.c.weak == WEAK_CONTAINER)
            format = "%d%c[wtable]\t:";
        else if (i == 0)
            format = "%d%c[global]\t:";
        else if (gc->pool[i].u.n.address == (xptr)FREED_POINTER)
            format = "%d%c[stack]\t:";
        else
            format = "%d%c[%p]\t:";

        x_string_append_printf (string, format, i, status, p);

        if (link) {
            xsize j;
            for (j=0;j<link->number;j++) {
                x_string_append_printf (string, "%"X_SIZE_FORMAT,link->children[j]);
            }
        }

        x_string_append_char(string, '\n');
    }
    gc->mark++;

    x_mutex_unlock (&gc->mutex);

    return x_string_delete (string, FALSE);
}
