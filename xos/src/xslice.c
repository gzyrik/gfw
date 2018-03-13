#include "config.h"
#include "xutil.h"
#include "xinit-prv.h"
#if defined HAVE_POSIX_MEMALIGN && defined POSIX_MEMALIGN_WITH_COMPLIANT_ALLOCS
#  define HAVE_COMPLIANT_POSIX_MEMALIGN 1
#endif

#if defined(HAVE_COMPLIANT_POSIX_MEMALIGN) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 600       /* posix_memalign() */
#endif
#include <stdlib.h>             /* posix_memalign() */
#include <errno.h>



#include <stdio.h>              /* fputs/fprintf */
#include <string.h>

#include "xslice.h"

#include "xmem.h"
#include "xstr.h"
#include "xthread.h"
#include "xbit.h"
#include "xtrash.h"
#include "xutil.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>             /* sysconf() */
#endif
#ifdef X_OS_WINDOWS
#include <windows.h>
#include <process.h>
#define getpid _getpid
#endif
/* --- macros and constants --- */
#define LARGEALIGNMENT          (256)
#define P2ALIGNMENT             (4 * sizeof (xsize))                            /* fits 2 pointers (assumed to be 2 * X_SIZEOF_SIZE_T below) */
#define ALIGN(size, base)       ((base) * (xsize) (((size) + (base) - 1) / (base)))
#define NATIVE_MALLOC_PADDING   P2ALIGNMENT                                     /* per-page padding left for native malloc(3) see [1] */
#define SLAB_INFO_SIZE          P2ALIGN (sizeof (SlabInfo) + NATIVE_MALLOC_PADDING)
#define MAX_MAGAZINE_SIZE       (256)                                           /* see [3] and allocator_get_magazine_threshold() for this */
#define MIN_MAGAZINE_SIZE       (4)
#define MAX_STAMP_COUNTER       (7)                                             /* distributes the load of gettimeofday() */
#define MAX_SLAB_CHUNK_SIZE(al) (((al)->max_page_size - SLAB_INFO_SIZE) / 8)    /* we want at last 8 chunks per page, see [4] */
#define MAX_SLAB_INDEX(al)      (SLAB_INDEX (al, MAX_SLAB_CHUNK_SIZE (al)) + 1)
#define SLAB_INDEX(al, asize)   ((asize) / P2ALIGNMENT - 1)                     /* asize must be P2ALIGNMENT aligned */
#define SLAB_CHUNK_SIZE(al, ix) (((ix) + 1) * P2ALIGNMENT)
#define SLAB_BPAGE_SIZE(al,csz) (8 * (csz) + SLAB_INFO_SIZE)

/* optimized version of ALIGN (size, P2ALIGNMENT) */
#if     X_SIZEOF_SIZE_T * 2 == 8  /* P2ALIGNMENT */
#define P2ALIGN(size)   (((size) + 0x7) & ~(xsize) 0x7)
#elif   X_SIZEOF_SIZE_T * 2 == 16 /* P2ALIGNMENT */
#define P2ALIGN(size)   (((size) + 0xf) & ~(xsize) 0xf)
#else
#define P2ALIGN(size)   ALIGN (size, P2ALIGNMENT)
#endif

/* special helpers to avoid gmessage.c dependency */
static void mem_error (const char *format, ...) X_PRINTF (1,2);
#define mem_assert(cond)    do { if (X_LIKELY (cond)) ; else mem_error ("assertion failed: %s", #cond); } while (0)

/* --- structures --- */
typedef struct _ChunkLink      ChunkLink;
typedef struct _SlabInfo       SlabInfo;
typedef struct _CachedMagazine CachedMagazine;
struct _ChunkLink {
    ChunkLink *next;
    ChunkLink *data;
};
struct _SlabInfo {
    ChunkLink *chunks;
    xuint n_allocated;
    SlabInfo *next, *prev;
};
typedef struct {
    ChunkLink *chunks;
    xsize      count;                     /* approximative chunks list length */
} Magazine;
typedef struct {
    Magazine   *magazine1;                /* array of MAX_SLAB_INDEX (allocator) */
    Magazine   *magazine2;                /* array of MAX_SLAB_INDEX (allocator) */
} ThreadMemory;
typedef struct {
    xbool always_malloc;
    xbool bypass_magazines;
    xbool debug_blocks;
    xsize    working_set_msecs;
    xuint    color_increment;
} SliceConfig;
typedef struct {
    /* const after initialization */
    xsize         min_page_size, max_page_size;
    SliceConfig   config;
    xsize         max_slab_chunk_size_for_magazine_cache;
    /* magazine cache */
    xMutex        magazine_mutex;
    ChunkLink   **magazines;                /* array of MAX_SLAB_INDEX (allocator) */
    xuint        *contention_counters;      /* array of MAX_SLAB_INDEX (allocator) */
    xint          mutex_counter;
    xuint         stamp_counter;
    xuint         last_stamp;
    /* slab allocator */
    xMutex        slab_mutex;
    SlabInfo    **slab_stack;                /* array of MAX_SLAB_INDEX (allocator) */
    xuint        color_accu;
} Allocator;

/* --- x-slice prototypes --- */
static xptr slab_allocator_alloc_chunk       (xsize chunk_size);
static void slab_allocator_free_chunk        (xsize chunk_size,  xptr   mem);
static void private_thread_memory_cleanup    (xptr  data);
static xptr allocator_memalign               (xsize alignment, xsize      memsize);
static void allocator_memfree                (xsize memsize, xptr   mem);
static inline void  magazine_cache_update_stamp (void);
static inline xsize allocator_get_magazine_threshold (Allocator *allocator, xsize ix);

/* --- x-slice memory checker --- */
static void     smc_notify_alloc  (void   *pointer,
        size_t  size);
static int      smc_notify_free   (void   *pointer,
        size_t  size);

/* --- variables --- */
static xPrivate    private_thread_memory = X_PRIVATE_INIT (private_thread_memory_cleanup);
static xsize       sys_page_size = 0;
static Allocator   allocator[1] = { { 0, }, };
static SliceConfig slice_config = {
    FALSE,        /* always_malloc */
    FALSE,        /* bypass_magazines */
    FALSE,        /* debug_blocks */
    15 * 1000,    /* working_set_msecs */
    1,            /* color increment, alt: 0x7fffffff */
};
static xMutex      smc_tree_mutex; /* mutex for X_SLICE=debug-blocks */

static void
slice_config_init (SliceConfig *config)
{
    const xchar *val;

    *config = slice_config;

    val = getenv ("X_SLICE");
    if (val) {
        if (strstr( val, "MALLOC"))
            config->always_malloc = TRUE;
        if (strstr (val, "DEBUG"))
            config->debug_blocks = TRUE;
    }
}

