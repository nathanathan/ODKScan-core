#ifndef FORM_ALIGNMENT_H
#define FORM_ALIGNMENT_H

#include <opencv2/core/core.hpp>

cv::Mat quadToTransformation(const std::vector<cv::Point>& foundCorners, const cv::Size& out_image_sz);
std::vector<cv::Point> transformationToQuad(const cv::Mat& H, const cv::Size& out_image_sz);

bool alignFormImage(const cv::Mat& img, cv::Mat& aligned_image, const std::string& dataPath, 
					const cv::Size& aligned_image_sz, float efficiencyScale = .25 );
					
std::vector<cv::Point> findFormQuad(const cv::Mat& img);
std::vector<cv::Point> findBoundedRegionQuad(const cv::Mat& img);
std::vector<cv::Point> findSegment(const cv::Mat& img, float buffer);
#endif
