LOCAL_PATH := $(call my-dir)/../src
include $(CLEAR_VARS)
LOCAL_MODULE    := gfw
LOCAL_INSTALLABLE := true

LOCAL_SRC_FILES := $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_SRC_FILES := $(subst $(LOCAL_PATH)/,,$(LOCAL_SRC_FILES))

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../include/xos \
	$(LOCAL_PATH)/../include/xts \
	$(LOCAL_PATH)/../include/xfw \
	$(LOCAL_PATH)/../include/gml \
	$(LOCAL_PATH)/../include/gfw

LOCAL_LDFLAGS := -L$(TARGET_OBJS) -lxos -lxts -lgml
include $(BUILD_STATIC_LIBRARY)
