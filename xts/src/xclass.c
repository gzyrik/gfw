#include "config.h"
#include "xclass.h"
#include "xparam.h"
#include "xclass-prv.h"
#include "xatomicarray.h"
#include "xtypeplugin.h"
#include "xvalue.h"
#include "xvalist.h"
#include <string.h>

typedef struct _TypeNode        TypeNode;
typedef struct _CommonData      CommonData;
typedef struct _BoxedData       BoxedData;
typedef struct _IFaceData       IFaceData;
typedef struct _ClassData       ClassData;
typedef struct _InstanceData    InstanceData;
typedef union  _TypeData        TypeData;
typedef struct _IFaceEntries    IFaceEntries;
typedef struct _IFaceEntry      IFaceEntry;
typedef struct _IFaceHolder     IFaceHolder;
typedef struct _QData           QData;
typedef struct _QDataList       QDataList;

typedef enum {
    UNINITIALIZED,
    BASE_CLASS_INIT,
    BASE_IFACE_INIT,
    CLASS_INIT,
    IFACE_INIT,
    INITIALIZED
} InitState;

struct _QDataList
{
    xuint               n_qdatas;
    QData               *qdatas;
};

struct _QData
{
    xQuark              quark;
    xptr                data;
};

struct _CommonData
{
    xValueTable         *value_table;
};

struct _BoxedData
{
    CommonData          common;
    xDupFunc            copy_func;
    xFreeFunc           free_func;
};

struct _IFaceData
{
    CommonData          common;
    xuint16             vtable_size;
    xBaseInitFunc       vtable_init_base;
    xBaseFinalizeFunc   vtable_finalize_base;
    xClassInitFunc      dflt_init;
    xClassFinalizeFunc  dflt_finalize;
    xcptr               dflt_data;
    xptr                dflt_vtable;
};

struct _ClassData
{
    CommonData          common;
    xuint16             class_size;
    xBaseInitFunc       class_init_base;
    xBaseFinalizeFunc   class_finalize_base;
    xClassInitFunc      class_init;
    xClassFinalizeFunc  class_finalize;
    xcptr               class_data;
    xptr                klass;

    xuint16             class_private_size;
    int volatile        init_state;
};

struct _InstanceData
{
    CommonData          common;
    xuint16             class_size;
    xBaseInitFunc       class_init_base;
    xBaseFinalizeFunc   class_finalize_base;
    xClassInitFunc      class_init;
    xClassFinalizeFunc  class_finalize;
    xcptr               class_data;
    xptr                klass;

    xuint16             class_private_size;
    int volatile        init_state;

    xuint16             instance_size;
    xuint16             private_size;
    xInstanceInitFunc   instance_init;
};

union _TypeData
{
    CommonData          common;
    BoxedData           boxed;
    IFaceData           iface;
    ClassData           klass;
    InstanceData        instance;

};

struct _IFaceHolder
{
    xType               instance_type;
    xIFaceInfo          *info;
    xTypePlugin         *plugin;
    IFaceHolder         *next;
};

struct _IFaceEntry
{
    xType               iface_type;
    xIFace              *vtable;
    InitState           init_state;
};

struct _IFaceEntries {
    xuint               offset_index;
    IFaceEntry          entry[1];
};

#define IFACE_ENTRIES_HEADER_SIZE (sizeof(IFaceEntries) - sizeof(IFaceEntry))
#define IFACE_ENTRIES_N_ENTRIES(_entries)                                   \
    ( (X_ATOMIC_ARRAY_DATA_SIZE((_entries)) - IFACE_ENTRIES_HEADER_SIZE)    \
      / sizeof(IFaceEntry) )

struct _TypeNode 
{
    xistr               name;
    xint volatile       ref_count;
    xuint               n_children;
    xuint               n_supers : 8;
    xuint               n_prerequisites : 9;
    xuint               is_classed : 1;
    xuint               is_instantiatable : 1;
    xuint               mutatable_check_cache:1;
    xType               *children; 
    xTypePlugin         *plugin;
    TypeData * volatile data; 
    QDataList           *qdata_list;
    union {
        xAtomicArray    iface_entries;      /* for !iface types */
        xAtomicArray    offsets;
    } _prot;
    xType               *prerequisites;
    xType               supers[1];
};

#define MAX_N_SUPERS                (255)
#define MAX_N_CHILDREN              (4095)
#define MAX_N_IFACES                (255) /* Limited by offsets being 8 bits */
#define MAX_N_PREREQUISITES         (511)

#define X_TYPE_FUNDAMENTAL_NUM      (0xFF)

#define X_TYPE_FUNDAMENTAL_MAX                                              \
    (X_TYPE_FUNDAMENTAL_NUM << X_TYPE_FUNDAMENTAL_SHIFT)

#define TYPE_ID_MASK                                                        \
    ((xType) ((1 << X_TYPE_FUNDAMENTAL_SHIFT) - 1))
#define TYPE_FLAG_MASK                                                      \
    (X_TYPE_ABSTRACT | X_TYPE_VALUE_ABSTRACT)

#define TYPE_FUNDAMENTAL_FLAG_MASK (X_TYPE_CLASSED |                   \
                                    X_TYPE_INSTANTIATABLE |            \
                                    X_TYPE_DERIVABLE |                 \
                                    X_TYPE_DEEP_DERIVABLE)

#define STRUCT_ALIGNMENT            (4 * sizeof (xsize))

#define ALIGN_STRUCT(offset)                                                \
    (((offset) + (STRUCT_ALIGNMENT - 1)) & (-(int)STRUCT_ALIGNMENT))

#define NODE_TYPE(node)             (node->supers[0])

#define NODE_NAME(node)             (node->name)

#define NODE_REFCOUNT(node)         (node->ref_count)

#define SIZEOF_BASE_TYPE_NODE                                               \
    (X_STRUCT_OFFSET (TypeNode, supers))

#define SIZEOF_FUNDAMENTAL_INFO                                             \
    ((xssize) MAX (MAX (sizeof (xFundamentalInfo), sizeof (xptr)),          \
                   sizeof (xlong)))

#define NODE_PARENT_TYPE(node)      (node->supers[1])

#define NODE_FUNDAMENTAL_TYPE(node) (node->supers[node->n_supers])

#define NODE_IS_ANCESTOR(ancestor, node)                                    \
    ((ancestor)->n_supers <= (node)->n_supers &&                            \
     (node)->supers[(node)->n_supers - (ancestor)->n_supers]                \
     == NODE_TYPE (ancestor))

#define NODE_IS_IFACE(node)                                                 \
    (NODE_FUNDAMENTAL_TYPE (node) == X_TYPE_IFACE)

#define NODE_IS_BOXED(node)                                                 \
    (NODE_FUNDAMENTAL_TYPE (node) == X_TYPE_BOXED)

#define CLASS_NODE_IFACE_ENTRIES(node)   (&(node)->_prot.iface_entries)
#define CLASS_NODE_IFACE_ENTRIES_LOCKED(node)                                    \
    (X_ATOMIC_ARRAY_GET_LOCKED (CLASS_NODE_IFACE_ENTRIES((node)), IFaceEntries))

#define IFACE_NODE_N_PREREQUISITES(node)  ((node)->n_prerequisites)
#define IFACE_NODE_PREREQUISITES(node)    ((node)->prerequisites)
#define IFACE_NODE_IFACE_OFFSETS(node)   (&(node)->_prot.offsets)
#define IFACE_NODE_IFACE_OFFSETS_LOCKED(node)                                    \
    (X_ATOMIC_ARRAY_GET_LOCKED (IFACE_NODE_IFACE_OFFSETS((node)), xuint8))


#define INVALID_RECURSION(func, arg, type_name) X_STMT_START{               \
    static xcstr _action = " invalidly modified type ";                     \
    xptr _arg = (xptr) (arg);                                               \
    xcstr _tname = (type_name), _fname = (func);                            \
    if (_arg != NULL)                                                       \
    x_error ("%s(%p)%s`%s'", _fname, _arg, _action, _tname);                \
    else                                                                    \
    x_error ("%s()%s`%s'", _fname, _action, _tname);                        \
}X_STMT_END

#define x_return_val_if_uninitialized(return_value) X_STMT_START{           \
    if (X_UNLIKELY (!_quark_type_flags)){                             \
        x_log (X_LOG_DOMAIN, X_LOG_LEVEL_CRITICAL,                                \
               "%s: You forgot to call x_type_init()",                      \
               X_STRLOC);                                                   \
        return (return_value);                                              \
    }                                                                       \
}X_STMT_END


X_RECLOCK_DEFINE_STATIC (class_init);
X_RWLOCK_DEFINE_STATIC  (type_nodes);
static xHashTable         *_type_nodes_ht = NULL;

X_LOCK_DEFINE_STATIC    (real_class);
static xHashTable         *_real_class_ht = NULL;

static TypeNode         *_fundamental_nodes[X_TYPE_FUNDAMENTAL_NUM+1]={NULL,};
static xType            _fundamental_next = 1;
static xQuark           _quark_iface_holder;
static xQuark           _quark_type_flags;
static xQuark           _quark_dependants_array;
static xQuark           _quark_type_mime;
/* LOCKING:
 * lock handling issues when calling static functions are indicated by
 * uppercase letter postfixes, all static functions have to have
 * one of the below postfixes:
 * - _I:    [Indifferent about locking]
 *   function doesn't care about locks at all
 * - _U:    [Unlocked invocation]
 *   no read or write lock has to be held across function invocation
 *   (locks may be acquired and released during invocation though)
 * - _L:    [Locked invocation]
 *   a write lock or more than 0 read locks have to be held across
 *   function invocation
 * - _W:    [Write-locked invocation]
 *   a write lock has to be held across function invocation
 * - _Wm:   [Write-locked invocation, mutatable]
 *   like _W, but the write lock might be released and reacquired
 *   during invocation, watch your pointers
 * - _WmREC:    [Write-locked invocation, mutatable, recursive]
 *   like _Wm, but also acquires recursive mutex `class_init'
 *
 * type_*  通用结点操作
 * iface_* 接口结点操作
 * class_* 类结点操作
 * 单词 vtable 等价于xIFace*,即接口的实现(虚函数表)
 */
static inline void
type_data_unref_U       (TypeNode       *node,
                         xbool          uncached);
static void
type_init               (void);
/* 查找结点 */
static inline TypeNode *
lookup_type_node_I      (xType          type)
{
    TypeNode    *node;
    if (type > X_TYPE_FUNDAMENTAL_MAX)
        node = (TypeNode *) (type & ~TYPE_ID_MASK);
    else
        node = _fundamental_nodes[type >> X_TYPE_FUNDAMENTAL_SHIFT];
    return node;
}
/* 类型名 */
static inline xcstr
type_name_I             (xType          type)
{
    if (type) {
        TypeNode *node = lookup_type_node_I (type);
        return node ? NODE_NAME (node) : "<unknown>";
    }
    return "<invalid>";
}
/* 基类型信息 */
static inline xFundamentalInfo*
type_fundamental_info_I (TypeNode       *node)
{
    xType ftype = NODE_FUNDAMENTAL_TYPE (node);

    if (ftype != NODE_TYPE (node))
        node = lookup_type_node_I (ftype);

    return node ? X_STRUCT_MEMBER_P (node, -SIZEOF_FUNDAMENTAL_INFO) : NULL;
}
/* 二分法查找结点中quark -> data */
static inline xptr
type_get_qdata_L        (TypeNode       *node,
                         xQuark         quark)
{
    QDataList *gdata = node->qdata_list;

    if (quark && gdata && gdata->n_qdatas) {
        QData *qdatas = gdata->qdatas - 1;
        xuint n_qdatas = gdata->n_qdatas;

        do {
            xuint i;
            QData *check;

            i = (n_qdatas + 1) / 2;
            check = qdatas + i;
            if (quark == check->quark)
                return check->data;
            else if (quark > check->quark) {
                n_qdatas -= i;
                qdatas = check;
            }
            else /* if (quark < check->quark) */
                n_qdatas = i - 1;
        }
        while (n_qdatas);
    }
    return NULL;
}
/* 设置结点中quark -> data */
static inline void
type_set_qdata_W        (TypeNode       *node,
                         xQuark         quark,
                         xptr           data)
{
    QDataList       *gdata;
    QData           *qdata;
    xuint           i;

    /* setup qdata list if necessary */
    if (!node->qdata_list)
        node->qdata_list = x_malloc0 (sizeof(QDataList));
    gdata = node->qdata_list;

    /* try resetting old data */
    qdata = gdata->qdatas;
    for (i = 0; i < gdata->n_qdatas; i++){
        if (qdata[i].quark == quark) {
            qdata[i].data = data;
            return;
        }
    }

    /* add new entry */
    gdata->n_qdatas++;
    gdata->qdatas = x_renew (QData, gdata->qdatas, gdata->n_qdatas);
    qdata = gdata->qdatas;
    for (i = 0; i < gdata->n_qdatas - 1; i++)
        if (qdata[i].quark > quark)
            break;
    memmove (qdata + i + 1, qdata + i,
             sizeof (qdata[0]) * (gdata->n_qdatas - i - 1));
    qdata[i].quark = quark;
    qdata[i].data = data;
}
/* 返回接口结点中的IFaceHolder链表 */
#define iface_get_holders_L(node)                                   \
    ((IFaceHolder*) type_get_qdata_L ((node), _quark_iface_holder))
