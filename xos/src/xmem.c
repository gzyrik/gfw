#include "config.h"
#include <stdlib.h> 
#include <string.h>
#include "xmem.h"
#include "xmsg.h"
#ifdef HAVE_MALLOC_H
#include <malloc.h>             /* memalign() */
#endif

xptr
x_malloc(xsize bytes)
{
    xptr ret = malloc(bytes);
    if (!ret)
        x_error("when alloc %"X_SIZE_FORMAT" bytes, out of memory", bytes);
    return ret;
}

xptr
x_malloc0(xsize bytes)
{
    return memset(x_malloc(bytes),0x0,bytes);
}

xptr
x_realloc(xptr ptr, xsize bytes)
{
    return !ptr  ? malloc (bytes) : realloc(ptr, bytes);
}

xptr
x_memdup(xcptr ptr, xsize bytes)
{
    return memcpy(x_malloc(bytes), ptr, bytes);
}

void
x_free(xptr ptr)
{
    if (ptr) free(ptr);
}
xptr
x_align_malloc          (xsize          bytes,
                         xsize          align)
{
    xptr addr = NULL;
    xsize ret = 0;
    x_return_val_if_fail(align >0, NULL);
    x_return_val_if_fail((align & (align-1))==0, NULL);
#if HAVE_POSIX_MEMALIGN
    ret = posix_memalign (&addr, align, bytes);
    return addr;
#elif HAVE_MEMALIGN
    return  memalign (align, bytes);
#elif HAVE_ALIGNED_MALLOC
    return _aligned_malloc(bytes, align);
#else
    addr = malloc(bytes + sizeof(xsize) + align - 1);
    x_return_val_if_fail(addr, NULL);

    ret = (xsize)addr;
    ret += sizeof(xsize);
    ret = (ret + align - 1) & ~(align - 1);
    *(xsize*)(ret-sizeof(xsize)) = (xsize)addr;
    return (xptr)ret;
#endif
}
void 
x_align_free            (xptr           ptr)
{
    if (ptr) {
#if HAVE_POSIX_MEMALIGN
        free(ptr);
#elif HAVE_MEMALIGN
        free(ptr);
#elif HAVE_ALIGNED_MALLOC
        _aligned_free(ptr);
#else
        xsize addr = (xsize)ptr;
        ptr = (xptr)(*(xsize*)(addr - sizeof(xsize)));
        free(ptr);
#endif
    }
}
xptr
x_align_malloc0         (xsize          bytes,
                         xsize          align)
{
    return memset(x_align_malloc(bytes, align),0x0,bytes);
}
