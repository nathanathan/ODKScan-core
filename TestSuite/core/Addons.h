/*
Addons is a bunch of code that isn't really program specific and get's reused in multiple places.
It's a kind of random mix of stuff and it if grows a lot should probably be split up.
*/
#ifndef ADDONS_H
#define ADDONS_H
#include "configuration.h"


#include <opencv2/core/core.hpp>


#include <json/value.h>

//OpenCV oriented functions:
cv::Scalar getColor(bool filled);
cv::Size operator * (float lhs, cv::Size rhs);
cv::Rect operator * (float lhs, cv::Rect rhs);
cv::Rect resizeRect(const cv::Rect& r, float amount);
//JSON/OpenCV oriented functions:
Json::Value pointToJson(const cv::Point p);
cv::Point jsonToPoint(const Json::Value& jPoint);
Json::Value quadToJsonArray(std::vector<cv::Point>& quad, cv::Point offset);
std::vector<cv::Point> jsonArrayToQuad(const Json::Value& quadJson);
//Pure Json:
bool parseJsonFromFile(const char* filePath, Json::Value& myRoot);
bool parseJsonFromFile(const std::string& filePath, Json::Value& myRoot);
//Indexing is a little bit complicated here.
//The first element of the filledIntegral is always 0.
//A min error cut at 0 means no bubbles are considered filled.
enum inferenceMethod{INFER_LTR_TTB, INFER_NEIGHBORS};
//TODO: one idea is to infer that bubbles labeled barely are filled if their neighbors are.
void inferBubbles(Json::Value& field, inferenceMethod method);
int minErrorCut(const std::vector<int>& filledIntegral);
std::vector<int> computedFilledIntegral(const Json::Value& field);
//Misc:
std::string replaceFilename(const std::string& filepath, const std::string& newName );
template <class Tp>
bool returnTrue(Tp& anything){
	return true;
}
#endif