/* 设置接口结点中的IFaceHolder链表 */
#define iface_set_holders_W(node, holders)                          \
    (type_set_qdata_W ((node), _quark_iface_holder, (holders)))
#define	iface_get_dependants_array_L(node)	                        \
    ((xType*) type_get_qdata_L ((node), _quark_dependants_array))
#define	iface_set_dependants_array_W(node,dependants)	            \
    (type_set_qdata_W ((node), _quark_dependants_array, (dependants)))

/* 在类结点的接口表中, 查找的接口项 */
static inline IFaceEntry*
lookup_iface_entry_I    (volatile IFaceEntries *entries,
                         TypeNode       *iface_node)
{
    xuint8      *offsets;
    xuint       offset_index;
    IFaceEntry  *check;
    int         index;
    IFaceEntry  *entry;

    if (entries == NULL) return NULL;

    X_ATOMIC_ARRAY_BEGIN_TRANSACTION (IFACE_NODE_IFACE_OFFSETS (iface_node),
                                      offsets);
    entry = NULL;
    offset_index = entries->offset_index;
    if (offsets != NULL &&
        offset_index < X_ATOMIC_ARRAY_DATA_SIZE(offsets)) {
        index = offsets[offset_index];
        if (index > 0) {
            /* zero means unset, subtract one to get real index */
            index -= 1;

            if (index < (int)IFACE_ENTRIES_N_ENTRIES (entries)) {
                check = (IFaceEntry *)&entries->entry[index];
                if (check->iface_type == NODE_TYPE (iface_node))
                    entry = check;
            }
        }
    }
    X_ATOMIC_ARRAY_END_TRANSACTION (offsets);

    return entry;
}
/* 类结点中, 查找接口项 */
static inline IFaceEntry*
class_peek_iface_entry_L(TypeNode       *node,
                         TypeNode       *iface_node)
{
    if (!NODE_IS_IFACE (iface_node))
        return NULL;

    return lookup_iface_entry_I (CLASS_NODE_IFACE_ENTRIES_LOCKED (node),
                                 iface_node);
}
/* 类结点中,查找接口实现 */
static inline xbool
class_peek_vtable_I     (TypeNode       *node,
                         TypeNode       *iface_node,
                         xptr           *vtable_ptr)
{
    volatile IFaceEntries   *entries;
    IFaceEntry  *entry;
    xbool       res;

    if (!NODE_IS_IFACE (iface_node)) {
        if (vtable_ptr)
            *vtable_ptr = NULL;
        return FALSE;
    }

    X_ATOMIC_ARRAY_BEGIN_TRANSACTION (CLASS_NODE_IFACE_ENTRIES (node),
                                      entries);
    entry = lookup_iface_entry_I (entries, iface_node);
    res = entry != NULL;
    if (vtable_ptr) {
        if (entry)
            *vtable_ptr = entry->vtable;
        else
            *vtable_ptr = NULL;
    }
    X_ATOMIC_ARRAY_END_TRANSACTION (entries);
    return res;
}
/* 接口是否依赖prerequisite_type */
static inline xbool
iface_has_prerequisite_L(TypeNode       *iface_node,
                         xType          prerequisite_type)
{
    xType *prerequisites;
    xuint n_prerequisites;

    if (!NODE_IS_IFACE (iface_node)
        || !IFACE_NODE_N_PREREQUISITES (iface_node))
        return FALSE;

    prerequisites = IFACE_NODE_PREREQUISITES (iface_node) - 1;
    n_prerequisites = IFACE_NODE_N_PREREQUISITES (iface_node);

    do {
        xuint i;
        xType *check;

        i = (n_prerequisites + 1) >> 1;
        check = prerequisites + i;
        if (prerequisite_type == *check)
            return TRUE;
        else if (prerequisite_type > *check) {
            n_prerequisites -= i;
            prerequisites = check;
        }
        else /* if (prerequisite_type < *check) */
            n_prerequisites = i - 1;
    } while (n_prerequisites);
    return FALSE;
}
/* 按要求检查类与接口间的关系是否合法 */
static inline xbool
check_conformities_UorL (TypeNode       *node,
                         TypeNode       *iface_node,
                         xbool          support_interfaces,
                         xbool          support_prerequisites,
                         xbool          have_lock)
{
    xbool   match;

    if (NODE_IS_ANCESTOR (iface_node, node))
        return TRUE;

    support_interfaces = support_interfaces
        && node->is_instantiatable && NODE_IS_IFACE (iface_node);
    support_prerequisites = support_prerequisites && NODE_IS_IFACE (node);
    match = FALSE;
    if (support_interfaces) {
        if (have_lock) {
            if (class_peek_iface_entry_L (node, iface_node))
                match = TRUE;
        }
        else {
            if (class_peek_vtable_I (node, iface_node, NULL))
                match = TRUE;
        }
    }
    if (!match && support_prerequisites) {
        if (!have_lock)
            X_READ_LOCK (type_nodes);
        if (support_prerequisites
            && iface_has_prerequisite_L (node, NODE_TYPE (iface_node)))
            match = TRUE;
        if (!have_lock)
            X_READ_UNLOCK (type_nodes);
    }
    return match;
}
/* 检查类型转换是否合法 */
static xbool
type_is_a_L             (TypeNode       *node,
                         TypeNode       *iface_node)
{
    return check_conformities_UorL (node, iface_node, TRUE, TRUE, TRUE);
}
/* 检查类型间关系是否合法 */
static inline xbool
type_conforms_to_U      (TypeNode       *node,
                         TypeNode       *iface_node,
                         xbool          support_interfaces,
                         xbool          support_prerequisites)
{
    return check_conformities_UorL (node, iface_node,
                                    support_interfaces,
                                    support_prerequisites,
                                    FALSE);
}
/* 查找接口结点中instance_type的定义IFaceHolder */
static IFaceHolder*
iface_peek_holder_L     (TypeNode       *iface,
                         xType          instance_type)
{
    IFaceHolder *iholder;

    x_assert (NODE_IS_IFACE (iface));

    iholder = iface_get_holders_L (iface);
    while (iholder && iholder->instance_type != instance_type)
        iholder = iholder->next;
    return iholder;
}

/* xIFace结点中offset处是否有效(为空或等于for_index)*/
static xbool
iface_has_available_offset_L  (TypeNode       *iface_node,
                               int            offset,
                               int            for_index)
{
    xuint8 *offsets;

    offsets = IFACE_NODE_IFACE_OFFSETS_LOCKED (iface_node);
    if (offsets == NULL)
        return TRUE;

    if ((int)X_ATOMIC_ARRAY_DATA_SIZE (offsets) <= offset)
        return TRUE;

    if (offsets[offset] == 0 ||
        offsets[offset] == for_index+1)
        return TRUE;

    return FALSE;
}

/* 查找能满足所有接口的映射偏移 */
static int
find_free_iface_offset_L  (IFaceEntries   *entries)
{
    IFaceEntry *entry;
    TypeNode *iface_node;
    int offset;
    int i;
    int n_entries;

    n_entries = IFACE_ENTRIES_N_ENTRIES (entries);
    offset = -1;
    do {
        offset++;
        for (i = 0; i < n_entries; i++) {
            entry = &entries->entry[i];
            iface_node = lookup_type_node_I (entry->iface_type);

            if (!iface_has_available_offset_L (iface_node, offset, i))
                break;
        }
    }
    while (i != n_entries);

    return offset;
}
/* 递归查找首个实现iface接口的子结点 */
static TypeNode*
class_find_conforming_child_L (TypeNode       *pnode,
                               TypeNode       *iface)
{
    TypeNode    *cnode;
    TypeNode    *node;
    xuint       i;

    if (class_peek_iface_entry_L (pnode, iface))
        return pnode;

    node = NULL;
    for (i = 0; i < pnode->n_children && !node; i++) {
        cnode = lookup_type_node_I (pnode->children[i]);
        node = class_find_conforming_child_L (cnode, iface);
    }

    return node;
}
/* 检查类结点是否可实现接口 */
static xbool
check_class_add_iface_L (xType       instance_type,
                         xType       iface_type)
{
    TypeNode        *node;
    TypeNode        *iface;
    IFaceEntry*     entry;
    TypeNode        *tnode;
    xType           *prerequisites;
    xuint           i;

    node = lookup_type_node_I (instance_type);

    if (!node || !node->is_instantiatable) {
        x_warning ("cannot add interfaces to invalid "
                   "(non-instantiatable) type `%s'",
                   type_name_I (instance_type));
        return FALSE;
    }
    iface = lookup_type_node_I (iface_type);
    if (!iface || !NODE_IS_IFACE (iface)) {
        x_warning ("cannot add invalid (non-interface) type `%s' to type `%s'",
                   type_name_I (iface_type),
                   NODE_NAME (node));
        return FALSE;
    }
    tnode = lookup_type_node_I (NODE_PARENT_TYPE (iface));
    if (NODE_PARENT_TYPE (tnode) && !class_peek_iface_entry_L (node, tnode))
    {
        /* 2001/7/31:timj: erk,
         * i guess this warning is junk as interface derivation is flat */
        x_warning ("cannot add sub-interface `%s' to type `%s' "
                   "which does not conform to super-interface `%s'",
                   NODE_NAME (iface),
                   NODE_NAME (node),
                   NODE_NAME (tnode));
        return FALSE;
    }

    /* 若已存在,则须还未定义 */
    entry = class_peek_iface_entry_L (node, iface);
    if (entry && entry->vtable == NULL
        && !iface_peek_holder_L (iface, NODE_TYPE (node))) {
        return TRUE;
    }
    /* 须没有子结点实现该接口 */
    tnode = class_find_conforming_child_L (node, iface);
    if (tnode) {
        x_warning ("cannot add interface type `%s' to type `%s', "
                   "since child type `%s' already conforms to interface",
                   NODE_NAME (iface), NODE_NAME (node), NODE_NAME (tnode));
        return FALSE;
    }
    /* node须可实现必备的其它接口 */
    prerequisites = IFACE_NODE_PREREQUISITES (iface);
    for (i = 0; i < IFACE_NODE_N_PREREQUISITES (iface); i++) {
        tnode = lookup_type_node_I (prerequisites[i]);
        if (!type_is_a_L (node, tnode))
        {
            x_warning ("cannot add interface type `%s' to type `%s' "
                       "which does not conform to prerequisite `%s'",
                       NODE_NAME (iface), NODE_NAME (node), NODE_NAME (tnode));
            return FALSE;
        }
    }
    return TRUE;
}
/* 设置接口结点的映射关系 */
static void
iface_set_offset_L      (TypeNode       *iface_node,
                         int            offset,
                         int            index)
{
    xuint8      *offsets, *old_offsets;
    int         new_size, old_size;
    int         i;

    old_offsets = IFACE_NODE_IFACE_OFFSETS_LOCKED (iface_node);
    if (old_offsets == NULL)
        old_size = 0;
    else {
        old_size = X_ATOMIC_ARRAY_DATA_SIZE (old_offsets);
        if (offset < old_size &&
            old_offsets[offset] == index + 1)
            return; /* Already set to this index, return */
    }
    new_size = MAX (old_size, offset + 1);

    offsets = x_atomic_array_copy (&iface_node->_prot.offsets,
                                   0, new_size - old_size);

    /* Mark new area as unused */
    for (i = old_size; i < new_size; i++)
        offsets[i] = 0;

    offsets[offset] = index + 1;

    x_atomic_array_update (&iface_node->_prot.offsets, offsets);
}

