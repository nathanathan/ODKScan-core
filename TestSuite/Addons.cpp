#include "Addons.h"
using namespace std;
using namespace cv;

Size operator * (float lhs, Size rhs) {
	return Size(lhs*rhs.width, lhs*rhs.height);
}
Rect operator * (float lhs, Rect rhs) {
	return Rect(lhs*rhs.tl(), lhs*rhs.size());
}
Rect expandRect(const Rect& r, const float expansionPercentage){
	Point center = (r.tl() + r.br()) * .5;
	return Rect(expansionPercentage * (r.tl() - center) + r.tl(), 
				expansionPercentage * (r.br() - center) + r.br());
}

Json::Value pointToJson(const Point p){
	Json::Value jPoint;
	jPoint.append(p.x);
	jPoint.append(p.y);
	return jPoint;
}
Point jsonToPoint(const Json::Value& jPoint){
	return Point(jPoint[0u].asInt(), jPoint[1u].asInt());
}
Json::Value quadToJsonArray(vector<Point>& quad, Point offset){
	Json::Value out;
	for(size_t i = 0; i < quad.size(); i++){
		Point myPoint = offset + quad[i];
		out.append(pointToJson(myPoint));
	}
	return out;
}
vector<Point> jsonArrayToQuad(const Json::Value& quadJson){
	vector<Point> out;
	for(size_t i = 0; i < quadJson.size(); i++){
		out.push_back(jsonToPoint(quadJson[i]));
	}
	return out;
}
