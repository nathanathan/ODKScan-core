#ifndef ADDONS_H
#define ADDONS_H
#include "configuration.h"

#ifdef USE_ANDROID_HEADERS_AND_IO
#include <opencv2/core/core.hpp>
#else
#include "cv.h"
#endif

#include <json/value.h>

//OpenCV oriented functions:
cv::Size operator * (float lhs, cv::Size rhs);
cv::Rect operator * (float lhs, cv::Rect rhs);
cv::Rect expandRect(const cv::Rect& r, const float expansionPercentage);
//JSON oriented functions:
Json::Value pointToJson(const cv::Point p);
cv::Point jsonToPoint(const Json::Value& jPoint);
Json::Value quadToJsonArray(std::vector<cv::Point>& quad, cv::Point offset);
std::vector<cv::Point> jsonArrayToQuad(const Json::Value& quadJson);
//Misc
template <class Tp>
bool returnTrue(Tp& anything){
	return true;
}
#endif