/* 递归类结点添加接口项 */
static void
class_add_iface_entry_W (TypeNode       *node,
                         xType          iface_type,
                         IFaceEntry     *parent_entry)
{
    TypeNode        *iface_node;
    IFaceEntries    *entries;
    IFaceEntry      *entry;
    xuint           i,j;
    xuint           num_entries;

    x_assert (node->is_instantiatable);

    entries = CLASS_NODE_IFACE_ENTRIES_LOCKED (node);
    if (entries != NULL) {
        num_entries = IFACE_ENTRIES_N_ENTRIES (entries);

        x_assert (num_entries < MAX_N_IFACES);

        for (i = 0; i < num_entries; i++) {
            entry = &entries->entry[i];
            if (entry->iface_type == iface_type) {
                /* 已存在项的情况,合法的只有两种:
                 * 1 当前拥有独立的项, 但须在UNINITIALIZED状态时,
                 * 2 子结点已有该接口后,此时正在上层结点中添加.
                 */
                if (!parent_entry)
                    x_assert (entry->vtable == NULL
                              && entry->init_state == UNINITIALIZED);
                else
                    ;/* 情况2时, 子结点已设置过该接口项, 无须操作 */
                return;
            }
        }
    }

    /* 不存情况,须添加项 */
    entries = x_atomic_array_copy (CLASS_NODE_IFACE_ENTRIES (node),
                                   IFACE_ENTRIES_HEADER_SIZE,
                                   sizeof (IFaceEntry));
    num_entries = IFACE_ENTRIES_N_ENTRIES (entries);
    i = num_entries - 1;
    if (i == 0)
        entries->offset_index = 0;
    entries->entry[i].iface_type = iface_type;
    entries->entry[i].vtable = NULL;
    entries->entry[i].init_state = UNINITIALIZED;

    /* 子结点被动实现项,从上层复制 */
    if (parent_entry) {
        if (node->data && node->data->klass.init_state >= BASE_IFACE_INIT){
            entries->entry[i].init_state = INITIALIZED;
            entries->entry[i].vtable = parent_entry->vtable;
        }
    }

    /* 更新entries与iface_node中映射关系 */
    iface_node = lookup_type_node_I (iface_type);

    if (iface_has_available_offset_L (iface_node,
                                      entries->offset_index,
                                      i)) {
        iface_set_offset_L (iface_node,
                            entries->offset_index, i);
    }
    else {
        entries->offset_index =
            find_free_iface_offset_L (entries);
        for (j = 0; j < IFACE_ENTRIES_N_ENTRIES (entries); j++) {
            entry = &entries->entry[j];
            iface_node = lookup_type_node_I (entry->iface_type);
            iface_set_offset_L (iface_node,
                                entries->offset_index, j);
        }
    }

    x_atomic_array_update (CLASS_NODE_IFACE_ENTRIES (node), entries);

    if (parent_entry) {
        for (i = 0; i < node->n_children; i++) {
            TypeNode *cnode = lookup_type_node_I (node->children[i]);
            class_add_iface_entry_W (cnode, iface_type, &entries->entry[i]);
        }
    }
}
/* 检查 xTypeInfo info 的合法性 */
static xbool
check_type_info_I       (TypeNode       *pnode,
                         xType          ftype,
                         xcstr          type_name,
                         const xTypeInfo      *info)
{
    xFundamentalInfo *finfo;
    xbool is_interface = (ftype == X_TYPE_IFACE);

    x_assert (ftype <= X_TYPE_FUNDAMENTAL_MAX && !(ftype & TYPE_ID_MASK));

    finfo = type_fundamental_info_I (lookup_type_node_I (ftype));
    /* check instance members */
    if (!(finfo->type_flags & X_TYPE_INSTANTIATABLE) &&
        (info->instance_size || info->instance_init))
    {
        if (pnode)
            x_warning ("cannot instantiate `%s', "
                       "derived from non-instantiatable parent type `%s'",
                       type_name,
                       NODE_NAME (pnode));
        else
            x_warning ("cannot instantiate `%s' "
                       "as non-instantiatable fundamental",
                       type_name);
        return FALSE;
    }
    /* check class & interface members */
    if (!((finfo->type_flags & X_TYPE_CLASSED) || is_interface) &&
        (info->class_init || info->class_finalize || info->class_data ||
         info->class_size || info->base_init || info->base_finalize))
    {
        if (pnode)
            x_warning ("cannot create class for `%s', "
                       "derived from non-classed parent type `%s'",
                       type_name,
                       NODE_NAME (pnode));
        else
            x_warning ("cannot create class for `%s' "
                       "as non-classed fundamental",
                       type_name);
        return FALSE;
    }
    /* check interface size */
    if (is_interface && info->class_size < sizeof (xIFace)) {
        x_warning ("specified interface size for type `%s' "
                   "is smaller than `xTypeIFace' size",
                   type_name);
        return FALSE;
    }
    /* check class size */
    if (finfo->type_flags & X_TYPE_CLASSED) {
        if (info->class_size < sizeof (xClass)) {
            x_warning ("specified class size for type `%s' "
                       "is smaller than `xClass' size",
                       type_name);
            return FALSE;
        }
        if (pnode && info->class_size < pnode->data->klass.class_size) {
            x_warning ("specified class size for type `%s' is smaller "
                       "than the parent type's `%s' class size",
                       type_name,
                       NODE_NAME (pnode));
            return FALSE;
        }
    }
    /* check instance size */
    if (finfo->type_flags & X_TYPE_INSTANTIATABLE) {
        if (info->instance_size < sizeof (xInstance)) {
            x_warning ("specified instance size for type `%s' "
                       "is smaller than `xInstance' size",
                       type_name);
            return FALSE;
        }
        if (pnode &&
            info->instance_size < pnode->data->instance.instance_size) {
            x_warning ("specified instance size for type `%s' is smaller "
                       "than the parent type's `%s' instance size",
                       type_name,
                       NODE_NAME (pnode));
            return FALSE;
        }
    }

    return TRUE;
}
/* 装配TypeData */
static void
type_data_make_W        (TypeNode           *node,
                         const xTypeInfo    *info,
                         const xValueTable  *value_table)
{
    TypeNode    *pnode;
    TypeData    *data;
    xValueTable *vtable = NULL;
    xuint       vtable_size = 0;

    x_assert (node->data == NULL && info != NULL);

    if (!value_table) {
        pnode = lookup_type_node_I (NODE_PARENT_TYPE (node));

        if (pnode)
            vtable = pnode->data->common.value_table;
        else {
            static const xValueTable zero_vtable = { NULL, };
            value_table = &zero_vtable;
        }
    }
    if (value_table) {
        vtable_size = sizeof (xValueTable);
        if (value_table->collect_format)
            vtable_size += strlen (value_table->collect_format);
        if (value_table->lcopy_format)
            vtable_size += strlen (value_table->lcopy_format);
        vtable_size += 2;
    }

    if (node->is_instantiatable) {
        pnode = lookup_type_node_I (NODE_PARENT_TYPE (node));

        data = x_malloc0 (sizeof (InstanceData) + vtable_size);
        if (vtable_size)
            vtable = X_STRUCT_MEMBER_P (data, sizeof (InstanceData));
        data->instance.class_size = info->class_size;
        data->instance.class_init_base = info->base_init;
        data->instance.class_finalize_base = info->base_finalize;
        data->instance.class_init = info->class_init;
        data->instance.class_finalize = info->class_finalize;
        data->instance.class_data = info->class_data;
        data->instance.klass = NULL;
        data->instance.init_state = UNINITIALIZED;
        data->instance.instance_size = info->instance_size;
        data->instance.private_size = 0;
        data->instance.class_private_size = 0;
        data->instance.instance_init = info->instance_init;
        if (pnode)
            data->instance.class_private_size =
                pnode->data->instance.class_private_size;
    }
    else if (node->is_classed) {
        pnode = lookup_type_node_I (NODE_PARENT_TYPE (node));

        data = x_malloc0 (sizeof (ClassData) + vtable_size);
        if (vtable_size)
            vtable = X_STRUCT_MEMBER_P (data, sizeof (ClassData));
        data->klass.class_size = info->class_size;
        data->klass.class_init_base = info->base_init;
        data->klass.class_finalize_base = info->base_finalize;
        data->klass.class_init = info->class_init;
        data->klass.class_finalize = info->class_finalize;
        data->klass.class_data = info->class_data;
        data->klass.klass = NULL;
        data->klass.class_private_size = 0;
        data->klass.init_state = UNINITIALIZED;
        if (pnode)
            data->klass.class_private_size =
                pnode->data->klass.class_private_size;
    }
    else if (NODE_IS_IFACE (node)) {
        data = x_malloc0 (sizeof (IFaceData) + vtable_size);
        if (vtable_size)
            vtable = X_STRUCT_MEMBER_P (data, sizeof (IFaceData));
        data->iface.vtable_size = info->class_size;
        data->iface.vtable_init_base = info->base_init;
        data->iface.vtable_finalize_base = info->base_finalize;
        data->iface.dflt_init = info->class_init;
        data->iface.dflt_finalize = info->class_finalize;
        data->iface.dflt_data = info->class_data;
        data->iface.dflt_vtable = NULL;
    }
    else if (NODE_IS_BOXED (node)) {
        data = x_malloc0 (sizeof (BoxedData) + vtable_size);
        if (vtable_size)
            vtable = X_STRUCT_MEMBER_P (data, sizeof (BoxedData));
    }
    else {
        data = x_malloc0 (sizeof (CommonData) + vtable_size);
        if (vtable_size)
            vtable = X_STRUCT_MEMBER_P (data, sizeof (CommonData));
    }

    node->data = data;

    if (vtable_size) {
        xchar *p;
        *vtable = *value_table;
        p = X_STRUCT_MEMBER_P (vtable, sizeof (*vtable));
        p[0] = 0;
        vtable->collect_format = p;
        if (value_table->collect_format) {
            strcat (p, value_table->collect_format);
            p += strlen (value_table->collect_format);
        }
        p++;
        p[0] = 0;
        vtable->lcopy_format = p;
        if (value_table->lcopy_format)
            strcat  (p, value_table->lcopy_format);
    }
    node->data->common.value_table = vtable;
    if (node->data->common.value_table->value_init != NULL &&
        !((X_TYPE_VALUE_ABSTRACT | X_TYPE_ABSTRACT) &
          X_PTR_TO_SIZE (type_get_qdata_L (node, _quark_type_flags))))
        node->mutatable_check_cache = TRUE;
    else
        node->mutatable_check_cache = FALSE;

    x_assert (node->data->common.value_table != NULL); /* paranoid */

    x_atomic_int_set (&node->ref_count, 1);
}
/* 装配Boxed中的TypeData */
void
_x_type_boxed_init      (xType          type,
                         xDupFunc       copy_func,
                         xFreeFunc      free_func)
{
    TypeNode *node = lookup_type_node_I (type);

    node->data->boxed.copy_func = copy_func;
    node->data->boxed.free_func = free_func;
}
/* Boxed 复制 */
xptr
_x_type_boxed_copy      (xType          type,
                         xptr           boxed)
{
    TypeNode *node = lookup_type_node_I (type);

    return node->data->boxed.copy_func (boxed);
}

void
_x_type_boxed_free      (xType          type,
                         xptr           boxed)
{
    TypeNode *node = lookup_type_node_I (type);

    node->data->boxed.free_func (boxed);
}

/* 检查value table中collect_format是否合法 */
static xbool
check_collect_format_I  (xcstr          collect_format)
{
    xcstr p = collect_format;
    xcstr valid_format =
        X_VALUE_COLLECT_INT X_VALUE_COLLECT_LONG \
        X_VALUE_COLLECT_INT64 X_VALUE_COLLECT_DOUBLE X_VALUE_COLLECT_PTR;

    while (*p)
        if (!strchr (valid_format, *p++))
            return FALSE;
    return p - collect_format <= X_VALUE_COLLECT_MAX_LENGTH;
}

/* 检查 value table是否合法*/
static xbool
check_value_table_I     (xcstr              type_name,
                         const xValueTable  *value_table)
{
    if (!value_table)
        return FALSE;
    else if (value_table->value_init == NULL) {
        if (value_table->value_clear || value_table->value_copy ||
            value_table->value_peek ||
            value_table->collect_format || value_table->value_collect ||
            value_table->lcopy_format || value_table->value_lcopy)
            x_warning ("cannot handle uninitializable values of type `%s'",
                       type_name);
        return FALSE;
    }
    else {/* value_table->value_init != NULL */
        if (!value_table->value_clear) {
            /* +++ optional +++
             *x_warning ("missing `value_free()' for type `%s'", type_name);
             * return FALSE;
             */
        }
        if (!value_table->value_copy) {
            x_warning ("missing `value_copy()' for type `%s'", type_name);
            return FALSE;
        }
        if ((value_table->collect_format || value_table->value_collect) &&
            (!value_table->collect_format || !value_table->value_collect)) {
            x_warning ("one of `collect_format' and `collect_value()' "
                       "is unspecified for type `%s'",
                       type_name);
            return FALSE;
        }
        if (value_table->collect_format
            && !check_collect_format_I (value_table->collect_format)) {
            x_warning ("the `collect_format' specification for type `%s' "
                       "is too long or invalid",
                       type_name);
            return FALSE;
        }
        if ((value_table->lcopy_format || value_table->value_lcopy) &&
            (!value_table->lcopy_format || !value_table->value_lcopy)) {
            x_warning ("one of `lcopy_format' and `lcopy_value()' "
                       "is unspecified for type `%s'",
                       type_name);
            return FALSE;
        }
        if (value_table->lcopy_format
            && !check_collect_format_I (value_table->lcopy_format)) {
            x_warning ("the `lcopy_format' specification for type `%s' "
                       "is too long or invalid",
                       type_name);
            return FALSE;
        }
    }
    return TRUE;
}
/* 增加TypeNode.data的引用计数,必要时创建 */
static inline void
type_data_ref_Wm        (TypeNode       *node)
{
    if (!node->data) {
        TypeNode *pnode = lookup_type_node_I (NODE_PARENT_TYPE (node));
        xTypeInfo tmp_info;
        xValueTable tmp_vtable;

        x_assert (node->plugin != NULL);

        if (pnode) {
            type_data_ref_Wm (pnode);
            if (node->data)
                INVALID_RECURSION ("x_type_plugin_*",
                                   node->plugin, NODE_NAME (node));
        }

        memset (&tmp_info, 0, sizeof (tmp_info));
        memset (&tmp_vtable, 0, sizeof (tmp_vtable));

        X_WRITE_UNLOCK (type_nodes);
        x_type_plugin_use (node->plugin);
        x_type_plugin_type_info (node->plugin,
                                 NODE_TYPE (node),
                                 &tmp_info,
                                 &tmp_vtable);
        X_WRITE_LOCK (type_nodes);
        if (node->data)
            INVALID_RECURSION ("x_type_plugin_*",
                               node->plugin, NODE_NAME (node));

        check_type_info_I (pnode, NODE_FUNDAMENTAL_TYPE (node),
                           NODE_NAME (node), &tmp_info);
        type_data_make_W (node, &tmp_info,
                          check_value_table_I (NODE_NAME (node), &tmp_vtable)
                          ? &tmp_vtable : NULL);
    }
    else {
        x_assert (NODE_REFCOUNT (node) > 0);
        x_atomic_int_inc ((int *) &node->ref_count);
    }
}
static xbool
check_iface_info_I      (TypeNode       *iface,
                         xType          instance_type,
                         const xIFaceInfo *info)
{
    if ((info->iface_finalize || info->iface_data) && !info->iface_init)
    {
        x_warning ("interface type `%s' for type `%s' "
                   "comes without initializer",
                   NODE_NAME (iface),
                   type_name_I (instance_type));
        return FALSE;
    }
    return TRUE;
}

