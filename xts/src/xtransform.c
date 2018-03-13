#include "config.h"
#include "xvalue.h"
#include "xclass.h"
#include "xclass-prv.h"
#include "xboxed.h"
#include <string.h>
static void
value_transform_str_enum (const xValue *src_value,
                          xValue       *dest_value)
{
    xEnumClass *klass = x_class_ref (X_VALUE_TYPE (dest_value));
    xcstr name = x_value_get_str (src_value);
    if (klass->n_values) {
        xEnumValue *value;

        for (value = klass->values; value->value_name; value++) {
            if (strcmp (name, value->value_name) == 0) {
                x_value_set_enum (dest_value, value->value);
                break;
            }
        }
    }
    x_class_unref (klass);
}
static void
value_transform_str_strv (const xValue *src_value,
                          xValue       *dest_value)
{
    xstrv dst;
    xcstr src;
    src = x_value_get_str (src_value);
    dst = x_strsplit (src, " ", 0);
    x_value_set_boxed(dest_value, dst);
}
void
_x_value_transforms_init(void)
{
    x_value_register_transform (X_TYPE_STR, X_TYPE_ENUM,
                                value_transform_str_enum);

    x_value_register_transform (X_TYPE_STR, X_TYPE_STRV,
                                value_transform_str_strv);
}