static void
x_slice_init_nomessage (void)
{
    /* we may not use x_error() or friends here */
    mem_assert (sys_page_size == 0);
    mem_assert (MIN_MAGAZINE_SIZE >= 4);

#ifdef X_OS_WINDOWS
    {
        SYSTEM_INFO system_info;
        GetSystemInfo (&system_info);
        sys_page_size = system_info.dwPageSize;
    }
#else
    sys_page_size = sysconf (_SC_PAGESIZE); /* = sysconf (_SC_PAGE_SIZE); = getpagesize(); */
#endif
    mem_assert (sys_page_size >= 2 * LARGEALIGNMENT);
    mem_assert ((sys_page_size & (sys_page_size - 1)) == 0);
    slice_config_init (&allocator->config);
    allocator->min_page_size = sys_page_size;
#if HAVE_COMPLIANT_POSIX_MEMALIGN || HAVE_MEMALIGN
    /* allow allocation of pages up to 8KB (with 8KB alignment).
     * this is useful because many medium to large sized structures
     * fit less than 8 times (see [4]) into 4KB pages.
     * we allow very small page sizes here, to reduce wastage in
     * threads if only small allocations are required (this does
     * bear the risk of increasing allocation times and fragmentation
     * though).
     */
    allocator->min_page_size = MAX (allocator->min_page_size, 4096);
    allocator->max_page_size = MAX (allocator->min_page_size, 8192);
    allocator->min_page_size = MIN (allocator->min_page_size, 128);
#else
    /* we can only align to system page size */
    allocator->max_page_size = sys_page_size;
#endif
    if (allocator->config.always_malloc)
    {
        allocator->contention_counters = NULL;
        allocator->magazines = NULL;
        allocator->slab_stack = NULL;
    }
    else
    {
        allocator->contention_counters = x_new0 (xuint, MAX_SLAB_INDEX (allocator));
        allocator->magazines = x_new0 (ChunkLink*, MAX_SLAB_INDEX (allocator));
        allocator->slab_stack = x_new0 (SlabInfo*, MAX_SLAB_INDEX (allocator));
    }

    x_mutex_init (&allocator->magazine_mutex);
    allocator->mutex_counter = 0;
    allocator->stamp_counter = MAX_STAMP_COUNTER; /* force initial update */
    allocator->last_stamp = 0;
    x_mutex_init (&allocator->slab_mutex);
    allocator->color_accu = 0;
    magazine_cache_update_stamp();
    /* values cached for performance reasons */
    allocator->max_slab_chunk_size_for_magazine_cache = MAX_SLAB_CHUNK_SIZE (allocator);
    if (allocator->config.always_malloc || allocator->config.bypass_magazines)
        allocator->max_slab_chunk_size_for_magazine_cache = 0;      /* non-optimized cases */
}

static inline xuint
allocator_categorize (xsize aligned_chunk_size)
{
    /* speed up the likely path */
    if (X_LIKELY (aligned_chunk_size && aligned_chunk_size <= allocator->max_slab_chunk_size_for_magazine_cache))
        return 1;           /* use magazine cache */

    if (!allocator->config.always_malloc &&
            aligned_chunk_size &&
            aligned_chunk_size <= MAX_SLAB_CHUNK_SIZE (allocator))
    {
        if (allocator->config.bypass_magazines)
            return 2;       /* use slab allocator, see [2] */
        return 1;         /* use magazine cache */
    }
    return 0;             /* use malloc() */
}

static inline void
x_mutex_lock_a (xMutex *mutex,
        xuint  *contention_counter)
{
    xbool contention = FALSE;
    if (!x_mutex_try_lock (mutex))
    {
        x_mutex_lock (mutex);
        contention = TRUE;
    }
    if (contention)
    {
        allocator->mutex_counter++;
        if (allocator->mutex_counter >= 1)        /* quickly adapt to contention */
        {
            allocator->mutex_counter = 0;
            *contention_counter = MIN (*contention_counter + 1, MAX_MAGAZINE_SIZE);
        }
    }
    else /* !contention */
    {
        allocator->mutex_counter--;
        if (allocator->mutex_counter < -11)       /* moderately recover magazine sizes */
        {
            allocator->mutex_counter = 0;
            *contention_counter = MAX (*contention_counter, 1) - 1;
        }
    }
}

static inline ThreadMemory*
thread_memory_from_self (void)
{
    ThreadMemory *tmem = x_private_get (&private_thread_memory);
    if (X_UNLIKELY (!tmem))
    {
        static xMutex init_mutex;
        xsize n_magazines;

        x_mutex_lock (&init_mutex);
        if X_UNLIKELY (sys_page_size == 0)
            x_slice_init_nomessage ();
        x_mutex_unlock (&init_mutex);

        n_magazines = MAX_SLAB_INDEX (allocator);
        tmem = x_malloc0 (sizeof (ThreadMemory) + sizeof (Magazine) * 2 * n_magazines);
        tmem->magazine1 = (Magazine*) (tmem + 1);
        tmem->magazine2 = &tmem->magazine1[n_magazines];
        x_private_set (&private_thread_memory, tmem);
    }
    return tmem;
}

static inline ChunkLink*
magazine_chain_pop_head (ChunkLink **magazine_chunks)
{
    /* magazine chains are linked via ChunkLink->next.
     * each ChunkLink->data of the toplevel chain may point to a subchain,
     * linked via ChunkLink->next. ChunkLink->data of the subchains just
     * contains uninitialized junk.
     */
    ChunkLink *chunk = (*magazine_chunks)->data;
    if (X_UNLIKELY (chunk))
    {
        /* allocating from freed list */
        (*magazine_chunks)->data = chunk->next;
    }
    else
    {
        chunk = *magazine_chunks;
        *magazine_chunks = chunk->next;
    }
    return chunk;
}

#if 0 /* useful for debugging */
static xuint
magazine_count (ChunkLink *head)
{
    xuint count = 0;
    if (!head)
        return 0;
    while (head)
    {
        ChunkLink *child = head->data;
        count += 1;
        for (child = head->data; child; child = child->next)
            count += 1;
        head = head->next;
    }
    return count;
}
#endif

static inline xsize
allocator_get_magazine_threshold (Allocator *allocator, xsize ix)
{
    /* the magazine size calculated here has a lower bound of MIN_MAGAZINE_SIZE,
     * which is required by the implementation. also, for moderately sized chunks
     * (say >= 64 bytes), magazine sizes shouldn't be much smaller then the number
     * of chunks available per page/2 to avoid excessive traffic in the magazine
     * cache for small to medium sized structures.
     * the upper bound of the magazine size is effectively provided by
     * MAX_MAGAZINE_SIZE. for larger chunks, this number is scaled down so that
     * the content of a single magazine doesn't exceed ca. 16KB.
     */
    xsize chunk_size = SLAB_CHUNK_SIZE (allocator, ix);
    xsize threshold = MAX (MIN_MAGAZINE_SIZE, allocator->max_page_size / MAX (5 * chunk_size, 5 * 32));
    xsize contention_counter = allocator->contention_counters[ix];
    if (X_UNLIKELY (contention_counter))  /* single CPU bias */
    {
        /* adapt contention counter thresholds to chunk sizes */
        contention_counter = contention_counter * 64 / chunk_size;
        threshold = MAX (threshold, contention_counter);
    }
    return threshold;
}