/* 引用接口结点中的实现定义IFaceHolder,必要时动态加载 */
static IFaceHolder*
iface_retrieve_iface_holder_Wm(TypeNode       *iface,
                               xType          instance_type,
                               xbool          need_info)
{
    IFaceHolder     *iholder;

    iholder = iface_peek_holder_L (iface, instance_type);
    if (iholder && !iholder->info && need_info) {
        xIFaceInfo  tmp_info;
        x_assert (iholder->plugin != NULL);

        type_data_ref_Wm (iface);
        if (iholder->info)
            INVALID_RECURSION ("x_type_plugin_*",
                               iface->plugin,
                               NODE_NAME (iface));

        memset (&tmp_info, 0, sizeof (tmp_info));

        X_WRITE_UNLOCK (type_nodes);
        x_type_plugin_use (iholder->plugin);
        x_type_plugin_iface_info (iholder->plugin,
                                  instance_type,
                                  NODE_TYPE (iface),
                                  &tmp_info);
        X_WRITE_LOCK (type_nodes);
        if (iholder->info)
            INVALID_RECURSION ("x_type_plugin_*",
                               iholder->plugin,
                               NODE_NAME (iface));


        if (check_iface_info_I (iface, instance_type, &tmp_info))
            iholder->info = x_memdup (&tmp_info, sizeof (tmp_info));
    }

    return iholder; /* we don't modify write lock upon returning NULL */
}
/* 放弃接口结点中的实现定义IFaceHolder,必要时动态卸载 */
static void
iface_blow_holder_Wm    (TypeNode       *iface,
                         xType          instance_type)
{
    IFaceHolder *iholder = iface_get_holders_L (iface);

    x_assert (NODE_IS_IFACE (iface));

    while (iholder->instance_type != instance_type)
        iholder = iholder->next;

    if (iholder->info && iholder->plugin) {
        x_free (iholder->info);
        iholder->info = NULL;

        X_WRITE_UNLOCK (type_nodes);
        x_type_plugin_unuse (iholder->plugin);
        type_data_unref_U (iface, FALSE);
        X_WRITE_LOCK (type_nodes);
    }
}

/* 确保已生成接口结点中的dflt_vtable */
static void
iface_ensure_dflt_vtable_Wm (TypeNode *iface)
{
    x_assert (iface->data);

    if (!iface->data->iface.dflt_vtable) {
        xIFace *vtable = x_malloc0 (iface->data->iface.vtable_size);
        iface->data->iface.dflt_vtable = vtable;
        vtable->type = NODE_TYPE (iface);
        vtable->instance_type = 0;
        if (iface->data->iface.vtable_init_base ||
            iface->data->iface.dflt_init) {
            xptr iface_data = (xptr)iface->data->iface.dflt_data;
            X_WRITE_UNLOCK (type_nodes);
            if (iface->data->iface.vtable_init_base)
                iface->data->iface.vtable_init_base (vtable);
            if (iface->data->iface.dflt_init)
                iface->data->iface.dflt_init (vtable, iface_data);
            X_WRITE_LOCK (type_nodes);
        }
    }
}

/* 虚函数表的基本初始化,后续是iface_vtable_iface_init_Wm() */
static xbool
iface_vtable_base_init_Wm (TypeNode *iface,
                           TypeNode *node)
{
    IFaceEntry  *entry;
    IFaceHolder *iholder;
    xIFace      *vtable = NULL;
    TypeNode    *pnode;

    iholder = iface_retrieve_iface_holder_Wm (iface,
                                              NODE_TYPE (node),
                                              TRUE);
    if (!iholder)
        return FALSE;
    x_assert (iholder && iholder->info);

    iface_ensure_dflt_vtable_Wm (iface);

    entry = class_peek_iface_entry_L (node, iface);

    x_assert (iface->data && entry && entry->vtable == NULL);

    entry->init_state = IFACE_INIT;

    /* 复制基类实现的接口表 */
    pnode = lookup_type_node_I (NODE_PARENT_TYPE (node));
    if (pnode) {
        IFaceEntry *pentry = class_peek_iface_entry_L (pnode, iface);
        if (pentry)
            vtable = x_memdup (pentry->vtable,
                               iface->data->iface.vtable_size);
    }
    if (!vtable)
        vtable = x_memdup (iface->data->iface.dflt_vtable,
                           iface->data->iface.vtable_size);
    entry->vtable = vtable;
    vtable->type = NODE_TYPE (iface);
    vtable->instance_type = NODE_TYPE (node);

    if (iface->data->iface.vtable_init_base){
        xptr iface_data = (xptr)iface->data->iface.dflt_data;
        X_WRITE_UNLOCK (type_nodes);
        iface->data->iface.vtable_init_base (vtable);
        X_WRITE_LOCK (type_nodes);
    }
    return TRUE;
}
/* 虚函数表的初始化 */
static void
iface_vtable_iface_init_Wm (TypeNode *iface,
                            TypeNode *node)
{
    IFaceEntry      *entry;
    IFaceHolder     *iholder;
    xIFace          *vtable;

    entry = class_peek_iface_entry_L (node, iface);
    iholder = iface_peek_holder_L (iface, NODE_TYPE (node));
    /* iholder->info 已在iface_vtable_base_init_Wm()确保赋值 */
    x_assert (iface->data && entry && iholder && iholder->info);
    x_assert (entry->init_state == IFACE_INIT);

    entry->init_state = INITIALIZED;

    /* 调用iface_init() */
    vtable = entry->vtable;
    if (iholder->info->iface_init) {
        xptr iface_data = (xptr)iholder->info->iface_data;
        X_WRITE_UNLOCK (type_nodes);
        if (iholder->info->iface_init)
            iholder->info->iface_init (vtable, iface_data);
        X_WRITE_LOCK (type_nodes);
    }
}
/* 虚函数表的清理 */
static xbool
iface_vtable_finalize_Wm(TypeNode       *iface,
                         TypeNode       *node,
                         xIFace         *vtable)
{
    IFaceEntry *entry = class_peek_iface_entry_L (node, iface);
    IFaceHolder *iholder;

    /* iface_retrieve_iface_holder_Wm() doesn't modify write lock for returning NULL */
    iholder = iface_retrieve_iface_holder_Wm (iface, NODE_TYPE (node), FALSE);
    if (!iholder)
        return FALSE;	/* we don't modify write lock upon FALSE */

    x_assert (entry && entry->vtable == vtable && iholder->info);

    entry->vtable = NULL;
    entry->init_state = UNINITIALIZED;
    if (iholder->info->iface_finalize ||
        iface->data->iface.vtable_finalize_base) {
        X_WRITE_UNLOCK (type_nodes);
        if (iholder->info->iface_finalize)
            iholder->info->iface_finalize (vtable,
                                           (xptr)iholder->info->iface_data);
        if (iface->data->iface.vtable_finalize_base)
            iface->data->iface.vtable_finalize_base (vtable);
        X_WRITE_LOCK (type_nodes);
    }
    vtable->type = 0;
    vtable->instance_type = 0;
    x_free (vtable);

    iface_blow_holder_Wm (iface, NODE_TYPE (node));

    return TRUE;	/* write lock modified */
}
/* 类结点中添加接口 */
static void
class_add_iface_Wm      (TypeNode       *node,
                         TypeNode       *iface,
                         const xIFaceInfo     *info,
                         xTypePlugin    *plugin)
{
    IFaceHolder     *iholder;
    IFaceEntry      *entry;
    xuint           i;

    iholder = x_malloc0 (sizeof(IFaceHolder));
    /* 插入IFaceHolder */
    iholder->next = iface_get_holders_L (iface);
    iface_set_holders_W (iface, iholder);
    iholder->instance_type = NODE_TYPE (node);
    iholder->info = info ? x_memdup (info, sizeof (*info)) : NULL;
    iholder->plugin = plugin;
    iface->children = x_renew (xType, iface->children, iface->n_children+1);
    iface->children[iface->n_children++] = NODE_TYPE (node);


    /* node中添加iface项 */
    class_add_iface_entry_W (node, NODE_TYPE (iface), NULL);
    /* 须要的初始化 */
    if (node->data) {
        InitState class_state = node->data->klass.init_state;
        if (class_state >= BASE_IFACE_INIT)
            iface_vtable_base_init_Wm (iface, node);

        if (class_state >= IFACE_INIT)
            iface_vtable_iface_init_Wm (iface, node);
    }

    /* 添加子结点的iface项 */
    entry = class_peek_iface_entry_L (node, iface);
    for (i = 0; i < node->n_children; i++) {
        TypeNode *cnode = lookup_type_node_I (node->children[i]);
        class_add_iface_entry_W (cnode, NODE_TYPE (iface), entry);
    }
}

static void
iface_add_prerequisite_W(TypeNode       *iface,
                         TypeNode       *prerequisite_node)
{
    xType prerequisite_type = NODE_TYPE (prerequisite_node);
    xType *prerequisites, *dependants;
    xuint n_dependants, i;

    x_assert (NODE_IS_IFACE (iface) &&
              IFACE_NODE_N_PREREQUISITES (iface) < MAX_N_PREREQUISITES &&
              (prerequisite_node->is_instantiatable || NODE_IS_IFACE (prerequisite_node)));

    prerequisites = IFACE_NODE_PREREQUISITES (iface);
    for (i = 0; i < IFACE_NODE_N_PREREQUISITES (iface); i++)
        if (prerequisites[i] == prerequisite_type)
            return;			/* we already have that prerequisiste */
        else if (prerequisites[i] > prerequisite_type)
            break;
    IFACE_NODE_N_PREREQUISITES (iface) += 1;
    IFACE_NODE_PREREQUISITES (iface) = x_renew (xType,
                                                IFACE_NODE_PREREQUISITES (iface),
                                                IFACE_NODE_N_PREREQUISITES (iface));
    prerequisites = IFACE_NODE_PREREQUISITES (iface);
    memmove (prerequisites + i + 1, prerequisites + i,
             sizeof (prerequisites[0]) * (IFACE_NODE_N_PREREQUISITES (iface) - i - 1));
    prerequisites[i] = prerequisite_type;

    /* we want to get notified when prerequisites get added to prerequisite_node */
    if (NODE_IS_IFACE (prerequisite_node))
    {
        dependants = iface_get_dependants_array_L (prerequisite_node);
        n_dependants = dependants ? dependants[0] : 0;
        n_dependants += 1;
        dependants = x_renew (xType, dependants, n_dependants + 1);
        dependants[n_dependants] = NODE_TYPE (iface);
        dependants[0] = n_dependants;
        iface_set_dependants_array_W (prerequisite_node, dependants);
    }

    /* we need to notify all dependants */
    dependants = iface_get_dependants_array_L (iface);
    n_dependants = dependants ? dependants[0] : 0;
    for (i = 1; i <= n_dependants; i++)
        iface_add_prerequisite_W (lookup_type_node_I (dependants[i]), prerequisite_node);
}

