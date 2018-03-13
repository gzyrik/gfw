#include "config.h"
#include "gbuffer.h"
#include "grender.h"
#include <string.h>
#include <ctype.h>
X_DEFINE_TYPE (gBuffer, g_buffer, X_TYPE_OBJECT);
X_DEFINE_TYPE (gPixelBuffer, g_pixel_buffer, G_TYPE_BUFFER);
X_DEFINE_TYPE (gVertexBuffer, g_vertex_buffer, G_TYPE_BUFFER);
X_DEFINE_TYPE (gIndexBuffer, g_index_buffer, G_TYPE_BUFFER);

static xPtrArray *_buffers;

static void
g_buffer_init           (gBuffer       *buffer)
{
}
static void
buffer_finalize         (xObject        *object)
{
    gBuffer *buffer = (gBuffer*)object;
    if (buffer->data) {
        x_free (buffer->data);
        buffer->data = NULL;
    }
}
static void
g_buffer_class_init     (gBufferClass  *klass)
{
    xObjectClass *oclass;

    oclass = X_OBJECT_CLASS (klass);
    oclass->finalize = buffer_finalize;

}
static void
g_pixel_buffer_init         (gPixelBuffer       *buffer)
{
}
static void
g_pixel_buffer_class_init   (gPixelBufferClass  *klass)
{
}
static void
g_vertex_buffer_init        (gVertexBuffer      *buffer)
{
}
static void
g_vertex_buffer_class_init  (gVertexBufferClass *klass)
{
}
static void
g_index_buffer_init         (gIndexBuffer       *buffer)
{
}
static void
g_index_buffer_class_init   (gIndexBufferClass  *klass)
{
}

xptr
g_buffer_lock_range     (gBuffer        *buffer,
                         xsize          offset,
                         xssize         count)
{
    x_return_val_if_fail (G_IS_BUFFER (buffer), NULL);
    x_return_val_if_fail (buffer->size > 0, NULL);
    x_return_val_if_fail (offset < buffer->size, NULL);
    x_return_val_if_fail (!buffer->locked, NULL);

    buffer->locked = TRUE;

    if (count < 0)
        count = buffer->size - offset;

    if (!buffer->data)
        buffer->data = x_malloc (buffer->size);

    if (buffer->dirty_start != buffer->dirty_end) {
        buffer->dirty_start = MIN (buffer->dirty_start, offset); 
        buffer->dirty_end = MAX (buffer->dirty_end, offset + count);
    }
    else {
        buffer->dirty_start = offset;
        buffer->dirty_end = offset + count;
    }
    return buffer->data + offset;
}

void
g_buffer_unlock         (gBuffer        *buffer)
{
    x_return_if_fail (G_IS_BUFFER (buffer));
    x_return_if_fail (buffer->locked);

    buffer->locked = FALSE;
}

void
g_buffer_read           (gBuffer        *buffer,
                         xsize          offset,
                         xssize         count,
                         xptr           buf)
{
    x_return_if_fail (G_IS_BUFFER (buffer));
    x_return_if_fail (!buffer->locked);
    x_return_if_fail (buffer->data != NULL);

    if (count == -1)
        count = buffer->size - offset;
    else
        x_return_if_fail (offset + count <= buffer->size);

    memcpy (buf, buffer->data+offset, count);
}

void
g_buffer_write          (gBuffer        *buffer,
                         xsize          offset,
                         xssize         count,
                         xcptr          buf)
{
    x_return_if_fail (G_IS_BUFFER (buffer));
    x_return_if_fail (!buffer->locked);
    x_return_if_fail (buffer->size > 0);

    if (count == -1)
        count = buffer->size - offset;
    else
        x_return_if_fail (offset + count <= buffer->size);
    if (buffer->data == NULL)
        buffer->data = x_malloc (buffer->size);
    memcpy (buffer->data+offset, buf, count);

    if (buffer->dirty_start != buffer->dirty_end) {
        buffer->dirty_start = MIN (buffer->dirty_start, offset); 
        buffer->dirty_end = MAX (buffer->dirty_end, offset + count);
    }
    else {
        buffer->dirty_start = offset;
        buffer->dirty_end = offset + count;
    }
}

