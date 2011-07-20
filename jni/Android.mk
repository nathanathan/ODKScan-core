LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#define OPENCV_INCLUDES and OPENCV_LIBS
include $(OPENCV_CONFIG)

LOCAL_LDLIBS += $(OPENCV_LIBS) $(ANDROID_OPENCV_LIBS) -llog -lGLESv2
    
LOCAL_C_INCLUDES +=  $(OPENCV_INCLUDES) $(ANDROID_OPENCV_INCLUDES)

LOCAL_MODULE    := bubblebot

#For the JSON parsing library I added the following two lines
LOCAL_C_INCLUDES += jsoncpp-src-0.5.0/include
STUFF_FROM_JSON_PARSER := $(subst jsoncpp, ../jsoncpp,$(wildcard jsoncpp-src-0.5.0/src/lib_json/*.cpp))

LOCAL_SRC_FILES := Feedback.cpp Processor.cpp gen/bubblebot.cpp FormAlignment.cpp PCA_classifier.cpp FileUtils.cpp Addons.cpp MarkupForm.cpp $(STUFF_FROM_JSON_PARSER)

include $(BUILD_SHARED_LIBRARY)

