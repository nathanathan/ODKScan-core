#ifndef SEGMENT_ALIGNER_H
#define SEGMENT_ALIGNER_H

#include <opencv2/core/core.hpp>
// Tries to find a bounded segment in img, with roi representing the expected location.
// The functions returns a vector of 4 points representing the corners of the segment.
// Returns an empty vector if the segment was not found.
std::vector<cv::Point> findSegment(const cv::Mat& img, const cv::Rect& roi);

#endif
