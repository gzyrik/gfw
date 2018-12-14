LOCAL_PATH := $(MY_SOLUTION_PATH)/src
RDP_ALL_FILES := $(subst $(LOCAL_PATH)/,,\
	$(wildcard $(LOCAL_PATH)/*.hpp)	\
	$(wildcard $(LOCAL_PATH)/*.cpp)	\
	$(wildcard $(LOCAL_PATH)/fec/*.h)\
	$(wildcard $(LOCAL_PATH)/fec/*.cc))
RDP_SRC_FILES := $(filter-out %.tests.cpp, $(RDP_ALL_FILES))
include $(CLEAR_VARS)
LOCAL_MODULE    := rdp_tests
LOCAL_SRC_FILES := \
	$(filter %.tests.cpp, $(RDP_ALL_FILES)) \
	$(addsuffix .cov,$(filter %.cpp %.cc,$(RDP_SRC_FILES)))

LOCAL_STATIC_LIBRARIES := googletest_main
include $(BUILD_EXECUTABLE)
$(call import-module,third_party/googletest)