void
g_buffer_copy_range     (gBuffer        *dst_buf,
                         xsize          dst_off,
                         gBuffer        *src_buf,
                         xsize          src_off,
                         xssize          count)
{
    x_return_if_fail (G_IS_BUFFER (dst_buf));
    x_return_if_fail (!dst_buf->locked);
    x_return_if_fail (dst_buf->size > 0);
    x_return_if_fail (G_IS_BUFFER (src_buf));
    x_return_if_fail (!src_buf->locked);
    x_return_if_fail (src_buf->size > 0);

    if (count == -1) {
        count = MIN (dst_buf->size - dst_off,
                     src_buf->size - src_off);
    }
    else {
        x_return_if_fail (count + dst_off <= dst_buf->size);
        x_return_if_fail (count + src_off <= src_buf->size);
    }

    if (!dst_buf->data)
        dst_buf->data = x_malloc (dst_buf->size);

    g_buffer_read (src_buf, src_off, count, dst_buf->data + dst_off);

    if (dst_buf->dirty_start != dst_buf->dirty_end) {
        dst_buf->dirty_start = MIN (dst_buf->dirty_start, dst_off); 
        dst_buf->dirty_end = MAX (dst_buf->dirty_end, dst_off + count);
    }
    else {
        dst_buf->dirty_start = dst_off;
        dst_buf->dirty_end = dst_off + count;
    }
}

void
g_buffer_upload         (gBuffer        *buffer)
{
    x_return_if_fail (G_IS_BUFFER (buffer));

    if (buffer->dirty_end > buffer->dirty_start) {
        gBufferClass *klass;
        klass = G_BUFFER_GET_CLASS (buffer);
        klass->upload (buffer);
        if (!buffer->dynamic) {
            x_free (buffer->data);
            buffer->data = NULL;
        }
        buffer->dirty_end = buffer->dirty_start;
    }
}
void
g_pixel_buffer_write    (gPixelBuffer   *pixel_buffer,
                         xPixelBox      *src_box,
                         const xbox     *dst_box)
{
    gBuffer *buffer = (gBuffer*)pixel_buffer;
    xPixelBox dst;
    x_return_if_fail (G_IS_PIXEL_BUFFER (pixel_buffer));
    x_return_if_fail (buffer->size != 0);

    if (!dst_box)
        memcpy(&dst, src_box, sizeof(xbox));
    else
        memcpy(&dst, dst_box, sizeof(xbox));

    x_return_if_fail (dst.right <= pixel_buffer->width);
    x_return_if_fail (dst.bottom <= pixel_buffer->height);
    x_return_if_fail (dst.back <= pixel_buffer->depth);
    x_return_if_fail (dst.left < dst.right);
    x_return_if_fail (dst.top < dst.bottom);
    x_return_if_fail (dst.front < dst.back);

    if (buffer->data == NULL)
        buffer->data = x_malloc (buffer->size);
    dst.row_pitch   = pixel_buffer->row_pitch;
    dst.slice_pitch = pixel_buffer->slice_pitch;
    dst.data        = buffer->data;
    dst.format      = pixel_buffer->format;

    x_pixel_box_convert (src_box, &dst, X_SCALE_NEAREST);
    x_box_merge (&pixel_buffer->dirty_box, (xbox*)&dst);
    buffer->dirty_start = 0;
    buffer->dirty_end = buffer->size;
}
void
g_pixel_buffer_read     (gPixelBuffer   *pixel_buffer,
                         xPixelBox      *dst_box,
                         const xbox     *src_box)
{
    xPixelBox src;
    x_return_if_fail (G_IS_PIXEL_BUFFER (pixel_buffer));
    x_return_if_fail (pixel_buffer->parent.data != NULL);

    if (!src_box) {
        src.left = 0;
        src.top  = 0;
        src.front = 0;
        src.right= pixel_buffer->width;
        src.bottom= pixel_buffer->height;
        src.back = pixel_buffer->depth;
    }
    else {
        x_return_if_fail (src_box->right < pixel_buffer->width);
        x_return_if_fail (src_box->bottom < pixel_buffer->height);
        x_return_if_fail (src_box->back < pixel_buffer->depth);
        x_return_if_fail (src_box->left < src_box->right);
        x_return_if_fail (src_box->top < src_box->bottom);
        x_return_if_fail (src_box->front < src_box->back);
        memcpy(&src, src_box, sizeof(xbox));
    }
    src.row_pitch = pixel_buffer->row_pitch;
    src.slice_pitch = pixel_buffer->slice_pitch;
    src.data = pixel_buffer->parent.data;
    x_pixel_box_convert (&src, dst_box, X_SCALE_NEAREST);
}
static xint
vertex_elem_cmp         (gVertexElem    *e1,
                         gVertexElem    *e2)
{
    if (e1->source < e2->source)
        return -1;
    else if (e1->source > e2->source)
        return 1;
    else if (e1->offset < e2->offset)
        return -1;
    else if (e1->offset > e2->offset)
        return 1;
    else if (e1->type < e2->type)
        return -1;
    else if (e1->type > e2->type)
        return 1;
    else if (e1->semantic < e2->semantic)
        return -1;
    else if (e1->semantic > e2->semantic)
        return 1;
    else if (e1->index < e2->index)
        return -1;
    return e1->index > e2->index;
}
xsize
g_vertex_data_vsize     (gVertexData    *vdata,
                         xsize          source)
{
    gVertexElem *elem;
    xsize i;
    x_return_val_if_fail (vdata != NULL, 0);
    if (!vdata->elements)
        return 0;
    i = vdata->elements->len;
    while (i>0) {
        --i;
        elem = &x_array_index (vdata->elements, gVertexElem, i);
        if (elem->source == source)
            return elem->offset + g_vertex_type_size(elem->type);
    }
    return 0;
}
void
g_vertex_data_add       (gVertexData    *vdata,
                         gVertexElem    *velem)
{
    x_return_if_fail (vdata != NULL);
    x_return_if_fail (velem != NULL);

    if (!vdata->elements)
        vdata->elements = x_array_new (sizeof(gVertexElem));
    else if(velem->offset == 0 && vdata->elements->len > 0)
        velem->offset = g_vertex_data_vsize(vdata, velem->source);

    x_array_insert_sorted(vdata->elements, velem, velem,
                          (xCmpFunc)vertex_elem_cmp, FALSE);
    vdata->elem_changed = TRUE;
}
void
g_vertex_data_remove    (gVertexData    *vdata,
                         xsize          source)
{
    xsize i;
    gVertexElem *elem;
    for (i=0;i<vdata->elements->len;++i) {
        elem = &x_array_index (vdata->elements, gVertexElem, i);
        if (elem->source == source) {
            xsize j;
            for (j=i+1;j<vdata->elements->len;++j) {
                elem = &x_array_index (vdata->elements, gVertexElem, i);
                if (elem->source != source)
                    break;
            }
            x_array_remove_range (vdata->elements, i, j-i);
            g_vertex_data_unbind (vdata, source);
            break;
        }
    }
}

