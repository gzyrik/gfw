#include <windows.h>
#include <gfw.h>
gState* g_walk_state;
void init_scene (gScene  *scene){
    gNode *node;
    gMovable *entity;
    gBBoardSet *bbset;
    gMesh *mesh;
    gPSystem *psystem;

    /* bboard set */
    bbset = G_BBOARD_SET(g_movable_new("bboardset",
                                       "name", "bbset",
                                       "material", g_material_get ("Example"),
                                       "pool-size", 3,
                                       "accurate-face", FALSE,
                                       "common-width",100.0,
                                       "common-height",100.0,
                                       "common-size", TRUE,
                                       "lbbox-visible", FALSE,
                                       NULL));
    g_bboardset_grid (bbset, 2,2);
    g_bboardset_add (bbset, -150,0,0,0);
    g_bboardset_add (bbset, 0,0,0,1);
    g_bboardset_add (bbset, 150,0,0,2);
    g_bboardset_dirty (bbset);
    node = g_node_new_child (g_scene_root_node(scene),"wbbox-visible",FALSE,NULL);
    g_node_move (node, g_vec(0,260,0,0), G_XS_PARENT);
    g_snode_attach (G_SCENE_NODE(node), bbset);
 
    /* simple entity */
    mesh = g_mesh_new ("axes.mesh", "gfw-test", NULL, NULL);
    entity = g_movable_new("entity",
                           "name", "axes.mesh",
                           "mesh", mesh,
                           "lbbox-visible", FALSE,
                           NULL);
    x_object_unref (mesh);
    node = g_node_new_child (g_scene_root_node(scene),NULL);
    g_snode_attach (G_SCENE_NODE(node), entity);
    
    /* particle system */
    psystem = G_PSYSTEM (g_movable_new("psystem",
                                       "name", "psystem",
                                       "lbbox-visible", TRUE,
                                       NULL));
    g_psystem_add_emitter (psystem, "", NULL);
    g_psystem_set_render (psystem, "bboardset",
                          "name", "psystem-render",
                          "material", g_material_get ("flare"),
                          "accurate-face", FALSE,
                          "common-width",10.0,
                          "common-height",10.0,
                          "common-size", TRUE,
                          NULL);
    node = g_node_new_child (g_scene_root_node(scene), NULL);
    g_snode_attach (G_SCENE_NODE(node), psystem);

    /* entity and clone  */
    mesh = g_mesh_new ("robot.mesh", "gfw-test", NULL, NULL);
    entity = g_movable_new("entity",
                           "name", "robot.mesh",
                           "mesh", mesh, 
                           "material", g_material_get ("robot"),
                           NULL);
    x_object_unref (mesh);


    node = g_node_new_child (g_scene_root_node(scene), "wbbox-visible", FALSE, NULL);
    g_node_move(node, g_vec(-200,0,0,0), G_XS_PARENT);
    g_snode_attach (G_SCENE_NODE(node), entity);

    node = g_node_new_child (g_scene_root_node (scene), NULL);
    g_node_move(node, g_vec(200,0,0,0), G_XS_PARENT);
    g_node_yaw (node,G_PI, G_XS_LOCAL);
    g_snode_attach (G_SCENE_NODE(node), entity);
   
    /* animation */
    g_walk_state = g_states_get (g_entity_states (entity), "Walk");
    g_walk_state->enable = TRUE;

    /* sky box or sky dome     */
    //g_scene_new_sky_box (scene,"Examples/CloudyNoonSkyBox", 50, G_QUAT_EYE, 0);
    g_scene_new_sky_dome (scene,g_material_get("Examples/CloudySky"), 1000, G_QUAT_EYE, 0, NULL);
}
int main(int argc, char *argv[])
{
    gTarget *window;
    gScene  *scene;
    gNode   *camera_node;
    gCamera *camera;
    gViewport *viewport;
    gRayQuery *rquery;

    greal ftime=0;

    /* module */
    x_type_moudle_load ("gfw-d3d9");
    x_type_moudle_load ("xfw-fi");
    x_type_moudle_load ("gfw-octree");
    //x_type_moudle_load ("gfw-cg");


    /* render */
    g_render_open ("D3d9Render", "width", 400, "height", 300, NULL);
    window = g_render_target (NULL);
    viewport = g_target_add_view (window, 0);

    /* scene */
    scene  = g_scene_new ("",
                          "tree-depth", 5,
                          "world-size", 500.0,500.0,500.0,
                          NULL);

    camera = g_scene_add_camera (scene, NULL);

    /* resource */
    x_res_group_alloc ("gfw-test", NULL);
    x_res_group_mount ("gfw-test", "../media");

    /* link scene and render */
    g_viewport_set_camera (viewport, camera);

    /* load resource */
    x_res_group_load ("gfw-test",FALSE);

    init_scene (scene);

    /* attach camera to node */
    camera_node = g_node_new_child (g_scene_root_node (scene),NULL);
    g_snode_attach (G_SCENE_NODE(camera_node), G_MOVABLE (camera));
    g_frustum_move (G_FRUSTUM(camera), g_vec(0,500,550,0), G_XS_PARENT);
    g_frustum_lookat (G_FRUSTUM(camera), G_VEC_0, G_XS_PARENT, G_VEC_NEG_Z);

    /* reay query */
    rquery = G_RAY_QUERY(g_scene_new_query(scene, "ray", NULL));
    rquery->parent.query_mask = ~G_QUERY_SKY;
    rquery->parent.type_mask = G_QUERY_BBOARD | G_QUERY_ENTITY;
    /* loop rendering */
    while (TRUE) {
        MSG  msg;
        while (PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE)){
            if (msg.message == WM_QUIT || msg.message == WM_CLOSE || msg.message == WM_DESTROY)        
                goto clean;
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        g_node_yaw (camera_node,ftime *0.5f, G_XS_LOCAL);
        if (g_walk_state) {
            g_walk_state->time += ftime;
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
        ftime = g_render_update (x_mtime()/1000);
        g_render_swap ();
        //x_usleep (100000);
    }
clean:
    /* free resource */
    x_res_group_free ("gfw-test");

    /* free render, scene */
    x_object_unref (scene);
    g_render_close ();
    return 0;
}
