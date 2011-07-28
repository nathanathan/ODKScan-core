/*
Addons is a bunch of code that isn't really program specific and get's reused in multiple places.
It's a kind of random mix of stuff and it if grows a lot should probably be split up.
*/
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
//JSON/OpenCV oriented functions:
Json::Value pointToJson(const cv::Point p);
cv::Point jsonToPoint(const Json::Value& jPoint);
Json::Value quadToJsonArray(std::vector<cv::Point>& quad, cv::Point offset);
std::vector<cv::Point> jsonArrayToQuad(const Json::Value& quadJson);
//Pure Json:
bool parseJsonFromFile(const char* filePath, Json::Value& myRoot);
bool parseJsonFromFile(const std::string& filePath, Json::Value& myRoot);
//Misc:
template <class Tp>
bool returnTrue(Tp& anything){
	return true;
}
#endif