/* --- magazine cache --- */
static inline void
magazine_cache_update_stamp (void)
{
    if (allocator->stamp_counter >= MAX_STAMP_COUNTER)
    {
        xTimeVal tv;
        x_ctime (&tv);
        allocator->last_stamp = tv.sec * 1000 + tv.usec / 1000; /* milli seconds */
        allocator->stamp_counter = 0;
    }
    else
        allocator->stamp_counter++;
}

static inline ChunkLink*
magazine_chain_prepare_fields (ChunkLink *magazine_chunks)
{
    ChunkLink *chunk1;
    ChunkLink *chunk2;
    ChunkLink *chunk3;
    ChunkLink *chunk4;
    /* checked upon initialization: mem_assert (MIN_MAGAZINE_SIZE >= 4); */
    /* ensure a magazine with at least 4 unused data pointers */
    chunk1 = magazine_chain_pop_head (&magazine_chunks);
    chunk2 = magazine_chain_pop_head (&magazine_chunks);
    chunk3 = magazine_chain_pop_head (&magazine_chunks);
    chunk4 = magazine_chain_pop_head (&magazine_chunks);
    chunk4->next = magazine_chunks;
    chunk3->next = chunk4;
    chunk2->next = chunk3;
    chunk1->next = chunk2;
    return chunk1;
}

/* access the first 3 fields of a specially prepared magazine chain */
#define magazine_chain_prev(mc)         ((mc)->data)
#define magazine_chain_stamp(mc)        ((mc)->next->data)
#define magazine_chain_uint_stamp(mc)   X_PTR_TO_SIZE ((mc)->next->data)
#define magazine_chain_next(mc)         ((mc)->next->next->data)
#define magazine_chain_count(mc)        ((mc)->next->next->next->data)

static void
magazine_cache_trim (Allocator *allocator,
                     xsize      ix,
                     xsize      stamp)
{
    /* x_mutex_lock (allocator->mutex); done by caller */
    /* trim magazine cache from tail */
    ChunkLink *current = magazine_chain_prev (allocator->magazines[ix]);
    ChunkLink *trash = NULL;
    while (ABS (stamp, magazine_chain_uint_stamp (current)) >= allocator->config.working_set_msecs)
    {
        /* unlink */
        ChunkLink *prev = magazine_chain_prev (current);
        ChunkLink *next = magazine_chain_next (current);
        magazine_chain_next (prev) = next;
        magazine_chain_prev (next) = prev;
        /* clear special fields, put on trash stack */
        magazine_chain_next (current) = NULL;
        magazine_chain_count (current) = NULL;
        magazine_chain_stamp (current) = NULL;
        magazine_chain_prev (current) = trash;
        trash = current;
        /* fixup list head if required */
        if (current == allocator->magazines[ix])
        {
            allocator->magazines[ix] = NULL;
            break;
        }
        current = prev;
    }
    x_mutex_unlock (&allocator->magazine_mutex);
    /* free trash */
    if (trash)
    {
        const xsize chunk_size = SLAB_CHUNK_SIZE (allocator, ix);
        x_mutex_lock (&allocator->slab_mutex);
        while (trash)
        {
            current = trash;
            trash = magazine_chain_prev (current);
            magazine_chain_prev (current) = NULL; /* clear special field */
            while (current)
            {
                ChunkLink *chunk = magazine_chain_pop_head (&current);
                slab_allocator_free_chunk (chunk_size, chunk);
            }
        }
        x_mutex_unlock (&allocator->slab_mutex);
    }
}

static void
magazine_cache_push_magazine (xsize      ix,
                              ChunkLink *magazine_chunks,
                              xsize      count) /* must be >= MIN_MAGAZINE_SIZE */
{
    ChunkLink *current = magazine_chain_prepare_fields (magazine_chunks);
    ChunkLink *next, *prev;
    x_mutex_lock (&allocator->magazine_mutex);
    /* add magazine at head */
    next = allocator->magazines[ix];
    if (next)
        prev = magazine_chain_prev (next);
    else
        next = prev = current;
    magazine_chain_next (prev) = current;
    magazine_chain_prev (next) = current;
    magazine_chain_prev (current) = prev;
    magazine_chain_next (current) = next;
    magazine_chain_count (current) = (xptr) count;
    /* stamp magazine */
    magazine_cache_update_stamp();
    magazine_chain_stamp (current) = X_SIZE_TO_PTR (allocator->last_stamp);
    allocator->magazines[ix] = current;
    /* free old magazines beyond a certain threshold */
    magazine_cache_trim (allocator, ix, allocator->last_stamp);
    /* x_mutex_unlock (allocator->mutex); was done by magazine_cache_trim() */
}

static ChunkLink*
magazine_cache_pop_magazine (xsize  ix, xsize *countp)
{
    x_mutex_lock_a (&allocator->magazine_mutex, &allocator->contention_counters[ix]);
    if (!allocator->magazines[ix])
    {
        xsize magazine_threshold = allocator_get_magazine_threshold (allocator, ix);
        xsize i, chunk_size = SLAB_CHUNK_SIZE (allocator, ix);
        ChunkLink *chunk, *head;
        x_mutex_unlock (&allocator->magazine_mutex);
        x_mutex_lock (&allocator->slab_mutex);
        head = slab_allocator_alloc_chunk (chunk_size);
        head->data = NULL;
        chunk = head;
        for (i = 1; i < magazine_threshold; i++)
        {
            chunk->next = slab_allocator_alloc_chunk (chunk_size);
            chunk = chunk->next;
            chunk->data = NULL;
        }
        chunk->next = NULL;
        x_mutex_unlock (&allocator->slab_mutex);
        *countp = i;
        return head;
    }
    else
    {
        ChunkLink *current = allocator->magazines[ix];
        ChunkLink *prev = magazine_chain_prev (current);
        ChunkLink *next = magazine_chain_next (current);
        /* unlink */
        magazine_chain_next (prev) = next;
        magazine_chain_prev (next) = prev;
        allocator->magazines[ix] = next == current ? NULL : next;
        x_mutex_unlock (&allocator->magazine_mutex);
        /* clear special fields and hand out */
        *countp = (xsize) magazine_chain_count (current);
        magazine_chain_prev (current) = NULL;
        magazine_chain_next (current) = NULL;
        magazine_chain_count (current) = NULL;
        magazine_chain_stamp (current) = NULL;
        return current;
    }
}

