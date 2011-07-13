#include "Addons.h"
#include <json/json.h>
#include <fstream>

using namespace std;
using namespace cv;

Size operator * (float lhs, Size rhs) {
	return Size(lhs*rhs.width, lhs*rhs.height);
}
Rect operator * (float lhs, Rect rhs) {
	return Rect(lhs*rhs.tl(), lhs*rhs.size());
}
Rect expandRect(const Rect& r, const float expansionPercentage){
	Point diag = r.tl() - r.br();
	return Rect(expansionPercentage * diag + r.tl(), 
				-expansionPercentage * diag + r.br());
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

bool parseJsonFromFile(const char* filePath, Json::Value& myRoot){
	ifstream JSONin;
	Json::Reader reader;
	
	JSONin.open(filePath, ifstream::in);
	bool parse_successful = reader.parse( JSONin, myRoot );
	
	JSONin.close();
}
