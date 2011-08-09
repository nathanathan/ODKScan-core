LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#define OPENCV_INCLUDES and OPENCV_LIBS
include $(OPENCV_CONFIG)

LOCAL_LDLIBS += $(OPENCV_LIBS) $(ANDROID_OPENCV_LIBS) -llog -lGLESv2
    
LOCAL_C_INCLUDES +=  $(OPENCV_INCLUDES) $(ANDROID_OPENCV_INCLUDES)

LOCAL_MODULE    := bubblebot

CORE_SRCS := $(subst jni/,./, $(wildcard jni/core/*.cpp))
JSON_PARSER_SRCS := $(subst jsoncpp, ../jsoncpp,$(wildcard jsoncpp-src-0.5.0/src/lib_json/*.cpp))

LOCAL_C_INCLUDES += jni/core
LOCAL_C_INCLUDES += jsoncpp-src-0.5.0/include

LOCAL_SRC_FILES := Feedback.cpp $(CORE_SRCS) gen/bubblebot.cpp $(JSON_PARSER_SRCS)

include $(BUILD_SHARED_LIBRARY)
