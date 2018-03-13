APP_MODULES = gfw
APP_ABI = armeabi-v7a x86 arm64-v8a
ifeq ($(NDK_PLATFORM_PREFIX),android)
    APP_PLATFORM = android-9
endif
APP_BUILD_SCRIPT =$(my-dir)/make.mk