/* 检查类型名是否合法 */
static xbool
check_type_name_I       (xcstr          type_name)
{
    static const xchar extra_chars[] = "-_+";
    xcstr   p;
    xbool   name_valid;

    p = type_name;
    if (!p[0] || !p[1] || !p[2]) {
        x_warning ("type name `%s' is too short", p);
        return FALSE;
    }
    /* check the first letter */
    name_valid = (p[0] >= 'A' && p[0] <= 'Z')
        || (p[0] >= 'a' && p[0] <= 'z') || p[0] == '_';
    for (++p; *p; p++) {
        name_valid &= ((p[0] >= 'A' && p[0] <= 'Z') ||
                       (p[0] >= 'a' && p[0] <= 'z') ||
                       (p[0] >= '0' && p[0] <= '9') ||
                       strchr (extra_chars, p[0]));
    }
    if (!name_valid) {
        x_warning ("type name `%s' contains invalid characters", type_name);
        return FALSE;
    }
    if (x_type_from_name (type_name)) {
        x_warning ("cannot register existing type `%s'", type_name);
        return FALSE;
    }
    return TRUE;
}
/* 检查继承是否合法 */
static xbool
check_derivation_I      (xType          parent_type,
                         xcstr          type_name)
{
    TypeNode            *pnode;
    xFundamentalInfo    *finfo;

    pnode = lookup_type_node_I (parent_type);
    if (!pnode) {
        x_warning ("cannot derive type `%s' from invalid parent type `%s'",
                   type_name,
                   type_name_I (parent_type));
        return FALSE;
    }
    finfo = type_fundamental_info_I (pnode);
    /* 不允许继承 */
    if (!(finfo->type_flags & X_TYPE_DERIVABLE)) {
        x_warning ("cannot derive `%s' from non-derivable parent type `%s'",
                   type_name, NODE_NAME (pnode));
        return FALSE;
    }
    /* 不允许深度继承 */
    if (parent_type != NODE_FUNDAMENTAL_TYPE (pnode) &&
        !(finfo->type_flags & X_TYPE_DEEP_DERIVABLE)) {
        x_warning ("cannot derive `%s' from non-fundamental parent type `%s'",
                   type_name, NODE_NAME (pnode));
        return FALSE;
    }
    return TRUE;
}
/* 检查插件是否合法 */
static xbool
check_plugin_U          (xTypePlugin    *plugin,
                         xbool          need_type_info,
                         xbool          need_iface_info,
                         xcstr          type_name)
{
    if (!plugin) {
        x_warning ("plugin handle for type `%s' is NULL",
                   type_name);
        return FALSE;
    }
    if (!X_IS_TYPE_PLUGIN (plugin)) {
        x_warning ("plugin pointer (%p) for type `%s' is invalid",
                   plugin, type_name);
        return FALSE;
    }
    if (need_type_info
        && !X_TYPE_PLUGIN_GET_CLASS (plugin)->type_info) {
        x_warning ("plugin for type `%s' has no type_info() implementation",
                   type_name);
        return FALSE;
    }
    if (need_iface_info
        && !X_TYPE_PLUGIN_GET_CLASS (plugin)->iface_info) {
        x_warning ("plugin for type `%s' has no iface_info() implementation",
                   type_name);
        return FALSE;
    }
    return TRUE;
}
/* 创建新结点 */
static TypeNode*
type_node_any_new_W     (TypeNode       *pnode,
                         xType          ftype,
                         xcstr          name,
                         xTypePlugin    *plugin,
                         xFundamentalFlags type_flags)
{
    xuint n_supers;
    xType type;
    TypeNode *node;
    xuint i, node_size = 0;

    n_supers = pnode ? pnode->n_supers + 1 : 0;

    if (!pnode)
        node_size += SIZEOF_FUNDAMENTAL_INFO;/* fundamental type info */
    node_size += SIZEOF_BASE_TYPE_NODE; /* TypeNode structure */
    /* self + ancestors + (0) for ->supers[] */
    node_size += (sizeof (xType) * (1 + n_supers + 1));
    node = x_malloc0 (node_size);

    if (!pnode){/* offset fundamental types */
        node = X_STRUCT_MEMBER_P (node, SIZEOF_FUNDAMENTAL_INFO);
        _fundamental_nodes[ftype >> X_TYPE_FUNDAMENTAL_SHIFT] = node;
        type = ftype;
    }
    else
        type = (xType) node;

    x_assert ((type & TYPE_ID_MASK) == 0);

    node->n_supers = n_supers;
    if (!pnode) {
        node->supers[0] = type;
        node->supers[1] = 0;

        node->is_classed = (type_flags & X_TYPE_CLASSED) != 0;
        node->is_instantiatable = (type_flags & X_TYPE_INSTANTIATABLE) != 0;

        if (NODE_IS_IFACE (node)) {
            IFACE_NODE_N_PREREQUISITES (node) = 0;
            IFACE_NODE_PREREQUISITES (node) = NULL;
        }
        else
            x_atomic_array_init (CLASS_NODE_IFACE_ENTRIES (node));
    }
    else {
        node->supers[0] = type;
        memcpy (node->supers + 1, pnode->supers,
                sizeof (xType) * (1 + pnode->n_supers + 1));

        node->is_classed = pnode->is_classed;
        node->is_instantiatable = pnode->is_instantiatable;

        if (NODE_IS_IFACE (node)) {
            IFACE_NODE_N_PREREQUISITES (node) = 0;
            IFACE_NODE_PREREQUISITES (node) = NULL;
        }
        else {
            xuint j;
            IFaceEntries *entries;

            entries = x_atomic_array_copy (CLASS_NODE_IFACE_ENTRIES (pnode),
                                           IFACE_ENTRIES_HEADER_SIZE,
                                           0);
            if (entries) {
                for (j = 0; j < IFACE_ENTRIES_N_ENTRIES (entries); j++) {
                    entries->entry[j].vtable = NULL;
                    entries->entry[j].init_state = UNINITIALIZED;
                }
                x_atomic_array_update (CLASS_NODE_IFACE_ENTRIES (node),
                                       entries);
            }
        }

        i = pnode->n_children++;
        pnode->children = x_renew (xType, pnode->children, pnode->n_children);
        pnode->children[i] = type;
    }

    TRACE(X_CLASS_TYPE_NEW(name, NODE_PARENT_TYPE(node), type));

    node->plugin = plugin;
    node->n_children = 0;
    node->children = NULL;
    node->data = NULL;
    node->name = x_intern_str(name);
    node->qdata_list = NULL;
    x_hash_table_insert (_type_nodes_ht,
                         (xptr)node->name,
                         X_SIZE_TO_PTR (type));
    return node;
}
/* 创建非基类型的新结点 */
static TypeNode*
type_node_new_W         (TypeNode       *pnode,
                         xcstr          name,
                         xTypePlugin    *plugin)

{
    x_assert (pnode);
    x_assert (pnode->n_supers < MAX_N_SUPERS);
    x_assert (pnode->n_children < MAX_N_CHILDREN);

    return type_node_any_new_W (pnode,
                                NODE_FUNDAMENTAL_TYPE (pnode), name, plugin, 0);
}
static TypeNode*
type_node_fundamental_new_W (xType      ftype,
                             xcstr      name,
                             xFundamentalFlags type_flags)
{
    xFundamentalInfo *finfo;
    TypeNode *node;

    x_assert ((ftype & TYPE_ID_MASK) == 0);
    x_assert (ftype <= X_TYPE_FUNDAMENTAL_MAX);

    if (ftype >> X_TYPE_FUNDAMENTAL_SHIFT == _fundamental_next)
        _fundamental_next++;

    type_flags &= TYPE_FUNDAMENTAL_FLAG_MASK;

    node = type_node_any_new_W (NULL, ftype, name, NULL, type_flags);

    finfo = type_fundamental_info_I (node);
    finfo->type_flags = type_flags;

    return node;
}

/* 添加类型标志 */
static void
type_add_flags_W        (TypeNode       *node,
                         xTypeFlags     flags)
{
    xuint dflags;

    x_return_if_fail ((flags & ~TYPE_FLAG_MASK) == 0);
    x_return_if_fail (node != NULL);

    if ((flags & TYPE_FLAG_MASK) && node->is_classed && node->data && node->data->klass.klass)
        x_warning ("tagging type `%s' as abstract "
                   "after class initialization", NODE_NAME (node));
    dflags = X_PTR_TO_SIZE (type_get_qdata_L (node, _quark_type_flags));
    dflags |= flags;
    type_set_qdata_W (node, _quark_type_flags, X_SIZE_TO_PTR (dflags));
}

static inline xbool
type_data_ref_U (TypeNode *node)
{
    xuint current;

    do {
        current = NODE_REFCOUNT (node);

        if (current < 1)
            return FALSE;
    } while (!x_atomic_int_cas ((int *) &node->ref_count,
                                current, current + 1));

    return TRUE;
}
/* 创建class实例 */
static void
type_class_init_Wm      (TypeNode       *node,
                         xClass         *pclass)
{
    xSList *slist,  *init_slist = NULL;
    xClass          *klass;
    IFaceEntries    *entries;
    IFaceEntry      *entry;
    TypeNode        *bnode, *pnode;
    xuint           i;

    /* Accessing data->klass will work for instantiable types
     * too because ClassData is a subset of InstanceData
     */
    x_assert (node->is_classed && node->data &&
              node->data->klass.class_size &&
              !node->data->klass.klass &&
              node->data->klass.init_state == UNINITIALIZED);
    if (node->data->klass.class_private_size)
        klass = x_malloc0 (ALIGN_STRUCT (node->data->klass.class_size) + node->data->klass.class_private_size);
    else
        klass = x_malloc0 (node->data->klass.class_size);
    node->data->klass.klass = klass;
    x_atomic_int_set (&node->data->klass.init_state, BASE_CLASS_INIT);

    if (pclass) {
        TypeNode *pnode = lookup_type_node_I (pclass->type);
        /* 复制父类数据 */
        memcpy (klass, pclass, pnode->data->klass.class_size);
        memcpy (X_STRUCT_MEMBER_P (klass, ALIGN_STRUCT (node->data->klass.class_size)),
                X_STRUCT_MEMBER_P (pclass, ALIGN_STRUCT (pnode->data->klass.class_size)),
                pnode->data->klass.class_private_size);

        if (node->is_instantiatable) {
            /* We need to initialize the private_size here rather than in
             * type_data_make_W() since the class init for the parent
             * class may have changed pnode->data->instance.private_size.
             */
            node->data->instance.private_size = pnode->data->instance.private_size;
        }
    }
    klass->type = NODE_TYPE (node);

    X_WRITE_UNLOCK (type_nodes);

    /* stack all base class initialization functions, so we
     * call them in ascending order.
     */
    for (bnode = node; bnode;
         bnode = lookup_type_node_I (NODE_PARENT_TYPE (bnode)))
        if (bnode->data->klass.class_init_base)
            init_slist = x_slist_prepend (init_slist,(xptr) bnode->data);
    for (slist = init_slist; slist; slist = slist->next) {
        TypeData * data = (TypeData *) slist->data;
        data->klass.class_init_base (klass);
    }
    x_slist_free (init_slist);

    X_WRITE_LOCK (type_nodes);

    x_atomic_int_set (&node->data->klass.init_state, BASE_IFACE_INIT);

    /* Before we initialize the class, base initialize all interfaces, either
     * from parent, or through our holder info
     */
    pnode = lookup_type_node_I (NODE_PARENT_TYPE (node));

    i = 0;
    while ((entries = CLASS_NODE_IFACE_ENTRIES_LOCKED (node)) != NULL &&
           i < IFACE_ENTRIES_N_ENTRIES (entries)) {
        entry = &entries->entry[i];
        while (i < IFACE_ENTRIES_N_ENTRIES (entries) &&
               entry->init_state == IFACE_INIT) {
            entry++;
            i++;
        }

        if (i == IFACE_ENTRIES_N_ENTRIES (entries))
            break;

        if (!iface_vtable_base_init_Wm (lookup_type_node_I (entry->iface_type),
                                        node)) {
            xuint j;
            IFaceEntries *pentries = CLASS_NODE_IFACE_ENTRIES_LOCKED (pnode);

            /* need to get this interface from parent, type_iface_vtable_base_init_Wm()
             * doesn't modify write lock upon FALSE, so entry is still valid; 
             */
            x_assert (pnode != NULL);

            if (pentries)
                for (j = 0; j < IFACE_ENTRIES_N_ENTRIES (pentries); j++) {
                    IFaceEntry *pentry = &pentries->entry[j];

                    if (pentry->iface_type == entry->iface_type)
                    {
                        entry->vtable = pentry->vtable;
                        entry->init_state = INITIALIZED;
                        break;
                    }
                }
            x_assert (entry->vtable != NULL);
        }

        /* If the write lock was released, additional interface entries might
         * have been inserted into CLASSED_NODE_IFACES_ENTRIES (node); they'll
         * be base-initialized when inserted, so we don't have to worry that
         * we might miss them. Uninitialized entries can only be moved higher
         * when new ones are inserted.
         */
        i++;
    }

    x_atomic_int_set (&node->data->klass.init_state, CLASS_INIT);

    X_WRITE_UNLOCK (type_nodes);

    if (node->data->klass.class_init)
        node->data->klass.class_init (klass, (xptr) node->data->klass.class_data);

    X_WRITE_LOCK (type_nodes);

    x_atomic_int_set (&node->data->klass.init_state, IFACE_INIT);

    /* finish initializing the interfaces through our holder info.
     * inherited interfaces are already init_state == INITIALIZED, because
     * they either got setup in the above base_init loop, or during
     * class_init from within type_add_interface_Wm() for this or
     * an anchestor type.
     */
    i = 0;
    while ((entries = CLASS_NODE_IFACE_ENTRIES_LOCKED (node)) != NULL) {
        entry = &entries->entry[i];
        while (i < IFACE_ENTRIES_N_ENTRIES (entries) &&
               entry->init_state == INITIALIZED) {
            entry++;
            i++;
        }

        if (i == IFACE_ENTRIES_N_ENTRIES (entries))
            break;

        iface_vtable_iface_init_Wm (lookup_type_node_I (entry->iface_type), node);

        /* As in the loop above, additional initialized entries might be inserted
         * if the write lock is released, but that's harmless because the entries
         * we need to initialize only move higher in the list.
         */
        i++;
    }

    x_atomic_int_set (&node->data->klass.init_state, INITIALIZED);
}
/* 清理TypeData中类结点的接口项表 */
static void
type_data_finalize_class_ifaces_Wm (TypeNode *node)
{
    xuint           i;
    IFaceEntries    *entries;

    x_assert (node->is_instantiatable && node->data &&
              node->data->klass.klass && NODE_REFCOUNT (node) == 0);

reiterate:
    entries = CLASS_NODE_IFACE_ENTRIES_LOCKED (node);
    for (i = 0; entries != NULL && i < IFACE_ENTRIES_N_ENTRIES (entries); i++)
    {
        IFaceEntry *entry = &entries->entry[i];
        if (entry->vtable) {
            if (iface_vtable_finalize_Wm (lookup_type_node_I (entry->iface_type), node, entry->vtable)) {
                /* refetch entries, IFACES_ENTRIES might be modified */
                goto reiterate;
            }
            else {
                /* iface_vtable_finalize_Wm() doesn't modify write lock upon FALSE,
                 * iface vtable came from parent
                 */
                entry->vtable = NULL;
                entry->init_state = UNINITIALIZED;
            }
        }
    }
}
/* 清理TypeData中类结点 */
static void
type_data_finalize_class_U (TypeNode  *node,
                            ClassData *cdata)
{
    xClass      *klass = cdata->klass;
    TypeNode    *bnode;

    x_assert (cdata->klass && NODE_REFCOUNT (node) == 0);

    if (cdata->class_finalize)
        cdata->class_finalize (klass, (xptr) cdata->class_data);

    /* call all base class destruction functions in descending order
    */
    if (cdata->class_finalize_base)
        cdata->class_finalize_base (klass);
    for (bnode = lookup_type_node_I (NODE_PARENT_TYPE (node));
         bnode; bnode = lookup_type_node_I (NODE_PARENT_TYPE (bnode)))
        if (bnode->data->klass.class_finalize_base)
            bnode->data->klass.class_finalize_base (klass);

    x_free (cdata->klass);
}

