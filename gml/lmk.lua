require('lmk')
{
    deps = {
        xos = {
            inc = 'include',
            lib = {
                vs9 = 'libs'
            },
            bin = {
                vs9 = 'libs'
            }
        }
    },
    inc = {
        dir = 'include',
        'gml.h',
    },

    lib = {
        vs9 = {
            dir = 'libs',
            'x86/gml.lib',
        },
    },

    vs9 = {
        vs9 = {'gml.sln', 'gml'}
    },
    ndk = {
        mk = 'ndk-build NDK_PROJECT_PATH=projects NDK_OUT=obj NDK_LIBS_OUT=libs'
    },
    ios = {
        mk = 'ios-build NDK_PROJECT_PATH=projects NDK_OUT=obj NDK_LIBS_OUT=libs'
    }
}
