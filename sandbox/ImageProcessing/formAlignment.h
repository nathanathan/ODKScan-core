#ifndef FORM_ALIGNMENT_H
#define FORM_ALIGNMENT_H

#ifndef TEST_SUITE_H
#include <opencv2/core/core.hpp>
#else
#include "cv.h"
#endif

void align_image(cv::Mat& img, cv::Mat& aligned_image, float thresh_seed);
void straightenImage(const cv::Mat& input_image, cv::Mat& output_image);

#endif