/* TypeData的引用数为0时,销毁之,只支持动态类型 */
static void
type_data_last_unref_Wm (TypeNode       *node,
                         xbool          uncached)
{
    x_return_if_fail (node != NULL && node->plugin != NULL);

    if (!node->data || NODE_REFCOUNT (node) == 0) {
        x_warning ("cannot drop last reference to unreferenced type `%s'",
                   NODE_NAME (node));
        return;
    }

    /* may have been re-referenced meanwhile */
    if (!x_atomic_int_dec ((int *) &node->ref_count)) {
        xType ptype = NODE_PARENT_TYPE (node);
        TypeData *tdata;

        tdata = node->data;
        if (node->is_classed && tdata->klass.klass) {
            if (CLASS_NODE_IFACE_ENTRIES_LOCKED (node) != NULL)
                type_data_finalize_class_ifaces_Wm (node);
            node->mutatable_check_cache = FALSE;
            node->data = NULL;
            X_WRITE_UNLOCK (type_nodes);
            type_data_finalize_class_U (node, &tdata->klass);
            X_WRITE_LOCK (type_nodes);
        }
        else if (NODE_IS_IFACE (node) && tdata->iface.dflt_vtable) {
            node->mutatable_check_cache = FALSE;
            node->data = NULL;
            if (tdata->iface.dflt_finalize || tdata->iface.vtable_finalize_base) {
                X_WRITE_UNLOCK (type_nodes);
                if (tdata->iface.dflt_finalize)
                    tdata->iface.dflt_finalize (tdata->iface.dflt_vtable,
                                                (xptr) tdata->iface.dflt_data);
                if (tdata->iface.vtable_finalize_base)
                    tdata->iface.vtable_finalize_base (tdata->iface.dflt_vtable);
                X_WRITE_LOCK (type_nodes);
            }
            x_free (tdata->iface.dflt_vtable);
        }
        else {
            node->mutatable_check_cache = FALSE;
            node->data = NULL;
        }

        x_free (tdata);

        X_WRITE_UNLOCK (type_nodes);
        x_type_plugin_unuse (node->plugin);
        if (ptype)
            type_data_unref_U (lookup_type_node_I (ptype), FALSE);
        X_WRITE_LOCK (type_nodes);
    }
}
/* 减少TypeData的引用 */
static inline void
type_data_unref_U       (TypeNode       *node,
                         xbool          uncached)
{
    xuint current;

    do {
        current = NODE_REFCOUNT (node);

        if (current <= 1) {
            if (!node->plugin) {
                x_warning ("static type `%s' unreferenced too often",
                           NODE_NAME (node));
                return;
            }

            x_assert (current > 0);

            X_REC_LOCK (class_init); /* required locking order: 1) class_init_rec_mutex, 2) type_rw_lock */
            X_WRITE_LOCK (type_nodes);
            type_data_last_unref_Wm (node, uncached);
            X_WRITE_UNLOCK (type_nodes);
            X_REC_UNLOCK (class_init);
            return;
        }
    } while (!x_atomic_int_cas ((int *) &node->ref_count,
                                current, current - 1));
}
/* 假设类已存在,计算总大小 */
static inline xsize
total_instance_size_I   (TypeNode       *node)
{
    xsize total_instance_size;

    total_instance_size = node->data->instance.instance_size;
    if (node->data->instance.private_size != 0)
        total_instance_size = ALIGN_STRUCT (total_instance_size) + node->data->instance.private_size;

    return total_instance_size;
}
static xbool
check_is_value_type_U   (xType          type)
{
    xTypeFlags tflags = X_TYPE_VALUE_ABSTRACT;
    TypeNode *node;

    /* common path speed up */
    node = lookup_type_node_I (type);
    if (node && node->mutatable_check_cache)
        return TRUE;

    X_READ_LOCK (type_nodes);
restart_check:
    if (node) {
        if (node->data && NODE_REFCOUNT (node) > 0 &&
            node->data->common.value_table->value_init)
            tflags = X_PTR_TO_SIZE (type_get_qdata_L (node, _quark_type_flags));
        else if (NODE_IS_IFACE (node)) {
            xuint i;

            for (i = 0; i < IFACE_NODE_N_PREREQUISITES (node); i++) {
                xType prtype = IFACE_NODE_PREREQUISITES (node)[i];
                TypeNode *prnode = lookup_type_node_I (prtype);

                if (prnode->is_instantiatable) {
                    type = prtype;
                    node = lookup_type_node_I (type);
                    goto restart_check;
                }
            }
        }
    }
    X_READ_UNLOCK (type_nodes);

    return !(tflags & X_TYPE_VALUE_ABSTRACT);
}

xType
x_type_register_static  (xcstr          type_name,
                         xType          parent_type,
                         const xTypeInfo      *info,
                         xTypeFlags     flags)
{
    TypeNode *pnode, *node;
    xType type = 0;

    type_init();

    x_return_val_if_fail (type_name != NULL, 0);
    x_return_val_if_fail (parent_type > 0, 0);
    x_return_val_if_fail (info != NULL, 0);

    if(info->class_finalize) {
        x_warning ("class finalizer specified for static type `%s'",
                   type_name);
        return 0;
    }

    if (!check_type_name_I (type_name) ||
        !check_derivation_I (parent_type, type_name))
        return 0;

    X_WRITE_LOCK (type_nodes);
    pnode = lookup_type_node_I (parent_type);
    type_data_ref_Wm (pnode);
    if (check_type_info_I (pnode, NODE_FUNDAMENTAL_TYPE (pnode),
                           type_name, info)){
        node = type_node_new_W (pnode, type_name, NULL);
        type_add_flags_W (node, flags);
        type_data_make_W (node, info,
                          check_value_table_I (type_name, info->value_table)
                          ? info->value_table : NULL);
        type = NODE_TYPE (node);
    }
    X_WRITE_UNLOCK (type_nodes);

    return type;
}

xType
x_type_register_dynamic (xcstr          type_name,
                         xType          parent_type,
                         xTypePlugin    *plugin,
                         xTypeFlags     flags)
{
    TypeNode *pnode, *node;
    xType type = 0;

    type_init();

    x_return_val_if_fail (type_name != NULL, 0);
    x_return_val_if_fail (parent_type > 0, 0);
    x_return_val_if_fail (plugin != NULL, 0);

    if (!check_type_name_I (type_name) ||
        !check_derivation_I (parent_type, type_name) ||
        !check_plugin_U (plugin, TRUE, FALSE, type_name))
        return 0;

    X_WRITE_LOCK (type_nodes);
    pnode = lookup_type_node_I (parent_type);
    node = type_node_new_W (pnode, type_name, plugin);
    type_add_flags_W (node, flags);
    type = NODE_TYPE (node);
    X_WRITE_UNLOCK (type_nodes);

    return type;
}

xType
x_type_register_fundamental(xType           type_id,
                            xcstr           type_name,
                            const xTypeInfo       *info,
                            const xFundamentalInfo*finfo,
                            xTypeFlags       flags)
{
    TypeNode *node;

    type_init();

    x_return_val_if_fail (type_id > 0, 0);
    x_return_val_if_fail (type_name != NULL, 0);
    x_return_val_if_fail (info != NULL, 0);
    x_return_val_if_fail (finfo != NULL, 0);

    if (!check_type_name_I (type_name))
        return 0;
    if ((type_id & TYPE_ID_MASK) ||
        type_id > X_TYPE_FUNDAMENTAL_MAX) {
        x_warning ("attempt to register fundamental type `%s' "
                   "with invalid type id (%" X_SIZE_FORMAT ")",
                   type_name, type_id);
        return 0;
    }
    if ((finfo->type_flags & X_TYPE_INSTANTIATABLE) &&
        !(finfo->type_flags & X_TYPE_CLASSED)) {
        x_warning ("cannot register instantiatable "
                   "fundamental type `%s' as non-classed",
                   type_name);
        return 0;
    }
    if (lookup_type_node_I (type_id)) {
        x_warning ("cannot register existing fundamental type `%s' (as `%s')",
                   type_name_I (type_id),
                   type_name);
        return 0;
    }

    X_WRITE_LOCK (type_nodes);
    node = type_node_fundamental_new_W (type_id, type_name, finfo->type_flags);
    type_add_flags_W (node, flags);

    if (check_type_info_I (NULL,
                           NODE_FUNDAMENTAL_TYPE (node), type_name, info)){
        type_data_make_W (node, info,
                          check_value_table_I (type_name, info->value_table)
                          ? info->value_table : NULL);
    }
    X_WRITE_UNLOCK (type_nodes);

    return NODE_TYPE (node);
}

void
x_type_add_iface_static (xType          instance_type,
                         xType          iface_type,
                         const xIFaceInfo     *info)
{
    x_return_if_fail (info);
    x_return_if_fail (X_TYPE_IS_INSTANTIATABLE (instance_type));
    x_return_if_fail (x_type_parent (iface_type) == X_TYPE_IFACE);

    X_REC_LOCK (class_init);
    X_WRITE_LOCK (type_nodes);
    if (check_class_add_iface_L (instance_type, iface_type)) {
        TypeNode *node = lookup_type_node_I (instance_type);
        TypeNode *iface = lookup_type_node_I (iface_type);
        if (check_iface_info_I (iface, NODE_TYPE (node), info))
            class_add_iface_Wm (node, iface, info, NULL);
    }
    X_WRITE_UNLOCK (type_nodes);
    X_REC_UNLOCK(class_init);
}

void
x_type_add_iface_dynamic(xType          instance_type,
                         xType          iface_type,
                         xTypePlugin    *plugin)
{
    TypeNode    *node;

    x_return_if_fail (plugin);
    x_return_if_fail (X_TYPE_IS_INSTANTIATABLE (instance_type));
    x_return_if_fail (x_type_parent (iface_type) == X_TYPE_IFACE);

    node = lookup_type_node_I (instance_type);

    if (!check_plugin_U (plugin, FALSE, TRUE, NODE_NAME (node)))
        return;

    X_REC_LOCK (class_init);
    X_WRITE_LOCK (type_nodes);
    if (check_class_add_iface_L (instance_type, iface_type)) {
        TypeNode *iface = lookup_type_node_I (iface_type);
        class_add_iface_Wm (node, iface, NULL, plugin);
    }
    X_WRITE_UNLOCK (type_nodes);
    X_REC_UNLOCK(class_init);
}

xcstr
x_type_name             (xType          type)
{
    TypeNode *node;

    x_return_val_if_uninitialized (NULL);

    node = lookup_type_node_I (type);

    return node ? NODE_NAME (node) : NULL;
}
static xType
find_mime_type_L        (xType          type,
                         xcstr          mime)
{
    TypeNode *node;
    xuint flags;
    xType ret;
    xint i;

    node = lookup_type_node_I (type);
    for (i=node->n_children-1; i>=0; --i) {
        ret = find_mime_type_L (node->children[i], mime);
        if (ret != X_TYPE_INVALID) 
            return ret;
    }
    flags=X_PTR_TO_SIZE(type_get_qdata_L (node, _quark_type_flags));
    if ((flags & X_TYPE_ABSTRACT) == 0) {
        xcstr type_mime = type_get_qdata_L (node, _quark_type_mime);
        if (!type_mime)
            type_mime = node->name;
        if (x_stristr (type_mime, mime))
            return type;
    }
    return X_TYPE_INVALID;
}
xType
x_type_from_mime        (xType          type,
                         xcstr          mime)
{
    xType xtype;

    x_return_val_if_uninitialized (X_TYPE_INVALID);

    X_READ_LOCK (type_nodes);
    xtype = find_mime_type_L (type, mime);
    X_READ_UNLOCK (type_nodes);

    return xtype;
}
xTypePlugin*
x_type_plugin           (xType          type)
{
    TypeNode    *node;

    x_return_val_if_uninitialized (NULL);

    node = lookup_type_node_I(type);

    return node ? node->plugin : NULL;
}

