LOCAL_PATH := $(call my-dir)/../src
include $(CLEAR_VARS)
LOCAL_MODULE    := xfw
LOCAL_INSTALLABLE := true

LOCAL_SRC_FILES := $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_SRC_FILES := $(subst $(LOCAL_PATH)/,,$(LOCAL_SRC_FILES))

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../include/xos \
	$(LOCAL_PATH)/../include/xts \
	$(LOCAL_PATH)/../include/xfw

include $(BUILD_STATIC_LIBRARY)
