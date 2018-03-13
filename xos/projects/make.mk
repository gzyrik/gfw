LOCAL_PATH := $(APP_PATH)/src
include $(CLEAR_VARS)
LOCAL_MODULE    := xos
LOCAL_INSTALLABLE := true

LOCAL_SRC_FILES := $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_SRC_FILES := $(subst $(LOCAL_PATH)/,,$(LOCAL_SRC_FILES))

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../include/xos

include $(BUILD_STATIC_LIBRARY)

LOCAL_PATH := $(APP_PATH)/tests
include $(CLEAR_VARS)
LOCAL_MODULE 	:= xos-test
LOCAL_SRC_FILES := 	\
	main.cpp		\
	test-alloca.cpp	\
	test-json.cpp 	\
	test-mainloop.cpp \
	test-option.cpp

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../include/xos
LOCAL_STATIC_LIBRARIES := xos
LOCAL_LDFLAGS := -liconv
include $(BUILD_EXECUTABLE)
