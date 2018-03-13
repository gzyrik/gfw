LOCAL_PATH		:= $(call my-dir)
APP_BUILD_SCRIPT:= $(LOCAL_PATH)/make.mk
#default setting
APP_PATH		:= $(LOCAL_PATH)/..
APP_MODULES		:= xos
APP_ABI			:= armeabi-v7a x86 arm64-v8a
ifeq ($(NDK_PLATFORM_PREFIX),android)
    APP_PLATFORM:= android-9
endif
ifeq ($(NDK_PLATFORM_PREFIX),darwin)
    APP_MODULES := xos-test
    APP_ABI		:= x86_64
endif
