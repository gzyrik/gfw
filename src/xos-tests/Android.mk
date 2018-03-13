LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := xos-tests
LOCAL_SRC_FILES := uri-test.c gc-test.c xos-tests.c
LOCAL_SHARED_LIBRARIES := libxos libiconv
include $(BUILD_EXECUTABLE)
