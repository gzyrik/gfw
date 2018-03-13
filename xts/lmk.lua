require('lmk')
{
    deps = {
        xos = {
            inc = 'include',
            lib = {
                vs9 = 'libs',
                ndk = 'libs'
            },
            bin = {
                vs9 = 'libs',
                ndk = 'libs',
            }
        }
    },

    inc = {
        dir = 'include',
        'xts.h',
    },

    lib = {
        vs9 = {
            dir = 'libs', 
            'x86/xts.lib',
        },
        ndk = {
            dir = 'libs',
            'arm64-v8a/libxts.a',
            'armeabi-v7a/libxts.a',
            'x86/libxts.a',
        }
    },
    bin = {
        vs9 = {
            dir = "libs",
            'x86/xts.dll',
        },
    },

    vs9 = {
        vs9 = {'xts_vs9.sln', 'xts'}  
    },
    ndk = {
        mk = 'ndk-build NDK_PROJECT_PATH=projects NDK_OUT=obj NDK_LIBS_OUT=libs'
    },
    ios = {
        mk = 'ios-build NDK_PROJECT_PATH=projects NDK_OUT=obj NDK_LIBS_OUT=libs'
    }
}
