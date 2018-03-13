require('lmk')
{
    inc = {
        dir = 'include',
        'xos.h',
    },
    lib = {
        vs9 = {
            dir = 'libs', 
            'x86/xos.lib',
            --'x86_64/xos.lib',
        },
        ndk = {
            dir = 'libs',
            'arm64-v8a/libxos.a',
            'armeabi-v7a/libxos.a',
            'x86/libxos.a',
        }
    },
    bin = {
        vs9 = {
            dir = "libs",
            'x86/xos.dll',
            --'x86_64/xos.dll',
        },
    },

    vs9 = {
        vs9 = {'xos_vs9.sln', 'xos'}
    },
    ndk = {
        mk = 'ndk-build NDK_PROJECT_PATH=projects NDK_OUT=obj NDK_LIBS_OUT=libs'
    },
    ios = {
        mk = 'ios-build NDK_PROJECT_PATH=projects NDK_OUT=obj NDK_LIBS_OUT=libs'
    },
    mk = {
        mk = 'mk-build NDK_PROJECT_PATH=projects NDK_OUT=obj NDK_LIBS_OUT=libs'
    }
}
