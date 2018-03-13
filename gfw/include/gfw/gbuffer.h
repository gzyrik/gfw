#ifndef __G_BUFFER_H__
#define __G_BUFFER_H__
#include "gtype.h"
X_BEGIN_DECLS

#define G_TYPE_BUFFER                   (g_buffer_type())
#define G_BUFFER(object)                (X_INSTANCE_CAST((object), G_TYPE_BUFFER, gBuffer))
#define G_BUFFER_CLASS(klass)           (X_CLASS_CAST((klass), G_TYPE_BUFFER, gBufferClass))
#define G_IS_BUFFER(object)             (X_INSTANCE_IS_TYPE((object), G_TYPE_BUFFER))
#define G_BUFFER_GET_CLASS(object)      (X_INSTANCE_GET_CLASS((object), G_TYPE_BUFFER, gBufferClass))

#define G_TYPE_PIXEL_BUFFER             (g_pixel_buffer_type())
#define G_PIXEL_BUFFER(object)          (X_INSTANCE_CAST((object), G_TYPE_PIXEL_BUFFER, gPixelBuffer))
#define G_PIXEL_BUFFER_CLASS(klass)     (X_CLASS_CAST((klass), G_TYPE_PIXEL_BUFFER, gPixelBufferClass))
#define G_IS_PIXEL_BUFFER(object)       (X_INSTANCE_IS_TYPE((object), G_TYPE_PIXEL_BUFFER))
#define G_PIXEL_BUFFER_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_PIXEL_BUFFER, gPixelBufferClass))

#define G_TYPE_VERTEX_BUFFER            (g_vertex_buffer_type())
#define G_VERTEX_BUFFER(object)         (X_INSTANCE_CAST((object), G_TYPE_VERTEX_BUFFER, gVertexBuffer))
#define G_VERTEX_BUFFER_CLASS(klass)    (X_CLASS_CAST((klass), G_TYPE_VERTEX_BUFFER, gVertexBufferClass))
#define G_IS_VERTEX_BUFFER(object)      (X_INSTANCE_IS_TYPE((object), G_TYPE_VERTEX_BUFFER))
#define G_VERTEX_BUFFER_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_VERTEX_BUFFER, gVertexBufferClass))

#define G_TYPE_INDEX_BUFFER             (g_index_buffer_type())
#define G_INDEX_BUFFER(object)          (X_INSTANCE_CAST((object), G_TYPE_INDEX_BUFFER, gIndexBuffer))
#define G_INDEX_BUFFER_CLASS(klass)     (X_CLASS_CAST((klass), G_TYPE_INDEX_BUFFER, gIndexBufferClass))
#define G_IS_INDEX_BUFFER(object)       (X_INSTANCE_IS_TYPE((object), G_TYPE_INDEX_BUFFER))
#define G_INDEX_BUFFER_GET_CLASS(object) (X_INSTANCE_GET_CLASS((object), G_TYPE_INDEX_BUFFER, gIndexBufferClass))

typedef enum {
    G_VERTEX_FLOAT1,
    G_VERTEX_FLOAT2,
    G_VERTEX_FLOAT3,
    G_VERTEX_FLOAT4,
    G_VERTEX_A8R8G8B8,
    G_VERTEX_R8G8B8A8,
    G_VERTEX_COLOR32,
    G_VERTEX_SHORT1,
    G_VERTEX_SHORT2,
    G_VERTEX_SHORT3,
    G_VERTEX_SHORT4,
    G_VERTEX_UBYTE4,
} gVertexType;

typedef enum {
    /* unknown semantic */
    G_VERTEX_NONE,
    G_VERTEX_POSITION,
    /* blend weight [0, 1] */
    G_VERTEX_BWEIGHTS,
    /* blend index */
    G_VERTEX_BINDICES,
    G_VERTEX_NORMAL,
    G_VERTEX_DIFFUSE,
    G_VERTEX_SPECULAR,
    G_VERTEX_TEXCOORD,
    G_VERTEX_BINORMAL,
    G_VERTEX_TANGENT,
    G_VERTEX_PSIZE,
} gVertexSemantic;

gBuffer*    g_vertex_buffer_new     (xsize          vertex_size,
                                     xsize          vertex_count,
                                     xcstr          first_property,
                                     ...);

gBuffer*    g_index_buffer_new      (xsize          index_size,
                                     xsize          index_count,
                                     xcstr          first_property,
                                     ...);

xptr        g_buffer_lock_range     (gBuffer        *buffer,
                                     xsize          start,
                                     xssize         count);

#define     g_buffer_lock(buf)  g_buffer_lock_range (G_BUFFER(buf), 0, -1)