xTypePlugin*
x_type_iface_plugin     (xType          type,
                         xType          iface_type)
{
    TypeNode *node;
    TypeNode *iface;
    IFaceHolder *iholder;
    xTypePlugin *plugin;

    x_return_val_if_fail (X_TYPE_IS_IFACE (iface_type), NULL);

    node = lookup_type_node_I (type);  
    iface = lookup_type_node_I (iface_type);

    x_return_val_if_fail (node != NULL, NULL);
    x_return_val_if_fail (iface != NULL, NULL);

    X_READ_LOCK (type_nodes);

    iholder = iface_get_holders_L (iface);
    while (iholder && iholder->instance_type != type)
        iholder = iholder->next;
    plugin = iholder ? iholder->plugin : NULL;

    X_READ_UNLOCK (type_nodes);

    return plugin;
}

xType
x_type_from_name        (xcstr          name)
{
    xType   type;
    xQuark  quark;

    x_return_val_if_uninitialized (0);
    x_return_val_if_fail (name != NULL, 0);

    quark = x_quark_try (name);
    if (quark == 0)
        return  X_TYPE_INVALID;

    X_READ_LOCK (type_nodes);
    type = (xType)x_hash_table_lookup (_type_nodes_ht, X_SIZE_TO_PTR (quark));
    X_READ_UNLOCK (type_nodes);

    return type;
}

xType
x_type_fundamental      (xType          type)
{
    TypeNode    *node;

    x_return_val_if_uninitialized (0);

    node = lookup_type_node_I(type);

    return node ? NODE_FUNDAMENTAL_TYPE (node) : 0;
}

xType
x_type_parent           (xType          type)
{
    TypeNode    *node;

    x_return_val_if_uninitialized (0);

    node = lookup_type_node_I(type);

    return node ? NODE_PARENT_TYPE (node) : 0;
}

xsize
x_type_depth            (xType          type)
{
    TypeNode *node;

    node = lookup_type_node_I (type);

    return node ? node->n_supers + 1 : 0;
}

xType*
x_type_children         (xType          type,
                         xsize          *n_children)
{
    xType       *children;
    TypeNode    *node;

    x_return_val_if_uninitialized (NULL);

    node = lookup_type_node_I(type);
    if (!node) {
        if (n_children)
            *n_children = 0;
        return NULL;
    }

    X_READ_LOCK (type_nodes);
    children = x_new (xType, node->n_children + 1);
    memcpy (children, node->children, sizeof (xType) * node->n_children);
    children[node->n_children] = 0;
    if (n_children)
        *n_children = node->n_children;
    X_READ_UNLOCK (type_nodes);

    return children;
}

xType*
x_type_ifaces           (xType          type,
                         xsize          *n_ifaces)
{
    IFaceEntries    *entries;
    xType           *ifaces;
    xuint           i;
    TypeNode        *node;

    x_return_val_if_uninitialized (NULL);

    node = lookup_type_node_I (type);
    if (!node || !node->is_instantiatable){
        if (n_ifaces)
            *n_ifaces = 0;
        return NULL;
    }

    X_READ_LOCK (type_nodes);
    entries = CLASS_NODE_IFACE_ENTRIES_LOCKED (node);
    if (entries) {
        ifaces = x_new (xType, IFACE_ENTRIES_N_ENTRIES (entries) + 1);
        for (i = 0; i < IFACE_ENTRIES_N_ENTRIES (entries); i++)
            ifaces[i] = entries->entry[i].iface_type;
    }
    else {
        ifaces = x_new (xType, 1);
        i = 0;
    }
    ifaces[i] = 0;

    if (n_ifaces)
        *n_ifaces = i;
    X_READ_UNLOCK (type_nodes);
    return ifaces;
}

void
x_type_set_private      (xType          type,
                         xsize          private_size)
{
    TypeNode *node = lookup_type_node_I (type);
    xsize offset;

    x_return_if_fail (private_size > 0);
    x_return_if_fail (private_size < 0xffff);

    if (!node || !node->is_classed || !node->data) {
        x_warning ("cannot add class private field to invalid type '%s'",
                   type_name_I (type));
        return;
    }

    if (NODE_PARENT_TYPE (node)) {
        TypeNode *pnode = lookup_type_node_I (NODE_PARENT_TYPE (node));
        if (node->data->klass.class_private_size
            != pnode->data->klass.class_private_size) {
            x_warning ("x_type_add_private() called multiple times for the same type");
            return;
        }
    }

    X_WRITE_LOCK (type_nodes);

    offset = ALIGN_STRUCT (node->data->klass.class_private_size);
    node->data->klass.class_private_size = offset + private_size;

    X_WRITE_UNLOCK (type_nodes);
}
void
x_type_set_mime         (xType          type,
                         xcstr          mime)
{
    TypeNode *node;

    X_WRITE_LOCK (type_nodes);
    node = lookup_type_node_I (type);
    type_set_qdata_W (node, _quark_type_mime, (xptr)mime);
    X_WRITE_UNLOCK (type_nodes);
}

xbool
x_type_is               (xType          type,
                         xType          ancestor)
{
    TypeNode    *node;
    TypeNode    *iface_node;
    xbool       is_a;

    node = lookup_type_node_I (type);
    iface_node = lookup_type_node_I (ancestor);
    is_a = node && iface_node && type_conforms_to_U (node, iface_node, TRUE, TRUE);

    return is_a;
}

xbool
x_class_is              (const xClass   *klass,
                         xType          type)
{
    TypeNode    *node, *iface;
    xbool       check;

    x_return_val_if_uninitialized (FALSE);

    if (!klass)
        return FALSE;

    node = lookup_type_node_I (X_CLASS_TYPE (klass));
    iface = lookup_type_node_I (type);
    check = node && node->is_classed && iface && type_conforms_to_U (node, iface, FALSE, FALSE);

    return check;
}

xClass*
x_class_cast            (const xClass   *klass,
                         xType          type)
{
    x_return_val_if_uninitialized (NULL);

    if (klass) {
        TypeNode *node, *iface;
        xbool is_classed, check;

        node = lookup_type_node_I (klass->type);
        is_classed = node && node->is_classed;
        iface = lookup_type_node_I (type);
        check = is_classed && iface &&
            type_conforms_to_U (node, iface, FALSE, FALSE);
        if (check)
            return (xClass*)klass;

        if (is_classed)
            x_warning ("invalid class cast from `%s' to `%s'",
                       type_name_I (klass->type),
                       type_name_I (type));
        else
            x_warning ("invalid unclassed type `%s' in class cast to `%s'",
                       type_name_I (klass->type),
                       type_name_I (type));
    }
    else
        x_warning ("invalid class cast from (NULL) pointer to `%s'",
                   type_name_I (type));
    return NULL;
}

xptr
x_class_peek            (xType          type)
{
    TypeNode    *node;
    xptr        klass;

    x_return_val_if_uninitialized (NULL);

    node = lookup_type_node_I (type);
    if (node && node->is_classed && NODE_REFCOUNT (node) &&
        x_atomic_int_get (&node->data->klass.init_state) == INITIALIZED)
        /* ref_count _may_ be 0 */
        klass = node->data->klass.klass;
    else
        klass = NULL;

    return klass;
}

xptr
x_class_peek_parent     (xcptr          klass)
{
    TypeNode *node;
    xptr pclass = NULL;

    x_return_val_if_uninitialized (NULL);
    x_return_val_if_fail (klass != NULL, NULL);

    node = lookup_type_node_I (X_CLASS_TYPE (klass));
    if (node && node->is_classed && node->data && NODE_PARENT_TYPE (node)) {
        node = lookup_type_node_I (NODE_PARENT_TYPE (node));
        pclass = node->data->klass.klass;
    }
    else if (NODE_PARENT_TYPE (node))
        x_warning ("%s: invalid class pointer `%p'", X_STRLOC, klass);

    return pclass;
}

xptr
x_iface_peek            (xcptr          instance_class,
                         xType          iface_type)
{
    TypeNode    *node;
    TypeNode    *iface;
    xptr        vtable = NULL;
    xClass      *klass = (xClass*)instance_class;

    x_return_val_if_uninitialized (NULL);
    x_return_val_if_fail (instance_class != NULL, NULL);

    node = lookup_type_node_I (klass->type);
    iface = lookup_type_node_I (iface_type);
    if (node && node->is_instantiatable && iface)
        class_peek_vtable_I (node, iface, &vtable);
    else
        x_warning ("%s: invalid class pointer `%p'", X_STRLOC, klass);

    return vtable;
}
void
x_iface_add_prerequisite(xType          iface_type,
                         xType          prerequisite_type)
{
    TypeNode *iface, *prerequisite_node;
    IFaceHolder *holders;

    x_return_if_fail (X_TYPE_IS_IFACE (iface_type));
    x_return_if_fail (!x_type_is (iface_type, prerequisite_type));
    x_return_if_fail (!x_type_is (prerequisite_type, iface_type));

    iface = lookup_type_node_I (iface_type);
    prerequisite_node = lookup_type_node_I (prerequisite_type);
    if (!iface || !prerequisite_node || !NODE_IS_IFACE (iface)) {
        x_warning ("interface type `%s' or prerequisite type `%s' invalid",
                   type_name_I (iface_type),
                   type_name_I (prerequisite_type));
        return;
    }
    X_WRITE_LOCK (type_nodes);
    holders = iface_get_holders_L (iface);
    if (holders) {
        X_WRITE_UNLOCK (type_nodes);
        x_warning ("unable to add prerequisite `%s' to interface `%s' "
                   "which is already in use for `%s'",
                   type_name_I (prerequisite_type),
                   type_name_I (iface_type),
                   type_name_I (holders->instance_type));
        return;
    }
    if (prerequisite_node->is_instantiatable) {
        xuint i;

        /* can have at most one publicly installable 
         * instantiatable prerequisite
         */
        for (i = 0; i < IFACE_NODE_N_PREREQUISITES (iface); i++) {
            xType prtype = IFACE_NODE_PREREQUISITES (iface)[i];
            TypeNode *prnode = lookup_type_node_I (prtype);

            if (prnode->is_instantiatable) {
                X_WRITE_UNLOCK (type_nodes);
                x_warning ("adding prerequisite `%s' to interface `%s' "
                           "conflicts with existing prerequisite `%s'",
                           type_name_I (prerequisite_type),
                           type_name_I (iface_type),
                           type_name_I (NODE_TYPE (prnode)));
                return;
            }
        }

        for (i = 0; i < prerequisite_node->n_supers + 1; i++) {
            TypeNode *pnode = lookup_type_node_I (prerequisite_node->supers[i]);
            iface_add_prerequisite_W (iface,pnode);
        }
        X_WRITE_UNLOCK (type_nodes);
    }
    else if (NODE_IS_IFACE (prerequisite_node))
    {
        xType *prerequisites;
        xuint i;

        prerequisites = IFACE_NODE_PREREQUISITES (prerequisite_node);
        for (i = 0; i < IFACE_NODE_N_PREREQUISITES (prerequisite_node); i++) {
            TypeNode *prenode = lookup_type_node_I (prerequisites[i]);
            iface_add_prerequisite_W (iface, prenode);
        }
        iface_add_prerequisite_W (iface, prerequisite_node);
        X_WRITE_UNLOCK (type_nodes);
    }
    else
    {
        X_WRITE_UNLOCK (type_nodes);
        x_warning ("prerequisite `%s' for interface `%s' is "
                   "neither instantiatable nor interface",
                   type_name_I (prerequisite_type),
                   type_name_I (iface_type));
    }
}
xptr
x_iface_peek_parent     (xcptr          iface_ptr)
{
    TypeNode    *node;
    TypeNode    *iface;
    xptr        vtable = NULL;
    xIFace      *iface_class = (xIFace*)iface_ptr;

    x_return_val_if_uninitialized (NULL);
    x_return_val_if_fail (iface_ptr != NULL, NULL);

    iface = lookup_type_node_I (iface_class->type);
    node = lookup_type_node_I (iface_class->instance_type);
    if (node)
        node = lookup_type_node_I (NODE_PARENT_TYPE (node));
    if (node && node->is_instantiatable && iface)
        class_peek_vtable_I (node, iface, &vtable);
    else if (node)
        x_warning ("%s: invalid interface pointer `%p'", X_STRLOC, iface);

    return vtable;
}

xptr
x_class_ref             (xType          type)
{
    TypeNode        *node;
    xType           ptype;
    xbool           holds_ref;
    xClass          *pclass;

    /* optimize for common code path */
    node = lookup_type_node_I (type);
    if (!node || !node->is_classed) {
        x_warning ("cannot retrieve class for invalid (unclassed) type `%s'",
                   type_name_I (type));
        return NULL;
    }

    if (X_LIKELY (type_data_ref_U (node))) {
        if (X_LIKELY (x_atomic_int_get (&node->data->klass.init_state)
                      == INITIALIZED))
            return node->data->klass.klass;
        holds_ref = TRUE;
    }
    else
        holds_ref = FALSE;

    /* here, we either have node->data->class.class == NULL, or a recursive
     * call to g_type_class_ref() with a partly initialized class, or
     * node->data->class.init_state == INITIALIZED, because any
     * concurrently running initialization was guarded by class_init.
     */
    /* required locking order: 1) class_init, 2) type_rw_lock */
    X_REC_LOCK (class_init);

    /* we need an initialized parent class for initializing derived classes */
    ptype = NODE_PARENT_TYPE (node);
    pclass = ptype ? x_class_ref (ptype) : NULL;

    X_WRITE_LOCK (type_nodes);

    if (!holds_ref)
        type_data_ref_Wm (node);

    if (!node->data->klass.klass) /* class uninitialized */
        type_class_init_Wm (node, pclass);

    X_WRITE_UNLOCK (type_nodes);

    if (pclass)
        x_class_unref (pclass);

    X_REC_UNLOCK (class_init);

    return node->data->klass.klass;
}

