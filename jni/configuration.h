/*
This file is included in:
formAlignment.cpp/.h
PCA_classifier.cpp/.h
Processor.cpp/.h

It is used to set the value of switches like USE_ANDROID_HEADERS_AND_IO
*/
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define EIGENBUBBLES 7

#include <sys/stat.h>
#include "log.h"
#define LOG_COMPONENT "Nathan"
//Note: LOGI doesn't yet transfer to the test suite
//		Can I make it work like cout and use defines to switch between them?

#endif