void        g_buffer_unlock         (gBuffer        *buffer);

void        g_buffer_read           (gBuffer        *buffer,
                                     xsize          offset,
                                     xssize         count,
                                     xptr           buf);

void        g_buffer_write          (gBuffer        *buffer,
                                     xsize          offset,
                                     xssize         count,
                                     xcptr          buf);

void        g_buffer_copy_range     (gBuffer        *dst_buf,
                                     xsize          dst_off,
                                     gBuffer        *src_buf,
                                     xsize          src_off,
                                     xssize         count);

#define     g_buffer_copy(dst,src)   g_buffer_copy_range (dst, 0, src, 0, -1);

gBuffer*    g_buffer_shadow         (gBuffer        *buffer,
                                     xsize          lifetime,
                                     xbool          copydata);


/* upload  to gpu memory */
void        g_buffer_upload         (gBuffer        *buffer);

xType       g_pixel_buffer_type     (void);

void        g_pixel_buffer_write    (gPixelBuffer   *buffer,
                                     xPixelBox      *pixel_box,
                                     const xbox     *buf_box);

void        g_pixel_buffer_read     (gPixelBuffer   *buffer,
                                     xPixelBox      *pixel_box,
                                     const xbox     *buf_box);

gVertexSpec g_vertex_guess_spec     (xcstr          name);

xsize       g_vertex_type_size      (gVertexType    type);

xsize       g_vertex_data_vsize     (gVertexData    *vdata,
                                     xsize          source);

gVertexData*g_vertex_data_new       (xsize          count);

void        g_vertex_data_delete    (gVertexData    *vdata);

void        g_vertex_data_add       (gVertexData    *vdata,
                                     gVertexElem    *elem);

gBuffer*    g_vertex_data_get       (gVertexData    *vdata,
                                     xsize          source);

void        g_vertex_data_bind      (gVertexData    *vdata,
                                     xsize          source,
                                     gVertexBuffer  *buffer);

#define     g_vertex_data_unbind(vdata, source)  g_vertex_data_bind (vdata, source, NULL)

void        g_vertex_data_remove    (gVertexData    *vdata,
                                     xsize          source);

void        g_vertex_data_alloc     (gVertexData    *vdata);

void        g_vertex_data_copy      (gVertexData    *dst_vdata,
                                     gVertexData    *src_vdata,
                                     xbool          copy_data);

gIndexData* g_index_data_new        (xsize          count);

void        g_index_data_delete     (gIndexData     *idata);

xptr        g_index_data_get        (gIndexData     *idata);

void        g_index_data_bind       (gIndexData     *idata,
                                     gIndexBuffer   *buffer);

#define     g_index_data_unbind(idata)  g_index_data_bind (idata, NULL)

struct _gBuffer
{
    xObject         parent;
    xbyte           *data;
    xsize           size;
    xbool           locked;
    xsize           dirty_start;
    xsize           dirty_end;
    xbool           dynamic;
};
struct _gBufferClass
{
    xObjectClass    parent;
    void (*upload)  (gBuffer *buffer);
};
struct _gPixelBuffer
{
    gBuffer         parent;
    xsize           width;
    xsize           height;
    xsize           depth;
    xsize           row_pitch;
    xsize           slice_pitch;
    xbox            dirty_box;
    xPixelFormat    format;
};
struct _gPixelBufferClass
{
    gBufferClass    parent;
};
struct _gVertexBuffer
{
    gBuffer         parent;
    xsize           vertex_size;
    xsize           vertex_count;
};
struct _gVertexBufferClass
{
    gBufferClass    parent;
};
struct _gIndexBuffer
{
    gBuffer         parent;
    xsize           index_size;
    xsize           index_count;
};
struct _gIndexBufferClass
{
    gBufferClass    parent;
};

struct _gVertexSpec{
    gVertexSemantic semantic;
    /** 按其语义,额外的语义序号 */
    xint            index;
};
struct _gVertexElem
{
    xsize           source;
    /** 在每个顶点数据块中的偏移 */
    xsize           offset;
    gVertexType     type;
    gVertexSemantic semantic;
    /** 按其语义,额外的语义序号 */
    xint            index;
};

struct _gVertexData
{
    xPtrArray       *binding;
    xArray          *elements;
    xsize           start;
    xsize           count;
    xbool           elem_changed;
    xptr            vdeclare;
};
struct _gIndexData
{
    gIndexBuffer    *buffer;
    xsize           start;
    xsize           count;
};

xType       g_buffer_type           (void);

xType       g_vertex_buffer_type    (void);

xType       g_index_buffer_type     (void);

X_END_DECLS
#endif /* __G_BUFFER_H__ */
