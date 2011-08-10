/*
This file is included in:
formAlignment.cpp/.h
PCA_classifier.cpp/.h

It is used to set the value of switches like USE_ANDROID_HEADERS_AND_IO
*/
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

//Maybe rename this define or get rid of it.
//Right now the only place it gets used if for logging.
//I would prefer to put something in the android config.h that
//makes cout or a similar function do logging.
//#define USE_ANDROID_HEADERS_AND_IO

//For printing errors
#define LOGI printf

//Processor.cpp
#define DEBUG_PROCESSOR
#define OUTPUT_SEGMENT_IMAGES
//FormAlignmnet.cpp
#define DEBUG_ALIGN_IMAGE
#define OUTPUT_DEBUG_IMAGES
//#define ALWAYS_COMPUTE_TEMPLATE_FEATURES
//PCA_classifier.cpp
#define DEBUG_CLASSIFIER
#define OUTPUT_BUBBLE_IMAGES
#define OUTPUT_EXAMPLES
/*
To vary parameters with tests use externs like this
#include <opencv2/core/core.hpp>
extern cv::Point TEST_PARAM;
*/
#if 0
extern int EIGENBUBBLES;
#else
#define EIGENBUBBLES 7
#endif
#endif
