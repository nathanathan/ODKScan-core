#ifndef SEGMENT_ALIGNER_H
#define SEGMENT_ALIGNER_H

#include <opencv2/core/core.hpp>

std::vector<cv::Point> findBoundedRegionQuad(const cv::Mat& img, float buffer);
std::vector<cv::Point> findSegment(const cv::Mat& img, float buffer);

#endif
