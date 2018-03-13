#include "gfw.h"
int main(int argc, char *argv[])
{
    gMesh *mesh;
    xFile *file;
    xstr path_name, file_name;

    xcstr group = "gmeshc";
    x_putenv("G_MESH_OGRE=TRUE");
    x_putenv("G_SKELETON_OGRE=TRUE");
    x_putenv("X_SLICE=MALLOC");
    if (argc < 3)
        return -1;
    if (!x_path_test(argv[1], 'f'))
        return -1;

    path_name = x_path_get_dirname (argv[1]);
    file_name = x_path_get_basename (argv[1]);

    g_render_open ("", NULL);
    x_res_group_alloc (group, NULL);
    x_res_group_mount (group, path_name);

    mesh = g_mesh_new (file_name, group, NULL);
    x_free (file_name);
    x_resource_load(mesh, FALSE);

    file_name = x_strdup_printf ("%s:%s", path_name, argv[2]);
    file = x_res_group_open (group, file_name, "mode", "wb+", NULL);
    x_free (file_name);

    g_mesh_write(mesh, file, "comment");
    x_object_unref(file);

    if (mesh->skeleton) {
        gSkeleton *skeleton;
        skeleton = g_skeleton_get (mesh->skeleton);
        x_resource_load(skeleton, FALSE);

        file_name = x_strdup_printf ("%s:%s.skeleton", path_name, argv[2]);
        file = x_res_group_open (group, file_name, "mode", "wb+", NULL);
        g_skeleton_write(skeleton, file, "comment");
        x_object_unref(file);
    }

    x_free (path_name);
    x_object_unref (mesh);

    /* free resource */
    x_res_group_free (group);
    g_render_close ();

    return 0;
}