/* --- thread magazines --- */
static void
private_thread_memory_cleanup (xptr data)
{
    ThreadMemory *tmem = data;
    const xsize n_magazines = MAX_SLAB_INDEX (allocator);
    xsize ix;
    for (ix = 0; ix < n_magazines; ix++)
    {
        Magazine *mags[2];
        xsize j;
        mags[0] = &tmem->magazine1[ix];
        mags[1] = &tmem->magazine2[ix];
        for (j = 0; j < 2; j++)
        {
            Magazine *mag = mags[j];
            if (mag->count >= MIN_MAGAZINE_SIZE)
                magazine_cache_push_magazine (ix, mag->chunks, mag->count);
            else
            {
                const xsize chunk_size = SLAB_CHUNK_SIZE (allocator, ix);
                x_mutex_lock (&allocator->slab_mutex);
                while (mag->chunks)
                {
                    ChunkLink *chunk = magazine_chain_pop_head (&mag->chunks);
                    slab_allocator_free_chunk (chunk_size, chunk);
                }
                x_mutex_unlock (&allocator->slab_mutex);
            }
        }
    }
    x_free (tmem);
}

static void
thread_memory_magazine1_reload (ThreadMemory *tmem,
                                xsize         ix)
{
    Magazine *mag = &tmem->magazine1[ix];
    mem_assert (mag->chunks == NULL); /* ensure that we may reset mag->count */
    mag->count = 0;
    mag->chunks = magazine_cache_pop_magazine (ix, &mag->count);
}

static void
thread_memory_magazine2_unload (ThreadMemory *tmem, xsize ix)
{
    Magazine *mag = &tmem->magazine2[ix];
    magazine_cache_push_magazine (ix, mag->chunks, mag->count);
    mag->chunks = NULL;
    mag->count = 0;
}

static inline void
thread_memory_swap_magazines (ThreadMemory *tmem,
                              xsize         ix)
{
    Magazine xmag = tmem->magazine1[ix];
    tmem->magazine1[ix] = tmem->magazine2[ix];
    tmem->magazine2[ix] = xmag;
}

static inline xbool
thread_memory_magazine1_is_empty (ThreadMemory *tmem,
                                  xsize         ix)
{
    return tmem->magazine1[ix].chunks == NULL;
}

static inline xbool
thread_memory_magazine2_is_full (ThreadMemory *tmem, xsize ix)
{
    return tmem->magazine2[ix].count >= allocator_get_magazine_threshold (allocator, ix);
}

static inline xptr
thread_memory_magazine1_alloc (ThreadMemory *tmem, xsize ix)
{
    Magazine *mag = &tmem->magazine1[ix];
    ChunkLink *chunk = magazine_chain_pop_head (&mag->chunks);
    if (X_LIKELY (mag->count > 0))
        mag->count--;
    return chunk;
}

static inline void
thread_memory_magazine2_free (ThreadMemory *tmem,xsize ix,xptr mem)
{
    Magazine *mag = &tmem->magazine2[ix];
    ChunkLink *chunk = mem;
    chunk->data = NULL;
    chunk->next = mag->chunks;
    mag->chunks = chunk;
    mag->count++;
}

/* --- API functions --- */

/**
 * x_slice_new:
 * @type: the type to allocate, typically a structure name
 *
 * A convenience macro to allocate a block of memory from the
 * slice allocator.
 *
 * It calls x_slice_alloc() with <literal>sizeof (@type)</literal>
 * and casts the returned pointer to a pointer of the given type,
 * avoiding a type cast in the source code.
 * Note that the underlying slice allocation mechanism can
 * be changed with the <link linkend="X_SLICE">X_SLICE=always-malloc</link>
 * environment variable.
 *
 * Returns: a pointer to the allocated block, cast to a pointer to @type
 *
 * Since: 2.10
 */

/**
 * x_slice_new0:
 * @type: the type to allocate, typically a structure name
 *
 * A convenience macro to allocate a block of memory from the
 * slice allocator and set the memory to 0.
 *
 * It calls x_slice_alloc0() with <literal>sizeof (@type)</literal>
 * and casts the returned pointer to a pointer of the given type,
 * avoiding a type cast in the source code.
 * Note that the underlying slice allocation mechanism can
 * be changed with the <link linkend="X_SLICE">X_SLICE=always-malloc</link>
 * environment variable.
 *
 * Since: 2.10
 */

/**
 * x_slice_dup:
 * @type: the type to duplicate, typically a structure name
 * @mem: the memory to copy into the allocated block
 *
 * A convenience macro to duplicate a block of memory using
 * the slice allocator.
 *
 * It calls x_slice_copy() with <literal>sizeof (@type)</literal>
 * and casts the returned pointer to a pointer of the given type,
 * avoiding a type cast in the source code.
 * Note that the underlying slice allocation mechanism can
 * be changed with the <link linkend="X_SLICE">X_SLICE=always-malloc</link>
 * environment variable.
 *
 * Returns: a pointer to the allocated block, cast to a pointer to @type
 *
 * Since: 2.14
 */

/**
 * x_slice_free:
 * @type: the type of the block to free, typically a structure name
 * @mem: a pointer to the block to free
 *
 * A convenience macro to free a block of memory that has
 * been allocated from the slice allocator.
 *
 * It calls x_slice_free1() using <literal>sizeof (type)</literal>
 * as the block size.
 * Note that the exact release behaviour can be changed with the
 * <link linkend="X_DEBUG">X_DEBUG=gc-friendly</link> environment
 * variable, also see <link linkend="X_SLICE">X_SLICE</link> for
 * related debugging options.
 *
 * Since: 2.10
 */

/**
 * x_slice_free_chain:
 * @type: the type of the @mem_chain blocks
 * @mem_chain: a pointer to the first block of the chain
 * @next: the field name of the next pointer in @type
 *
 * Frees a linked list of memory blocks of structure type @type.
 * The memory blocks must be equal-sized, allocated via
 * x_slice_alloc() or x_slice_alloc0() and linked together by
 * a @next pointer (similar to #GSList). The name of the
 * @next field in @type is passed as third argument.
 * Note that the exact release behaviour can be changed with the
 * <link linkend="X_DEBUG">X_DEBUG=gc-friendly</link> environment
 * variable, also see <link linkend="X_SLICE">X_SLICE</link> for
 * related debugging options.
 *
 * Since: 2.10
 */

/**
 * x_slice_alloc:
 * @block_size: the number of bytes to allocate
 *
 * Allocates a block of memory from the slice allocator.
 * The block adress handed out can be expected to be aligned
 * to at least <literal>1 * sizeof (void*)</literal>,
 * though in general slices are 2 * sizeof (void*) bytes aligned,
 * if a malloc() fallback implementation is used instead,
 * the alignment may be reduced in a libc dependent fashion.
 * Note that the underlying slice allocation mechanism can
 * be changed with the <link linkend="X_SLICE">X_SLICE=always-malloc</link>
 * environment variable.
 *
 * Returns: a pointer to the allocated memory block
 *
 * Since: 2.10
 */
