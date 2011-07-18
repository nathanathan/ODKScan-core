#ifndef FORM_ALIGNMENT_H
#define FORM_ALIGNMENT_H

#include <opencv2/core/core.hpp>

void alignImage(cv::Mat& img, cv::Mat& aligned_image, std::vector<cv::Point>& maxRect, cv::Size aligned_image_sz);

cv::Mat getMyTransform(std::vector<cv::Point>& foundCorners, cv::Size init_image_sz,
						cv::Size out_image_sz, bool reverse = false);
std::vector<cv::Point> transformationToQuad(const cv::Mat& H, const cv::Size& out_image_sz);

bool alignFormImage(cv::Mat& img, cv::Mat& aligned_image, const std::string& dataPath, 
					const cv::Size& aligned_image_sz, float efficiencyScale = .25 );
					
std::vector<cv::Point> findFormQuad(cv::Mat& img);
std::vector<cv::Point> findBoundedRegionQuad(cv::Mat& img);

#endif