void
x_class_unref           (xptr           klass)
{
    TypeNode *node;
    xClass *xclass = klass;
    if (!klass) return;

    node = lookup_type_node_I (xclass->type);
    if (node && node->is_classed && NODE_REFCOUNT (node))
        type_data_unref_U (node, FALSE);
    else
        x_warning ("cannot unreference class of invalid (unclassed) type `%s'",
                   type_name_I (xclass->type));
}

void
x_class_set_private     (xcptr          klass,
                         xsize          private_size)
{
    xType instance_type = X_CLASS_TYPE (klass);
    TypeNode *node = lookup_type_node_I (instance_type);
    xsize offset;

    x_return_if_fail (private_size > 0);
    x_return_if_fail (private_size <= 0xffff);

    if (!node || !node->is_instantiatable
        || !node->data || node->data->klass.klass != klass) {
        x_warning ("cannot add private field to "
                   "invalid (non-instantiatable) type '%s'",
                   type_name_I (instance_type));
        return;
    }

    if (NODE_PARENT_TYPE (node)) {
        TypeNode *pnode = lookup_type_node_I (NODE_PARENT_TYPE (node));
        if (node->data->instance.private_size
            != pnode->data->instance.private_size) {
            x_warning ("x_class_add_private() called "
                       "multiple times for the same type");
            return;
        }
    }

    X_WRITE_LOCK (type_nodes);

    offset = ALIGN_STRUCT (node->data->instance.private_size);

    x_assert (offset + private_size <= 0xffff);

    node->data->instance.private_size = offset + private_size;

    X_WRITE_UNLOCK (type_nodes);
}

xbool
x_instance_check        (xcptr          ptr)
{
    x_return_val_if_uninitialized (FALSE);

    if (ptr) {
        const xInstance *instance = ptr;
        if (instance->klass) {
            TypeNode *node = lookup_type_node_I (instance->klass->type);

            if (node && node->is_instantiatable)
                return TRUE;

            x_warning ("instance of invalid non-instantiatable type `%s'",
                       type_name_I (instance->klass->type));
        }
        else
            x_warning ("instance with invalid (NULL) class pointer");
    }
    else
        x_warning ("invalid (NULL) pointer instance");

    return FALSE;
}

xbool
x_instance_is           (xcptr          ptr,
                         xType          type)
{
    TypeNode *node, *iface;
    const xInstance *instance = ptr;

    if (!instance || !instance->klass)
        return FALSE;

    node = lookup_type_node_I (instance->klass->type);
    if (!node)
        return FALSE;
    iface = lookup_type_node_I (type);
    if (!iface)
        return FALSE;
    return node->is_instantiatable &&
        type_conforms_to_U (node, iface, TRUE, FALSE);
}

xInstance*
x_instance_cast         (xcptr          ptr,
                         xType          type)
{
    x_return_val_if_uninitialized (NULL);

    if (ptr) {
        const xInstance *instance = ptr;
        if (instance->klass) {
            TypeNode *node, *iface;
            xbool is_instantiatable, check;

            node = lookup_type_node_I (instance->klass->type);
            is_instantiatable = node && node->is_instantiatable;
            iface = lookup_type_node_I (type);
            check = is_instantiatable && iface &&
                type_conforms_to_U (node, iface, TRUE, FALSE);
            if (check)
                return (xInstance*)instance;

            if (is_instantiatable)
                x_warning ("invalid cast from `%s' to `%s'",
                           type_name_I (instance->klass->type),
                           type_name_I (type));
            else
                x_warning ("invalid uninstantiatable type `%s' in cast to `%s'",
                           type_name_I (instance->klass->type),
                           type_name_I (type));
        }
        else
            x_warning ("invalid unclassed pointer in cast to `%s'",
                       type_name_I (type));
    }

    return NULL;
}

xptr
x_instance_new          (xType          type)
{
    TypeNode        *node;
    xInstance       *instance;
    xClass          *klass;
    xuint           i;
    xsize           total_size;

    node = lookup_type_node_I (type);
    if (!node || !node->is_instantiatable) {
        x_error ("cannot create new instance of "
                 "invalid (non-instantiatable) type `%s'",
                 type_name_I (type));
    }
    /* X_TYPE_IS_ABSTRACT() is an external call: _U */
    if (!node->mutatable_check_cache && X_TYPE_IS_ABSTRACT (type)) {
        x_error ("cannot create instance of "
                 "abstract (non-instantiatable) type `%s'",
                 type_name_I (type));
    }

    klass = x_class_ref (type);

    total_size = total_instance_size_I (node);
    instance = x_slice_alloc0 (total_size);

    X_LOCK (real_class);
    x_hash_table_insert (_real_class_ht, instance, klass);
    X_UNLOCK (real_class);

    for (i = node->n_supers; i > 0; i--) {
        TypeNode *pnode;

        pnode = lookup_type_node_I (node->supers[i]);
        if (pnode->data->instance.instance_init) {
            instance->klass = pnode->data->instance.klass;
            pnode->data->instance.instance_init (instance, klass);
        }
    }

    instance->klass = klass;
    if (node->data->instance.instance_init)
        node->data->instance.instance_init (instance, klass);

    X_LOCK (real_class);
    x_hash_table_remove (_real_class_ht, instance);
    X_UNLOCK (real_class);

    TRACE(X_INSTANCE_NEW(instance, type));
    return instance;
}

void
x_instance_delete       (xptr           instance)
{
    TypeNode    *node;
    xClass      *klass;
    xsize       total_size;

    x_return_if_fail (instance != NULL);
    klass = X_INSTANCE_CLASS(instance);
    x_return_if_fail (klass != NULL);

    node = lookup_type_node_I (klass->type);
    if (!node || !node->is_instantiatable || !node->data ||
        node->data->klass.klass != (xptr) klass) {
        x_warning ("cannot free instance of "
                   "invalid (non-instantiatable) type `%s'",
                   type_name_I (klass->type));
        return;
    }
    /* G_TYPE_IS_ABSTRACT() is an external call: _U */
    if (!node->mutatable_check_cache &&
        X_TYPE_IS_ABSTRACT (NODE_TYPE (node))) {
        x_warning ("cannot free instance of "
                   "abstract (non-instantiatable) type `%s'",
                   NODE_NAME (node));
        return;
    }
    total_size = total_instance_size_I (node);
#ifdef G_ENABLE_DEBUG  
    memset (instance, 0xaa, total_size);
#endif
    x_slice_free1 (total_size, instance);
    x_class_unref (klass);
}

xptr
x_instance_private      (xcptr          instance,
                         xType          private_type)
{
    TypeNode    *node;
    TypeNode    *pnode;
    TypeNode    *private_node;
    xClass      *klass;
    xsize       offset;

    x_return_val_if_fail (instance, NULL);

    X_LOCK (real_class);
    klass = x_hash_table_lookup (_real_class_ht, instance);
    X_UNLOCK (real_class);

    if (!klass)
        klass = X_INSTANCE_CLASS(instance);

    node = lookup_type_node_I (X_CLASS_TYPE(klass));
    if (X_UNLIKELY (!node ||
                    !node->is_instantiatable)) {
        x_warning ("instance of invalid non-instantiatable type `%s'",
                   type_name_I (X_CLASS_TYPE(klass)));
        return NULL;
    }

    private_node = lookup_type_node_I (private_type);
    if (X_UNLIKELY (!private_node ||
                    !NODE_IS_ANCESTOR (private_node, node))) {
        x_warning ("attempt to retrieve private data for invalid type '%s'",
                   type_name_I (private_type));
        return NULL;
    }

    /* Note that we don't need a read lock, since instance existing
     * means that the instance class and all parent classes
     * exist, so the node->data, node->data->instance.instance_size,
     * and node->data->instance.private_size are not going to be changed.
     * for any of the relevant types.
     */

    offset = ALIGN_STRUCT (node->data->instance.instance_size);

    if (NODE_PARENT_TYPE (private_node)) {
        pnode = lookup_type_node_I (NODE_PARENT_TYPE (private_node));
        x_assert (pnode->data && NODE_REFCOUNT (pnode) > 0);

        if (X_UNLIKELY (private_node->data->instance.private_size ==
                        pnode->data->instance.private_size))
        {
            x_warning ("x_instance_private() requires a prior call "
                       "to x_type_add_private()");
            return NULL;
        }

        offset += ALIGN_STRUCT (pnode->data->instance.private_size);
    }

    return X_STRUCT_MEMBER_P (instance, offset);
}

static void
type_init               (void)
{
    if (!x_atomic_ptr_get (&_real_class_ht)
        && x_once_init_enter (&_real_class_ht) ){
        xTypeInfo       info;
        TypeNode        *node;
        xType           type;

        /* quarks */
        _quark_type_flags = x_quark_from_static ("-x-type-private-xTypeFlags");
        _quark_iface_holder = x_quark_from_static ("-x-type-private--IFaceHolder");
        _quark_dependants_array = x_quark_from_static ("-x-type-private--dependants-array");
        _quark_type_mime = x_quark_from_static ("-x-type-private-mime");

        _type_nodes_ht = x_hash_table_new_full (x_direct_hash, (xCmpFunc)x_direct_cmp,
                                                x_free, NULL);

        node = type_node_fundamental_new_W (X_TYPE_VOID, "void", 0);
        type = NODE_TYPE (node);
        x_assert (type == X_TYPE_VOID);
        memset (&info, 0, sizeof (info));
        node = type_node_fundamental_new_W (X_TYPE_IFACE,
                                            "xIFace", X_TYPE_DERIVABLE);
        type = NODE_TYPE (node);
        type_data_make_W (node, &info, NULL);
        x_assert (type == X_TYPE_IFACE);

        /* X_TYPE_* value types */
        _x_value_types_init();

        /* X_TYPE_BOXED, X_TYPE_ENUM, X_TYPE_FLAGS */
        _x_boxed_types_init ();

        /* X_TYPE_OBJECT */
        _x_object_type_init ();

        /* X_TYPE_PARAM, X_TYPE_PARAM_* types*/
        _x_param_types_init();

        /* value transforms */
        _x_value_transforms_init ();

        /* signal system */
        _x_signal_init();

        _real_class_ht = x_hash_table_new (x_direct_hash, x_direct_cmp);
        x_once_init_leave (&_real_class_ht);
    }
}
xbool
x_value_type_check      (xType          type)

{
    return check_is_value_type_U (type);
}

xbool
x_value_holds           (const xValue   *value,
                         xType          type)
{
    return value && check_is_value_type_U (value->type)
        && x_type_is (value->type, type);
}

xValueTable*
x_value_table_peek      (xType          type)
{
    xValueTable *vtable = NULL;
    TypeNode *node = lookup_type_node_I (type);
    xbool has_refed_data, has_table;

    if (node && NODE_REFCOUNT (node) && node->mutatable_check_cache)
        return node->data->common.value_table;

    X_READ_LOCK (type_nodes);

restart_table_peek:
    has_refed_data = node && node->data && NODE_REFCOUNT (node) > 0;
    has_table = has_refed_data && node->data->common.value_table->value_init;
    if (has_refed_data) {
        if (has_table)
            vtable = node->data->common.value_table;
        else if (NODE_IS_IFACE (node)) {
            xuint i;

            for (i = 0; i < IFACE_NODE_N_PREREQUISITES (node); i++) {
                xType prtype = IFACE_NODE_PREREQUISITES (node)[i];
                TypeNode *prnode = lookup_type_node_I (prtype);

                if (prnode->is_instantiatable) {
                    type = prtype;
                    node = lookup_type_node_I (type);
                    goto restart_table_peek;
                }
            }
        }
    }

    X_READ_UNLOCK (type_nodes);

    if (vtable)
        return vtable;

    if (!node)
        x_warning ("%s: type id `%" X_SIZE_FORMAT "' is invalid", X_STRLOC, type);
    if (!has_refed_data)
        x_warning ("can't peek value table for type `%s' "
                   "which is not currently referenced",
                   type_name_I (type));

    return NULL;
}


xbool
x_type_test_flags       (xType          type,
                         xuint          flags)
{
    TypeNode *node;
    xbool result = FALSE;

    node = lookup_type_node_I (type);
    if (node) {
        xuint fflags = flags & TYPE_FUNDAMENTAL_FLAG_MASK;
        xuint tflags = flags & TYPE_FLAG_MASK;

        if (fflags) {
            xFundamentalInfo *finfo = type_fundamental_info_I (node);

            fflags = (finfo->type_flags & fflags) == fflags;
        }
        else
            fflags = TRUE;

        if (tflags) {
            xptr qtf;
            X_READ_LOCK (type_nodes);
            qtf=type_get_qdata_L (node, _quark_type_flags);
            tflags = ((tflags & X_PTR_TO_SIZE (qtf)) == tflags);
            X_READ_UNLOCK (type_nodes);
        }
        else
            tflags = TRUE;

        result = tflags && fflags;
    }

    return result;
}