xptr
x_slice_alloc (xsize mem_size)
{
    ThreadMemory *tmem;
    xsize chunk_size;
    xptr mem;
    xsize acat;

    /* This gets the private structure for this thread.  If the private
     * structure does not yet exist, it is created.
     *
     * This has a side effect of causing GSlice to be initialised, so it
     * must come first.
     */
    tmem = thread_memory_from_self ();

    chunk_size = P2ALIGN (mem_size);
    acat = allocator_categorize (chunk_size);
    if (X_LIKELY (acat == 1))     /* allocate through magazine layer */
    {
        xsize ix = SLAB_INDEX (allocator, chunk_size);
        if (X_UNLIKELY (thread_memory_magazine1_is_empty (tmem, ix)))
        {
            thread_memory_swap_magazines (tmem, ix);
            if (X_UNLIKELY (thread_memory_magazine1_is_empty (tmem, ix)))
                thread_memory_magazine1_reload (tmem, ix);
        }
        mem = thread_memory_magazine1_alloc (tmem, ix);
    }
    else if (acat == 2)           /* allocate through slab allocator */
    {
        x_mutex_lock (&allocator->slab_mutex);
        mem = slab_allocator_alloc_chunk (chunk_size);
        x_mutex_unlock (&allocator->slab_mutex);
    }
    else                          /* delegate to system malloc */
        mem = x_align_malloc (mem_size, P2ALIGNMENT);
    if (X_UNLIKELY (allocator->config.debug_blocks))
        smc_notify_alloc (mem, mem_size);

    return mem;
}

/**
 * x_slice_alloc0:
 * @block_size: the number of bytes to allocate
 *
 * Allocates a block of memory via x_slice_alloc() and initializes
 * the returned memory to 0. Note that the underlying slice allocation
 * mechanism can be changed with the
 * <link linkend="X_SLICE">X_SLICE=always-malloc</link>
 * environment variable.
 *
 * Returns: a pointer to the allocated block
 *
 * Since: 2.10
 */
xptr
x_slice_alloc0 (xsize mem_size)
{
    xptr mem = x_slice_alloc (mem_size);
    if (mem)
        memset (mem, 0, mem_size);
    return mem;
}

/**
 * x_slice_copy:
 * @block_size: the number of bytes to allocate
 * @mem_block: the memory to copy
 *
 * Allocates a block of memory from the slice allocator
 * and copies @block_size bytes into it from @mem_block.
 *
 * Returns: a pointer to the allocated memory block
 *
 * Since: 2.14
 */
xptr
x_slice_copy (xsize         mem_size,
        xcptr mem_block)
{
    xptr mem = x_slice_alloc (mem_size);
    if (mem)
        memcpy (mem, mem_block, mem_size);
    return mem;
}

/**
 * x_slice_free1:
 * @block_size: the size of the block
 * @mem_block: a pointer to the block to free
 *
 * Frees a block of memory.
 *
 * The memory must have been allocated via x_slice_alloc() or
 * x_slice_alloc0() and the @block_size has to match the size
 * specified upon allocation. Note that the exact release behaviour
 * can be changed with the
 * <link linkend="X_DEBUG">X_DEBUG=gc-friendly</link> environment
 * variable, also see <link linkend="X_SLICE">X_SLICE</link> for
 * related debugging options.
 *
 * Since: 2.10
 */
void
x_slice_free1 (xsize    mem_size,
        xptr mem_block)
{
    xsize chunk_size = P2ALIGN (mem_size);
    xuint acat = allocator_categorize (chunk_size);
    if (X_UNLIKELY (!mem_block))
        return;
    if (X_UNLIKELY (allocator->config.debug_blocks) &&
            !smc_notify_free (mem_block, mem_size))
        abort();
    if (X_LIKELY (acat == 1))             /* allocate through magazine layer */
    {
        ThreadMemory *tmem = thread_memory_from_self();
        xsize ix = SLAB_INDEX (allocator, chunk_size);
        if (X_UNLIKELY (thread_memory_magazine2_is_full (tmem, ix)))
        {
            thread_memory_swap_magazines (tmem, ix);
            if (X_UNLIKELY (thread_memory_magazine2_is_full (tmem, ix)))
                thread_memory_magazine2_unload (tmem, ix);
        }
        if (X_UNLIKELY (x_mem_gc_friendly))
            memset (mem_block, 0, chunk_size);
        thread_memory_magazine2_free (tmem, ix, mem_block);
    }
    else if (acat == 2)                   /* allocate through slab allocator */
    {
        if (X_UNLIKELY (x_mem_gc_friendly))
            memset (mem_block, 0, chunk_size);
        x_mutex_lock (&allocator->slab_mutex);
        slab_allocator_free_chunk (chunk_size, mem_block);
        x_mutex_unlock (&allocator->slab_mutex);
    }
    else                                  /* delegate to system malloc */
    {
        if (X_UNLIKELY (x_mem_gc_friendly))
            memset (mem_block, 0, mem_size);
        x_align_free (mem_block);
    }
}

/**
 * x_slice_free_chain_with_offset:
 * @block_size: the size of the blocks
 * @mem_chain:  a pointer to the first block of the chain
 * @next_offset: the offset of the @next field in the blocks
 *
 * Frees a linked list of memory blocks of structure type @type.
 *
 * The memory blocks must be equal-sized, allocated via
 * x_slice_alloc() or x_slice_alloc0() and linked together by a
 * @next pointer (similar to #GSList). The offset of the @next
 * field in each block is passed as third argument.
 * Note that the exact release behaviour can be changed with the
 * <link linkend="X_DEBUG">X_DEBUG=gc-friendly</link> environment
 * variable, also see <link linkend="X_SLICE">X_SLICE</link> for
 * related debugging options.
 *
 * Since: 2.10
 */
void
x_slice_free_chain_with_offset (xsize    mem_size,
        xptr mem_chain,
        xsize    next_offset)
{
    xptr slice = mem_chain;
    /* while the thread magazines and the magazine cache are implemented so that
     * they can easily be extended to allow for free lists containing more free
     * lists for the first level nodes, which would allow O(1) freeing in this
     * function, the benefit of such an extension is questionable, because:
     * - the magazine size counts will become mere lower bounds which confuses
     *   the code adapting to lock contention;
     * - freeing a single node to the thread magazines is very fast, so this
     *   O(list_length) operation is multiplied by a fairly small factor;
     * - memory usage histograms on larger applications seem to indicate that
     *   the amount of released multi node lists is negligible in comparison
     *   to single node releases.
     * - the major performance bottle neck, namely x_private_get() or
     *   x_mutex_lock()/x_mutex_unlock() has already been moved out of the
     *   inner loop for freeing chained slices.
     */
    xsize chunk_size = P2ALIGN (mem_size);
    xuint acat = allocator_categorize (chunk_size);
    if (X_LIKELY (acat == 1))             /* allocate through magazine layer */
    {
        ThreadMemory *tmem = thread_memory_from_self();
        xsize ix = SLAB_INDEX (allocator, chunk_size);
        while (slice)
        {
            xuint8 *current = slice;
            slice = *(xptr*) (current + next_offset);
            if (X_UNLIKELY (allocator->config.debug_blocks) &&
                    !smc_notify_free (current, mem_size))
                abort();
            if (X_UNLIKELY (thread_memory_magazine2_is_full (tmem, ix)))
            {
                thread_memory_swap_magazines (tmem, ix);
                if (X_UNLIKELY (thread_memory_magazine2_is_full (tmem, ix)))
                    thread_memory_magazine2_unload (tmem, ix);
            }
            if (X_UNLIKELY (x_mem_gc_friendly))
                memset (current, 0, chunk_size);
            thread_memory_magazine2_free (tmem, ix, current);
        }
    }
    else if (acat == 2)                   /* allocate through slab allocator */
    {
        x_mutex_lock (&allocator->slab_mutex);
        while (slice)
        {
            xuint8 *current = slice;
            slice = *(xptr*) (current + next_offset);
            if (X_UNLIKELY (allocator->config.debug_blocks) &&
                    !smc_notify_free (current, mem_size))
                abort();
            if (X_UNLIKELY (x_mem_gc_friendly))
                memset (current, 0, chunk_size);
            slab_allocator_free_chunk (chunk_size, current);
        }
        x_mutex_unlock (&allocator->slab_mutex);
    }
    else                                  /* delegate to system malloc */
        while (slice)
        {
            xuint8 *current = slice;
            slice = *(xptr*) (current + next_offset);
            if (X_UNLIKELY (allocator->config.debug_blocks) &&
                    !smc_notify_free (current, mem_size))
                abort();
            if (X_UNLIKELY (x_mem_gc_friendly))
                memset (current, 0, mem_size);
            x_align_free (current);
        }
}

