#include <windows.h>
#include <gfw.h>
gState* g_walk_state = 0;
void write_mesh(gMesh *mesh) {
    xArchive* archive;
    xFile* file;
    archive = x_archive_load(".",NULL);
    file = x_archive_open(archive,"tets.meshx","mode", "wb", NULL);
    g_mesh_write(mesh, file, FALSE);
    x_object_unref(file);
    x_object_unref(archive);
}
void init_scene (gScene  *scene){
    gNode *node;
    gMovable *entity;
    gBBoardSet *bbset;
    gMesh *mesh;
    gPSystem *psystem;
    xArray* boards;
    gBBoard *board;
    xFile *file;

    /* sky box or sky dome     */
    //g_scene_new_sky_box (scene,g_material_get("Examples/CloudyNoonSkyBox"), 1000, G_QUAT_EYE, 0);
    g_scene_new_sky_dome (scene,"Example", 1000, G_QUAT_EYE, 0, NULL);

    /* entity and clone  */
    entity = g_movable_new("entity",
        "mesh", "robot.mesh", 
        "material", "robot",
        NULL);

    node = g_node_new_child (g_scene_root_node(scene), "wbbox-visible", FALSE, NULL);
    g_node_move(node, g_vec(-200,0,0,0), G_XS_PARENT);
    g_scene_node_attach (G_SCENE_NODE(node), entity);

    node = g_node_new_child (g_scene_root_node (scene), NULL);
    g_node_move(node, g_vec(200,0,0,0), G_XS_PARENT);
    g_node_yaw (node,G_PI, G_XS_LOCAL);
    g_scene_node_attach (G_SCENE_NODE(node), entity);

    node = g_node_new_child (g_scene_root_node(scene),NULL);
    g_scene_node_attach (G_SCENE_NODE(node), entity);
    return;

    /* bboard set */
    bbset = G_BBOARD_SET(g_movable_new("bboardset",
                                       "name", "bbset",
                                       "material", "Example",
                                       "pool-size", 3,
                                       "accurate-face", FALSE,
                                       "common-width",100.0,
                                       "common-height",100.0,
                                       "common-size", TRUE,
                                       "lbbox-visible", FALSE,
                                       NULL));
    g_bboardset_texgrid (bbset, 2,2);

    boards = g_bboardset_lock(bbset);
    board = x_array_append1(boards, NULL);
    board->position = g_vec(-150,0,0,0); board->tex_index = 0;
    board = x_array_append1(boards, NULL);
    board->position = g_vec(0,0,0,0); board->tex_index = 1;
    board = x_array_append1(boards, NULL);
    board->position = g_vec(150,0,0,0); board->tex_index = 2;
    g_bboardset_unlock (bbset);

    node = g_node_new_child (g_scene_root_node(scene),"wbbox-visible",TRUE,NULL);
    g_node_move (node, g_vec(0,260,0,0), G_XS_PARENT);
    g_scene_node_attach (G_SCENE_NODE(node), bbset);
    
    /* particle system */
    psystem = G_PSYSTEM (g_movable_new("psystem",
                                       "name", "psystem",
                                       "lbbox-visible", TRUE,
                                       NULL));
    g_psystem_add_emitter (psystem, "", NULL);
    g_psystem_set_render (psystem, "bboardset",
                          "name", "psystem-render",
                          "material", "Example",
                          "accurate-face", FALSE,
                          "common-width",10.0,
                          "common-height",10.0,
                          "common-size", TRUE,
                          NULL);
    node = g_node_new_child (g_scene_root_node(scene), NULL);
    g_scene_node_attach (G_SCENE_NODE(node), psystem);


   
    /* animation */
    g_walk_state = g_states_get (g_entity_states (entity), "Walk");
    if (g_walk_state)
        g_walk_state->enable = TRUE;

}

int main(int argc, char *argv[])
{
    gTarget *window;
    gScene  *scene;
    gNode   *camera_node;
    gCamera *camera;
    gViewport *viewport;
    gRayQuery *rquery;
    xssize nowMs, lastMs;

    /* module */
    x_type_moudle_load ("gfw-d3d9");
    x_type_moudle_load ("xfw-fi");
    x_type_moudle_load ("gfw-octree");
    //x_type_moudle_load ("gfw-cg");


    /* render */
    g_render_open ("D3d9Render", "width", 400, "height",300, NULL);
    window = g_render_target (NULL);
    viewport = g_target_add_view (window, 0);

    /* scene */
    scene  = g_scene_new ("",
                          "tree-depth", 5,
                          "world-size", 500.0,500.0,500.0,
                          NULL);

    camera = g_scene_add_camera (scene, NULL);

    /* resource */
    x_repository_mount ("gfw-test", "/", x_archive_load("../media/model", NULL));
    x_repository_mount ("gfw-test", "/", x_archive_load("../media/image", NULL));
    x_repository_mount ("gfw-test", "/", x_archive_load("../media/shader", NULL));
    x_repository_mount ("gfw-test", "/", x_archive_load("../media", NULL));

    /* link scene and render */
    g_viewport_set_camera (viewport, camera);

    /* load resource */
    x_repository_load ("gfw-test",FALSE);

    init_scene (scene);

    /* attach camera to node */
    camera_node = g_node_new_child (g_scene_root_node (scene),NULL);
    g_scene_node_attach (G_SCENE_NODE(camera_node), G_MOVABLE (camera));
    g_frustum_move (G_FRUSTUM(camera), g_vec(0,500,500,0), G_XS_PARENT);
    g_frustum_lookat (G_FRUSTUM(camera), G_VEC_0, G_XS_PARENT, G_VEC_NEG_Z);

    /* reay query */
    rquery = G_RAY_QUERY(g_scene_new_query(scene, "ray", NULL));
    rquery->parent.query_mask = ~G_QUERY_SKY;
    rquery->parent.type_mask = G_QUERY_BBOARD | G_QUERY_ENTITY;

    /* loop rendering */
    nowMs = lastMs = x_mtime()/1000;
    while (TRUE) {
        MSG  msg;
        greal deltaMs = (nowMs-lastMs)/1000.0f;
        while (PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE)){
            if (msg.message == WM_QUIT || msg.message == WM_CLOSE || msg.message == WM_DESTROY)        
                goto clean;
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        g_node_yaw (camera_node, deltaMs, G_XS_LOCAL);

        if (g_walk_state) {
            g_walk_state->time += deltaMs;
            g_states_dirty (g_walk_state->parent);
        }
/*
        rquery->ray = g_frustum_ray (G_FRUSTUM(camera), 0, 0);
        if (rquery->result->len)
            x_object_set (rquery->result->data[0], "lbbox-visible", FALSE, NULL);
        g_query_collect(G_QUERY(rquery));
        if (rquery->result->len)
            x_object_set (rquery->result->data[0], "lbbox-visible", TRUE, NULL);
*/
        g_render_update (nowMs);
        lastMs = nowMs;
        nowMs  = x_mtime()/1000;
        //x_usleep (100000);
    }
clean:
    /* free resource */
    x_repository_unload ("gfw-test", FALSE);

    /* free render, scene */
    x_object_unref (scene);
    g_render_close ();
    return 0;
}
