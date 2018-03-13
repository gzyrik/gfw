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
                vs9 ='libs',
                ndk = 'libs',
            }
        },
        xts = {
            inc = 'include',
            lib = {
                vs9 = 'libs',
                ndk = 'libs'
            },
            bin = {
                vs9 ='libs',
                ndk = 'libs',
            }
        },
        FreeImage={
            lib= {
                vs9 = 'libs/x86',
            },
            bin = {
                vs9 ='libs/x86',
            }
        },
    },

    inc = {
        dir = 'include',
        'xfw.h',
    },

    lib = {
        vs9 = {
            dir = 'libs', 
            'x86/xfw.lib',
        },
        ndk = {
            dir = 'libs',
            'arm64-v8a/libxfw.a',
            'armeabi-v7a/libxfw.a',
            'x86/libxfw.a',
        }
    },
    bin = {
        vs9 = {
            dir = "libs",
            'x86/xfw.dll',
        },
    },

    vs9 = {
        vs9 = {'xfw.sln','xfw-fi'}
    },
    ndk = {
        mk = 'ndk-build NDK_PROJECT_PATH=projects NDK_OUT=obj NDK_LIBS_OUT=libs'
    },
    ios = {
        mk = 'ios-build NDK_PROJECT_PATH=projects NDK_OUT=obj NDK_LIBS_OUT=libs'
    }
}