/* --- single page allocator --- */
static void
allocator_slab_stack_push (Allocator *allocator, xsize ix, SlabInfo  *sinfo)
{
    /* insert slab at slab ring head */
    if (!allocator->slab_stack[ix])
    {
        sinfo->next = sinfo;
        sinfo->prev = sinfo;
    }
    else
    {
        SlabInfo *next = allocator->slab_stack[ix], *prev = next->prev;
        next->prev = sinfo;
        prev->next = sinfo;
        sinfo->next = next;
        sinfo->prev = prev;
    }
    allocator->slab_stack[ix] = sinfo;
}

static xsize
allocator_aligned_page_size (Allocator *allocator, xsize n_bytes)
{
    xsize val = (xsize)1 << x_bit_storage (n_bytes - 1);
    val = MAX (val, allocator->min_page_size);
    return val;
}

static void
allocator_add_slab (Allocator *allocator, xsize ix, xsize chunk_size)
{
    ChunkLink *chunk;
    SlabInfo *sinfo;
    xsize addr, padding, n_chunks, color = 0;
    xsize page_size = allocator_aligned_page_size (allocator, SLAB_BPAGE_SIZE (allocator, chunk_size));
    /* allocate 1 page for the chunks and the slab */
    xptr aligned_memory = allocator_memalign (page_size, page_size - NATIVE_MALLOC_PADDING);
    xuint8 *mem = aligned_memory;
    xuint i;
    if (!mem)
    {
        const xchar *syserr = "unknown error";
#if HAVE_STRERROR
        syserr = strerror (errno);
#endif
        mem_error ("failed to allocate %u bytes (alignment: %u): %s\n",
                (xuint) (page_size - NATIVE_MALLOC_PADDING), (xuint) page_size, syserr);
    }
    /* mask page address */
    addr = ((xsize) mem / page_size) * page_size;
    /* assert alignment */
    mem_assert (aligned_memory == (xptr) addr);
    /* basic slab info setup */
    sinfo = (SlabInfo*) (mem + page_size - SLAB_INFO_SIZE);
    sinfo->n_allocated = 0;
    sinfo->chunks = NULL;
    /* figure cache colorization */
    n_chunks = ((xuint8*) sinfo - mem) / chunk_size;
    padding = ((xuint8*) sinfo - mem) - n_chunks * chunk_size;
    if (padding)
    {
        color = (allocator->color_accu * P2ALIGNMENT) % padding;
        allocator->color_accu += allocator->config.color_increment;
    }
    /* add chunks to free list */
    chunk = (ChunkLink*) (mem + color);
    sinfo->chunks = chunk;
    for (i = 0; i < n_chunks - 1; i++)
    {
        chunk->next = (ChunkLink*) ((xuint8*) chunk + chunk_size);
        chunk = chunk->next;
    }
    chunk->next = NULL;   /* last chunk */
    /* add slab to slab ring */
    allocator_slab_stack_push (allocator, ix, sinfo);
}

static xptr
slab_allocator_alloc_chunk (xsize chunk_size)
{
    ChunkLink *chunk;
    xsize ix = SLAB_INDEX (allocator, chunk_size);
    /* ensure non-empty slab */
    if (!allocator->slab_stack[ix] || !allocator->slab_stack[ix]->chunks)
        allocator_add_slab (allocator, ix, chunk_size);
    /* allocate chunk */
    chunk = allocator->slab_stack[ix]->chunks;
    allocator->slab_stack[ix]->chunks = chunk->next;
    allocator->slab_stack[ix]->n_allocated++;
    /* rotate empty slabs */
    if (!allocator->slab_stack[ix]->chunks)
        allocator->slab_stack[ix] = allocator->slab_stack[ix]->next;
    return chunk;
}

static void
slab_allocator_free_chunk (xsize    chunk_size,
        xptr mem)
{
    ChunkLink *chunk;
    xbool was_empty;
    xsize ix = SLAB_INDEX (allocator, chunk_size);
    xsize page_size = allocator_aligned_page_size (allocator, SLAB_BPAGE_SIZE (allocator, chunk_size));
    xsize addr = ((xsize) mem / page_size) * page_size;
    /* mask page address */
    xuint8 *page = (xuint8*) addr;
    SlabInfo *sinfo = (SlabInfo*) (page + page_size - SLAB_INFO_SIZE);
    /* assert valid chunk count */
    mem_assert (sinfo->n_allocated > 0);
    /* add chunk to free list */
    was_empty = sinfo->chunks == NULL;
    chunk = (ChunkLink*) mem;
    chunk->next = sinfo->chunks;
    sinfo->chunks = chunk;
    sinfo->n_allocated--;
    /* keep slab ring partially sorted, empty slabs at end */
    if (was_empty)
    {
        /* unlink slab */
        SlabInfo *next = sinfo->next, *prev = sinfo->prev;
        next->prev = prev;
        prev->next = next;
        if (allocator->slab_stack[ix] == sinfo)
            allocator->slab_stack[ix] = next == sinfo ? NULL : next;
        /* insert slab at head */
        allocator_slab_stack_push (allocator, ix, sinfo);
    }
    /* eagerly free complete unused slabs */
    if (!sinfo->n_allocated)
    {
        /* unlink slab */
        SlabInfo *next = sinfo->next, *prev = sinfo->prev;
        next->prev = prev;
        prev->next = next;
        if (allocator->slab_stack[ix] == sinfo)
            allocator->slab_stack[ix] = next == sinfo ? NULL : next;
        /* free slab */
        allocator_memfree (page_size, page);
    }
}

/* --- memalign implementation --- */
#ifdef HAVE_MALLOC_H
#include <malloc.h>             /* memalign() */
#endif

