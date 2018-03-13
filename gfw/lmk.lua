require('lmk')
{
    deps = {
        xos = {
            inc = 'include',
            lib = {
                vs9 ='libs'
            },
            bin = {
                vs9 ='libs'
            }
        },
        xts = {
            inc = 'include',
            lib = {
                vs9 ='libs',
            },
            bin = {
                vs9 ='libs',
            }
        },
        xfw = {
            inc = 'include',
            lib = {
                vs9 ='libs',
            },
            bin = {
                vs9 ='libs',
            }
        },
        gml = {
            inc = 'include',
            lib = {
                vs9 ='libs',
            },
            bin = {
                vs9 ='libs',
            }
        }
    },

    vs9 = {
        vs9 = {'gfw.sln','gfw-tests'}
    },
    ndk = {
        mk = 'ndk-build NDK_PROJECT_PATH=projects NDK_OUT=obj NDK_LIBS_OUT=libs'
    },
    ios = {
        mk = 'ios-build NDK_PROJECT_PATH=projects NDK_OUT=obj NDK_LIBS_OUT=libs'
    }
}
