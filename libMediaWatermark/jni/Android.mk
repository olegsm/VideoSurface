LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := libMediaWatermark
LOCAL_SRC_FILES := libMediaWatermark.cpp

LOCAL_CFLAGS    := -Werror -std=c++11 -fexceptions -fno-rtti -fPIC
LOCAL_LDLIBS    := -llog -lGLESv2 -ljnigraphics

include $(BUILD_SHARED_LIBRARY)