/* from config.h:
 * define HAVE_POSIX_MEMALIGN           1 // if free(posix_memalign(3)) works, <stdlib.h>
 * define HAVE_COMPLIANT_POSIX_MEMALIGN 1 // if free(posix_memalign(3)) works for sizes != 2^n, <stdlib.h>
 * define HAVE_MEMALIGN                 1 // if free(memalign(3)) works, <malloc.h>
 * define HAVE_VALLOC                   1 // if free(valloc(3)) works, <stdlib.h> or <malloc.h>
 * if none is provided, we implement malloc(3)-based alloc-only page alignment
 */

#if !(HAVE_COMPLIANT_POSIX_MEMALIGN || HAVE_MEMALIGN || HAVE_VALLOC)
static xTrash*compat_valloc_trash = NULL;
#endif

static xptr
allocator_memalign (xsize alignment,
        xsize memsize)
{
    xptr aligned_memory = NULL;
    xint err = ENOMEM;
#if     HAVE_COMPLIANT_POSIX_MEMALIGN
    err = posix_memalign (&aligned_memory, alignment, memsize);
#elif   HAVE_MEMALIGN
    errno = 0;
    aligned_memory = memalign (alignment, memsize);
    err = errno;
#elif   HAVE_VALLOC
    errno = 0;
    aligned_memory = valloc (memsize);
    err = errno;
#else
    /* simplistic non-freeing page allocator */
    mem_assert (alignment == sys_page_size);
    mem_assert (memsize <= sys_page_size);
    if (!compat_valloc_trash)
    {
        const xuint n_pages = 16;
        xuint8 *mem = malloc (n_pages * sys_page_size);
        err = errno;
        if (mem)
        {
            xint i = n_pages;
            xuint8 *amem = (xuint8*) ALIGN ((xsize) mem, sys_page_size);
            if (amem != mem)
                i--;        /* mem wasn't page aligned */
            while (--i >= 0)
                x_trash_push (&compat_valloc_trash, amem + i * sys_page_size);
        }
    }
    aligned_memory = x_trash_pop (&compat_valloc_trash);
#endif
    if (!aligned_memory)
        errno = err;
    return aligned_memory;
}

static void
allocator_memfree (xsize    memsize,
        xptr mem)
{
#if     HAVE_COMPLIANT_POSIX_MEMALIGN || HAVE_MEMALIGN || HAVE_VALLOC
    free (mem);
#else
    mem_assert (memsize <= sys_page_size);
    x_trash_push (&compat_valloc_trash, mem);
#endif
}

static void
mem_error (const char *format,
        ...)
{
    const char *pname;
    va_list args;
    /* at least, put out "MEMORY-ERROR", in case we segfault during the rest of the function */
    fputs ("\n***MEMORY-ERROR***: ", stderr);
    pname = x_get_prgname();
    fprintf (stderr, "%s[%ld]: xSlice: ", pname ? pname : "", (long)getpid());
    va_start (args, format);
    vfprintf (stderr, format, args);
    va_end (args);
    fputs ("\n", stderr);
    abort();
    _exit (1);
}

/* --- g-slice memory checker tree --- */
typedef size_t SmcKType;                /* key type */
typedef size_t SmcVType;                /* value type */
typedef struct {
    SmcKType key;
    SmcVType value;
} SmcEntry;
static void             smc_tree_insert      (SmcKType  key,
        SmcVType  value);
static xbool         smc_tree_lookup      (SmcKType  key,
        SmcVType *value_p);
static xbool         smc_tree_remove      (SmcKType  key);


/* --- g-slice memory checker implementation --- */
static void
smc_notify_alloc (void   *pointer,
        size_t  size)
{
    size_t adress = (size_t) pointer;
    if (pointer)
        smc_tree_insert (adress, size);
}

#if 0
static void
smc_notify_ignore (void *pointer)
{
    size_t adress = (size_t) pointer;
    if (pointer)
        smc_tree_remove (adress);
}
#endif

static int
smc_notify_free (void   *pointer,
        size_t  size)
{
    size_t adress = (size_t) pointer;
    SmcVType real_size;
    xbool found_one;

    if (!pointer)
        return 1; /* ignore */
    found_one = smc_tree_lookup (adress, &real_size);
    if (!found_one)
    {
        fprintf (stderr, "GSlice: MemChecker: attempt to release non-allocated block: %p size=%" X_SIZE_FORMAT "\n", pointer, size);
        return 0;
    }
    if (real_size != size && (real_size || size))
    {
        fprintf (stderr, "GSlice: MemChecker: attempt to release block with invalid size: %p size=%" X_SIZE_FORMAT " invalid-size=%" X_SIZE_FORMAT "\n", pointer, real_size, size);
        return 0;
    }
    if (!smc_tree_remove (adress))
    {
        fprintf (stderr, "GSlice: MemChecker: attempt to release non-allocated block: %p size=%" X_SIZE_FORMAT "\n", pointer, size);
        return 0;
    }
    return 1; /* all fine */
}

/* --- g-slice memory checker tree implementation --- */
#define SMC_TRUNK_COUNT     (4093 /* 16381 */)          /* prime, to distribute trunk collisions (big, allocated just once) */
#define SMC_BRANCH_COUNT    (511)                       /* prime, to distribute branch collisions */
#define SMC_TRUNK_EXTENT    (SMC_BRANCH_COUNT * 2039)   /* key address space per trunk, should distribute uniformly across BRANCH_COUNT */
#define SMC_TRUNK_HASH(k)   ((k / SMC_TRUNK_EXTENT) % SMC_TRUNK_COUNT)  /* generate new trunk hash per megabyte (roughly) */
#define SMC_BRANCH_HASH(k)  (k % SMC_BRANCH_COUNT)

typedef struct {
    SmcEntry    *entries;
    unsigned int n_entries;
} SmcBranch;

static SmcBranch     **smc_tree_root = NULL;

static void
smc_tree_abort (int errval)
{
    const char *syserr = "unknown error";
#if HAVE_STRERROR
    syserr = strerror (errval);
#endif
    mem_error ("MemChecker: failure in debugging tree: %s", syserr);
}

static inline SmcEntry*
smc_tree_branch_grow_L (SmcBranch   *branch, xsize index)
{
    unsigned int old_size = branch->n_entries * sizeof (branch->entries[0]);
    unsigned int new_size = old_size + sizeof (branch->entries[0]);
    SmcEntry *entry;
    mem_assert (index <= branch->n_entries);
    branch->entries = (SmcEntry*) realloc (branch->entries, new_size);
    if (!branch->entries)
        smc_tree_abort (errno);
    entry = branch->entries + index;
    memmove (entry + 1, entry, (branch->n_entries - index) * sizeof (entry[0]));
    branch->n_entries += 1;
    return entry;
}

