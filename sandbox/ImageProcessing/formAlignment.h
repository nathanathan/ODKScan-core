#ifndef FORM_ALIGNMENT_H
#define FORM_ALIGNMENT_H

#include "cv.h"

// how wide is the segment in pixels
#define SEGMENT_WIDTH 144
// how tall is the segment in pixels
#define SEGMENT_HEIGHT 200

#define SCALEPARAM 0.55

// buffer around segment in pixels
#define SEGMENT_BUFFER 70

string get_unique_name(string prefix);
void align_image(cv::Mat& img, cv::Mat& aligned_image, float thresh_seed);
void straightenImage(const cv::Mat& input_image, cv::Mat& output_image);

#endif