gBuffer*
g_vertex_data_get       (gVertexData    *vdata,
                         xsize          source)
{
    x_return_val_if_fail (vdata != NULL, NULL);
    x_return_val_if_fail (vdata->binding != NULL, NULL);
    x_return_val_if_fail (source < vdata->binding->len, NULL);

    return vdata->binding->data[source];
}

void
g_vertex_data_bind      (gVertexData    *vdata,
                         xsize          source,
                         gVertexBuffer  *buffer)
{
    x_return_if_fail (vdata != NULL);
    x_return_if_fail (buffer != NULL);

    if (!vdata->binding) {
        vdata->binding = x_ptr_array_new_full (source + 1,
                                               (xFreeFunc)x_object_unref);
    }

    if (source >= vdata->binding->len) {
        x_ptr_array_append (vdata->binding, NULL,
                            source - vdata->binding->len + 1);
        vdata->binding->data[source] = x_object_ref (buffer);
    }
    else {
        gVertexBuffer *old_buf;
        old_buf = vdata->binding->data[source];
        if (old_buf != buffer) {
            if (old_buf)
                x_object_unref (old_buf);
            if (buffer)
                x_object_ref (buffer);
            vdata->binding->data[source] = buffer;
        }
    }

}

/** close gaps in bindings */
void
g_vertex_data_shrink    (gVertexData    *vdata)
{
}

static void
vertex_data_alloc       (gVertexData    *vdata,
                         xsize          source,
                         xsize          vsize)
{
    gVertexBuffer *vbuf = NULL;
    if (vdata->binding && vdata->binding->len > source)
        vbuf = x_ptr_array_index (vdata->binding, gVertexBuffer, source);
    if (!vbuf || vbuf->vertex_size != vsize || vbuf->vertex_count < vdata->count) {
        vbuf = G_VERTEX_BUFFER(g_vertex_buffer_new (vsize, vdata->count, NULL));
        g_vertex_data_bind (vdata, source, vbuf);
        x_object_unref (vbuf);
    }
}
void
g_vertex_data_alloc     (gVertexData    *vdata)
{
    xsize i;
    xsize source;
    xsize decl_size;
    gVertexElem* elem;

    x_return_if_fail (vdata->count > 0);
    x_return_if_fail (vdata->elements != NULL);

    decl_size  = 0;
    source = -1;
    for (i=0; i<vdata->elements->len; ++i) {
        elem = &x_array_index (vdata->elements, gVertexElem, i);
        if (source == -1)
            source = elem->source;
        if (elem->source != source) {
            vertex_data_alloc (vdata, source, decl_size);
            decl_size = 0;
            source = elem->source;
        }
        decl_size += g_vertex_type_size (elem->type);
    }
    if (source != -1)
        vertex_data_alloc (vdata, source, decl_size);
}