static inline SmcEntry*
smc_tree_branch_lookup_nearest_L (SmcBranch *branch,
        SmcKType   key)
{
    unsigned int n_nodes = branch->n_entries, offs = 0;
    SmcEntry *check = branch->entries;
    int cmp = 0;
    while (offs < n_nodes)
    {
        unsigned int i = (offs + n_nodes) >> 1;
        check = branch->entries + i;
        cmp = key < check->key ? -1 : key != check->key;
        if (cmp == 0)
            return check;                   /* return exact match */
        else if (cmp < 0)
            n_nodes = i;
        else /* (cmp > 0) */
            offs = i + 1;
    }
    /* check points at last mismatch, cmp > 0 indicates greater key */
    return cmp > 0 ? check + 1 : check;   /* return insertion position for inexact match */
}

static void
smc_tree_insert (SmcKType key,
        SmcVType value)
{
    unsigned int ix0, ix1;
    SmcEntry *entry;

    x_mutex_lock (&smc_tree_mutex);
    ix0 = SMC_TRUNK_HASH (key);
    ix1 = SMC_BRANCH_HASH (key);
    if (!smc_tree_root)
    {
        smc_tree_root = calloc (SMC_TRUNK_COUNT, sizeof (smc_tree_root[0]));
        if (!smc_tree_root)
            smc_tree_abort (errno);
    }
    if (!smc_tree_root[ix0])
    {
        smc_tree_root[ix0] = calloc (SMC_BRANCH_COUNT, sizeof (smc_tree_root[0][0]));
        if (!smc_tree_root[ix0])
            smc_tree_abort (errno);
    }
    entry = smc_tree_branch_lookup_nearest_L (&smc_tree_root[ix0][ix1], key);
    if (!entry ||                                                                         /* need create */
            entry >= smc_tree_root[ix0][ix1].entries + smc_tree_root[ix0][ix1].n_entries ||   /* need append */
            entry->key != key)                                                                /* need insert */
        entry = smc_tree_branch_grow_L (&smc_tree_root[ix0][ix1], entry - smc_tree_root[ix0][ix1].entries);
    entry->key = key;
    entry->value = value;
    x_mutex_unlock (&smc_tree_mutex);
}

static xbool
smc_tree_lookup (SmcKType  key,
        SmcVType *value_p)
{
    SmcEntry *entry = NULL;
    unsigned int ix0 = SMC_TRUNK_HASH (key), ix1 = SMC_BRANCH_HASH (key);
    xbool found_one = FALSE;
    *value_p = 0;
    x_mutex_lock (&smc_tree_mutex);
    if (smc_tree_root && smc_tree_root[ix0])
    {
        entry = smc_tree_branch_lookup_nearest_L (&smc_tree_root[ix0][ix1], key);
        if (entry &&
                entry < smc_tree_root[ix0][ix1].entries + smc_tree_root[ix0][ix1].n_entries &&
                entry->key == key)
        {
            found_one = TRUE;
            *value_p = entry->value;
        }
    }
    x_mutex_unlock (&smc_tree_mutex);
    return found_one;
}

static xbool
smc_tree_remove (SmcKType key)
{
    unsigned int ix0 = SMC_TRUNK_HASH (key), ix1 = SMC_BRANCH_HASH (key);
    xbool found_one = FALSE;
    x_mutex_lock (&smc_tree_mutex);
    if (smc_tree_root && smc_tree_root[ix0])
    {
        SmcEntry *entry = smc_tree_branch_lookup_nearest_L (&smc_tree_root[ix0][ix1], key);
        if (entry &&
                entry < smc_tree_root[ix0][ix1].entries + smc_tree_root[ix0][ix1].n_entries &&
                entry->key == key)
        {
            xsize i = entry - smc_tree_root[ix0][ix1].entries;
            smc_tree_root[ix0][ix1].n_entries -= 1;
            memmove (entry, entry + 1, (smc_tree_root[ix0][ix1].n_entries - i) * sizeof (entry[0]));
            if (!smc_tree_root[ix0][ix1].n_entries)
            {
                /* avoid useless pressure on the memory system */
                free (smc_tree_root[ix0][ix1].entries);
                smc_tree_root[ix0][ix1].entries = NULL;
            }
            found_one = TRUE;
        }
    }
    x_mutex_unlock (&smc_tree_mutex);
    return found_one;
}

#ifdef X_ENABLE_DEBUG
void
x_slice_debug_tree_statistics (void)
{
    x_mutex_lock (&smc_tree_mutex);
    if (smc_tree_root)
    {
        unsigned int i, j, t = 0, o = 0, b = 0, su = 0, ex = 0, en = 4294967295u;
        double tf, bf;
        for (i = 0; i < SMC_TRUNK_COUNT; i++)
            if (smc_tree_root[i])
            {
                t++;
                for (j = 0; j < SMC_BRANCH_COUNT; j++)
                    if (smc_tree_root[i][j].n_entries)
                    {
                        b++;
                        su += smc_tree_root[i][j].n_entries;
                        en = MIN (en, smc_tree_root[i][j].n_entries);
                        ex = MAX (ex, smc_tree_root[i][j].n_entries);
                    }
                    else if (smc_tree_root[i][j].entries)
                        o++; /* formerly used, now empty */
            }
        en = b ? en : 0;
        tf = MAX (t, 1.0); /* max(1) to be a valid divisor */
        bf = MAX (b, 1.0); /* max(1) to be a valid divisor */
        fprintf (stderr, "GSlice: MemChecker: %u trunks, %u branches, %u old branches\n", t, b, o);
        fprintf (stderr, "GSlice: MemChecker: %f branches per trunk, %.2f%% utilization\n",
                b / tf,
                100.0 - (SMC_BRANCH_COUNT - b / tf) / (0.01 * SMC_BRANCH_COUNT));
        fprintf (stderr, "GSlice: MemChecker: %f entries per branch, %u minimum, %u maximum\n",
                su / bf, en, ex);
    }
    else
        fprintf (stderr, "GSlice: MemChecker: root=NULL\n");
    x_mutex_unlock (&smc_tree_mutex);

    /* sample statistics (beast + GSLice + 24h scripted core & GUI activity):
     *  PID %CPU %MEM   VSZ  RSS      COMMAND
     * 8887 30.3 45.8 456068 414856   beast-0.7.1 empty.bse
     * $ cat /proc/8887/statm # total-program-size resident-set-size shared-pages text/code data/stack library dirty-pages
     * 114017 103714 2354 344 0 108676 0
     * $ cat /proc/8887/status 
     * Name:   beast-0.7.1
     * VmSize:   456068 kB
     * VmLck:         0 kB
     * VmRSS:    414856 kB
     * VmData:   434620 kB
     * VmStk:        84 kB
     * VmExe:      1376 kB
     * VmLib:     13036 kB
     * VmPTE:       456 kB
     * Threads:        3
     * (gdb) print x_slice_debug_tree_statistics ()
     * GSlice: MemChecker: 422 trunks, 213068 branches, 0 old branches
     * GSlice: MemChecker: 504.900474 branches per trunk, 98.81% utilization
     * GSlice: MemChecker: 4.965039 entries per branch, 1 minimum, 37 maximum
     */
}
#endif /* X_ENABLE_DEBUG */
