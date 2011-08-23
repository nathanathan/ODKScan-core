LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OPENCV_CAMERA_MODULES:=off
include ../includeOpenCV.mk
ifeq ("$(wildcard $(OPENCV_MK_PATH))","")
	#try to load OpenCV.mk from default install location
	include $(TOOLCHAIN_PREBUILT_ROOT)/user/share/OpenCV/OpenCV.mk
else
	include $(OPENCV_MK_PATH)
endif

#LOCAL_MODULE    := mixed_sample
#LOCAL_SRC_FILES := jni_part.cpp
#LOCAL_LDLIBS +=  -llog -ldl

#

LOCAL_LDLIBS += $(OPENCV_LIBS) $(ANDROID_OPENCV_LIBS) -llog -ldl -lGLESv2
    
LOCAL_C_INCLUDES +=  $(OPENCV_INCLUDES) $(ANDROID_OPENCV_INCLUDES)

LOCAL_MODULE    := bubblebot

CORE_SRCS := $(addprefix core/, $(notdir $(wildcard jni/core/*.cpp)))
JSON_PARSER_SRCS := $(addprefix ../, $(wildcard jsoncpp-src-0.5.0/src/lib_json/*.cpp))

LOCAL_C_INCLUDES += jni/core
LOCAL_C_INCLUDES += jsoncpp-src-0.5.0/include

LOCAL_SRC_FILES := $(CORE_SRCS) gen/bubblebot.cpp $(JSON_PARSER_SRCS)

#


include $(BUILD_SHARED_LIBRARY)