void
g_vertex_data_copy      (gVertexData    *dst_vdata,
                         gVertexData    *src_vdata,
                         xbool          copy_data)
{
}

gVertexData*
g_vertex_data_new       (xsize          count)
{
    gVertexData* vdata;

    vdata = x_malloc0 (sizeof(gVertexData));
    vdata->count = count;
    return vdata;
}

void
g_vertex_data_delete    (gVertexData    *vdata)
{
    if (!vdata) return;
    x_array_unref (vdata->elements);
    x_ptr_array_unref (vdata->binding);
    x_free (vdata);
}

xsize
g_vertex_type_size      (gVertexType    type)
{
    switch (type) {
    case G_VERTEX_FLOAT1: return sizeof(float)*1; 
    case G_VERTEX_FLOAT2: return sizeof(float)*2;
    case G_VERTEX_FLOAT3: return sizeof(float)*3;
    case G_VERTEX_FLOAT4: return sizeof(float)*4;
    case G_VERTEX_A8R8G8B8: return sizeof(xint32);
    case G_VERTEX_SHORT1: return sizeof(short)*1;
    case G_VERTEX_SHORT2: return sizeof(short)*2;
    case G_VERTEX_SHORT3: return sizeof(short)*3;
    case G_VERTEX_SHORT4: return sizeof(short)*4;
    case G_VERTEX_UBYTE4: return sizeof(unsigned char)*4;
    }
    x_warn_if_reached();
    return 0;
}
xptr
g_index_data_get        (gIndexData     *idata)
{
    x_return_val_if_fail (idata != NULL, NULL);
    return idata->buffer;
}

void
g_index_data_bind       (gIndexData     *idata,
                         gIndexBuffer   *buffer)
{
    gIndexBuffer *old_buf;
    x_return_if_fail (idata != NULL);

    old_buf = idata->buffer;
    if (old_buf != buffer) {
        if (old_buf)
            x_object_unref (old_buf);
        if (buffer)
            x_object_ref (buffer);
        idata->buffer = buffer;
    }
}

gIndexData*
g_index_data_new        (xsize          count)
{
    gIndexData* idata;

    idata = x_malloc0 (sizeof(gIndexData));
    idata->count = count;
    return idata;
}

void
g_index_data_delete     (gIndexData     *idata)
{
    if (!idata) return;
    x_object_unref (idata->buffer);
    x_free (idata);
}

static xHashTable *_vsemantic;
gVertexSpec
g_vertex_guess_spec     (xcstr          name)
{
    xchar semantic[256];
    xchar *d = semantic;
    xbool alpha = TRUE;
    xint  s;
    gVertexSpec ret={G_VERTEX_NONE, 0};
    while ((s = *name++) != 0) {
        if (alpha) {
            if (isalpha(s))
                *d++ = toupper (s);
            else if (isdigit(s))
                alpha = FALSE;
        }
        if (!alpha && isdigit(s))
            ret.index = ret.index*10 + (s -'0');
    }
    *d = '\0';
    ret.semantic = (gVertexSemantic)x_hash_table_lookup(_vsemantic, semantic);
    return ret;
}
static xcstr _semantic[] = {
    NULL, "position", "bweights", "bindices",
    "normal","diffuse", "specular", "texcoord", "binormal",
    "tangent", "psize"
};

static xptr
on_vsemantic_enter      (xptr           parent,
                         xcstr          val,
                         xcstr          group,
                         xQuark         parent_name)
{
    xsize i;
    x_return_val_if_fail (val != NULL, NULL);

    for(i=1;i<X_N_ELEMENTS(_semantic); ++i) {
        if (!strcmp (_semantic[i], val))
            return X_SIZE_TO_PTR(i);
    }
    return NULL;
}
static void
on_vsemantic_visit      (xptr           object,
                         xcstr          key,
                         xcstr          val)
{
    x_return_if_fail (key[0] == '\0');
    x_return_if_fail (val != NULL);

    x_hash_table_insert (_vsemantic,
                         x_str_toupper (x_strdup (val)),
                         object);
}
void
_g_buffer_init          (void)
{
    xsize i;
    _buffers = x_ptr_array_new ();
    _vsemantic = x_hash_table_new_full (x_str_hash, (xCmpFunc)x_str_cmp,
                                        x_free, NULL);

    for(i=1;i<X_N_ELEMENTS(_semantic); ++i) {
        x_hash_table_insert (_vsemantic,
                             x_str_toupper (x_strdup (_semantic[i])),
                             X_SIZE_TO_PTR (i));
    }
    x_script_set (NULL, x_quark_from_static("vsemantic"), 
                  on_vsemantic_enter, on_vsemantic_visit, NULL);
}
void
_g_buffer_gc            (void)
{
}
