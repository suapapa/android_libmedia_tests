# Build the unit tests.
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifneq ($(TARGET_SIMULATOR),true)

# Build the unit tests.
test_src_files := \
	MediaScannerClient_test.cpp

shared_libraries := \
	libstlport \
	libutils \
	libmedia \
	libicuuc

static_libraries := \
	libgtest \
	libgtest_main

c_includes := \
    bionic \
    bionic/libstdc++/include \
    external/gtest/include \
    external/icu4c/common \
    external/stlport/stlport

module_tags := eng tests

$(foreach file,$(test_src_files), \
    $(eval include $(CLEAR_VARS)) \
    $(eval LOCAL_SHARED_LIBRARIES := $(shared_libraries)) \
    $(eval LOCAL_STATIC_LIBRARIES := $(static_libraries)) \
    $(eval LOCAL_C_INCLUDES := $(c_includes)) \
    $(eval LOCAL_SRC_FILES := $(file)) \
    $(eval LOCAL_MODULE := $(notdir $(file:%.cpp=%))) \
    $(eval LOCAL_MODULE_TAGS := $(module_tags)) \
    $(eval include $(BUILD_EXECUTABLE)) \
)

endif
