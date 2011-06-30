#ifndef FORM_ALIGNMENT_H
#define FORM_ALIGNMENT_H

#include "configuration.h"

#ifndef TEST_SUITE_H
#include <opencv2/core/core.hpp>
#else
#include "cv.h"
#endif

void align_image(cv::Mat& img, cv::Mat& aligned_image, cv::Size aligned_image_sz, int blurSize = 2);
void straightenImage(const cv::Mat& input_image, cv::Mat& output_image);

#endif
