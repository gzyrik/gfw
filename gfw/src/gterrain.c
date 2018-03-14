/* quadtree */
struct quadtree
{

};

struct _Tile
{
    gSimple         parent;
    xsize           max_batch;
    xsize           min_batch;
    xsize           n_leaf_lod; /* ��Ҷ��LOD */
    xsize           n_lod;      /* LOD����,LODԽСԽ��ϸ */
    xsize           tree_depth; /* �Ĳ������ */
    struct quadtree *quadtree; /* �Ĳ��� */
};

struct _TileClass
{
    gSimpleClass    parent;
};

struct _gTerrain
{
    xObject         parent;
    xsize           size;       /* �߶�����,�������2^n+1 */
    greal           world_size; /* ��������ϵ�еı߳� */
};

struct _gTerrainClass
{
    xObjectClass    parent;
};

#define TERRAIN_MAX_BATCH   129

/* �ݹ�ָ��Ĳ��� */
static void
quadtree_init (struct quadtree*quadtree, xsize max_batch)
{
    if (quadtree->size > max_batch) { /* �����Ĳ�ָ� */
        xsize i;
        struct quadtree *child;
        xsize half = (quadtree->size-1)/2;
        xsize off[4] = {0, half, 0, half};

        for (i=0;i<4;++i) {
            child = x_slice_new0 (struct quadtree);
            child->x = x + (i&1) * half;
            child->y = y + (i>>1)* half;
            child->size = half + 1;
            child->lod = quadtree->lod - 1;
            child->depth = quadtree->depth + 1;
            quadtree->childre[i] = child;
            quadtree_init (child, max_batch);
        }
    }
    else { /* ����Ҷ��� */

    }
}
static void
quadtree_new (xsize size, xsize n_lod, xsize max_batch)
{
    struct quadtree *quadtree;
    quadtree = x_slice_new0 (struct quadtree); 
    quadtree->x = 0;
    quadtree->y = 0;
    quadtree->size = size;
    quadtree->lod = n_lod-1;
    quadtree->depth = 0;
    quadtree_init (quadtree, max_batch);
    return quadtree;
}
/* �����������LOD�� */
static void
tile_calc_lod_depth (struct tile *tile, xsize size)
{
    tile->n_leaf_lod = g_log2 (tile->max_batch - 1)
        - g_log2 (tile->min_batch - 1) + 1;
    tile->n_lod = g_log2 (size - 1)
        - g_log2 (terrain->min_batch - 1) + 1;
    tile->tree_depth = tile->n_lod - tile->n_leaf_lod + 1;
    x_debug ("terrain: size=%d min_batch=%d max_batch=%d"
             "tree_depth=%d n_lod=%d n_leaf_lod=%d",
             size, tile->min_batch, tile->max_batch,
             tile->tree_depth, tile->n_load, tile->n_leaf_lod);
}
/* ����LOD��ĸ߶Ȳ� */
static void
tile_calc_lod_delta (struct tile *tile, const grect *rect)
{
    xsize lod;

    quadtree_lod_delta_begin (tile->quadtree, rect);

    for (lod=0; lod < terrain->n_lod; ++lod) {
        quadtree_lod_delta_set (tile->quadtree);
    }
    quadtree_lod_delta_end (tile->quadtree, rect);
}

static void
tile_distribute_vdata (struct tile *tile, xsize size)
{
    xsize reso, prev_reso;
    xsize depth, prev_depth;
    xsize splits, target_splits;

    reso = prev_reso = size;
    depth = prev_depth = tile->tree_depth;
    target_splits = (prev_reso-1)/(TERRAIN_MAX_BATCH-1);
    x_debug ("terrain: processing size=%d", size);
    while (depth-- && target_splits) {
        reso = ((reso-1)>>1) + 1;
        splits = (1<<depth);
        if (splits == target_splits) {
            x_debug ("\t assign vdata, resolution=%d,depth=%d - %d, splits=%d",
                     prev_reso, depth, prev_depth, splits);
            quadtree_assign_vdata (terrain->quadtree,
                                   depth, prev_depth, prev_reso,
                                   (prev_reso-1)/splits + 1);
            prev_reso = ((reso - 1) >> 1) + 1;
            target_splits = (prev_reso-1)/(TERRAIN_MAX_BATCH-1);
            prev_depth = depth;
        }
    }
    if (prev_depth > 0) {
        x_debug ("\t assign vdata, resolution=%d,depth=%d - %d, splits=%d",
                 prev_reso, depth, prev_depth, splits);
        x_assert (splits==1 && depth==0);
        quadtree_assign_vdata (terrain->quadtree,
                               depth, prev_depth, prev_reso,
                               (prev_reso-1)/splits + 1);
    }
    x_debug ("terrain: distribute vdata finished");
}
static void
tile_calc_lod (struct tile *tile, gViewport *vp)
{
    if (tile->quadtree) {
        const Camera* cam = vp->getCamera()->getLodCamera();
        /* ��ֱ��λ�߶��ڿɼ�����С����,���ྵͷ=Aʱ,ǡ���ܿ���1�߶���ϸ�� */
        greal A = 1.0f/g_tan(g_camera_fovy(cam) *0.5f);
        //�������
        greal maxPixelError = mMaxPixelError;
        //��������ڵ�λ�߶��ڵı���
        greal T = 2.0f * maxPixelError / vp->getRect().height();
        //����߶�deltaH��ϸ�ڱ���ྵͷ=deltaH*cFactor
        greal cFactor = A/T;
        quadtree_calc_lod (tile->quadtree, cam, cFactor);
    }
}
