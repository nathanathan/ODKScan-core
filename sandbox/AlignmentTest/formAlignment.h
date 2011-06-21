#ifndef FORM_ALIGNMENT_H
#define FORM_ALIGNMENT_H

#include "cv.h"

void align_image(cv::Mat& img, cv::Mat& aligned_image, float thresh_seed, float approx_p);
void straightenImage(const cv::Mat& input_image, cv::Mat& output_image);

#endif
