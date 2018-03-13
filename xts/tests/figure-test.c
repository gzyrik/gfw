#include "figure/figure.h"
#include <xos/xutil.h>
void figure_test()
{
    xTimer          timer;
    xTypeModule     *module;
    Figure          *figure;
    xType           *children;
    xsize           i,j,n_children;
    FigureIFace     *iface;
    const xsize     times = 0xFFFFFF;

    module = x_object_new (X_TYPE_TYPE_MODULE,
                           "path","figure-plugin",
                           //"entry","type_module_main",
                           NULL);
    x_type_plugin_use(X_TYPE_PLUGIN(module));
    children = x_type_children(TYPE_FIGURE,&n_children);
    for (i = 0; i < n_children;++i){
        figure = x_object_new (children[i], "width", 100, "height", 100, NULL);
        iface =  FIGURE_GET_CLASS(figure);

        x_timer_start(&timer);
        for(j=0;j<times;++j)
            figure_get_area(figure);
        x_timer_stop(&timer);
        x_printf("child type:%s iface=%f", x_type_name(children[i]), x_timer_elapsed(&timer)/(double)times);

        x_timer_start(&timer);
        for(j=0;j<times;++j)
            iface->get_area(figure);
        x_timer_stop(&timer);
        x_printf("child type:%s iface=%f", x_type_name(children[i]), x_timer_elapsed(&timer)/(double)times);

        x_object_unref(figure);
    }
    x_free(children);
    x_type_plugin_unuse(X_TYPE_PLUGIN(module));
    x_object_unref(module);
}
