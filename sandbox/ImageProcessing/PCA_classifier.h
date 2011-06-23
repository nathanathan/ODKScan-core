#ifndef PCA_CLASSIFIER_H
#define PCA_CLASSIFIER_H

#include "cv.h"

#if 0
#define EXAMPLE_WIDTH 28
#define EXAMPLE_HEIGHT 36
#else
#define EXAMPLE_WIDTH 14
#define EXAMPLE_HEIGHT 18
#endif

enum bubble_val { EMPTY_BUBBLE = 0, PARTIAL_BUBBLE, FILLED_BUBBLE, NUM_BUBBLE_VALS };
//since this enum and the EXAMPLE_ size constants get used pretty much everywhere
//maybe they should be in a different header?

void set_weight(bubble_val classification, float weight);
void set_search_window(cv::Point sw);

bool returnTrue(string& filename);
void train_PCA_classifier(bool (*pred)(string& filename) = &returnTrue);
double rateBubble(cv::Mat& det_img_gray, cv::Point bubble_location);
cv::Point bubble_align(cv::Mat& det_img_gray, cv::Point bubble_location);
bubble_val classifyBubble(cv::Mat& det_img_gray, cv::Point bubble_location);


#endif
