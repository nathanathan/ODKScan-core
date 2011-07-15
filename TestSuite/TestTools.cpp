#include "TestTools.h"
#include "Addons.h"
#include <json/json.h>
#include <iostream>

#define DEBUG 0

using namespace std;

void compareSegments(const Json::Value& foundSeg, const Json::Value& actualSeg, int& tp, int& fp, int& tn, int& fn){
	#if DEBUG > 0
	cout << "Comparing segments..." << endl;
	#endif
	const Json::Value fBubbles = foundSeg["bubbles"];
	const Json::Value aBubbles = foundSeg["bubbles"];
	size_t i = 0;
	for( size_t i = 0; i < fBubbles.size(); i++){
		bool found = fBubbles[i]["value"].asBool();
		bool actual = aBubbles[i]["value"].asBool();
		if(found && actual){
			tp++;
		}
		else if(found && !actual){
			fp++;
		}
		else if(!found && actual){
			fn++;
		}
		else{
			tn++;
		}
	}
}
void compareFields(const Json::Value& foundField, const Json::Value& actualField, int& tp, int& fp, int& tn, int& fn){
	#if DEBUG > 0
	cout << "Comparing fields..." << endl;
	#endif
	const Json::Value fSegments = foundField["segments"];
	const Json::Value aSegments = actualField["segments"];
	for( size_t i = 0; i < fSegments.size(); i++){
		const int fKey = fSegments[i].get("key", -47).asInt();
		for( size_t j = i; j < aSegments.size(); j++){
			const int aKey = aSegments[j].get("key", -62).asInt();
			if(fKey == aKey) {
				compareSegments(fSegments[i], aSegments[j], tp, fp, tn, fn);
			}
		}
	}
}
void compareFiles(const string& foundPath, const string& actualPath, int& tp, int& fp, int& tn, int& fn){
	Json::Value foundRoot, actualRoot;
	parseJsonFromFile(foundPath.c_str(), foundRoot);
	parseJsonFromFile(actualPath.c_str(), actualRoot);
	#if DEBUG > 0
	cout << "Files parsed." << endl;
	#endif
	
	const Json::Value fFields = foundRoot["fields"];
	const Json::Value aFields = actualRoot["fields"];
	for( size_t i = 0; i < fFields.size(); i++){
		const string fLabel = fFields[i].get("label", "parser breaking label 1").asString();
		for( size_t j = i; j < aFields.size(); j++){
			const string aLabel = aFields[j].get("label", "parser breaking label 2").asString();
			if(fLabel.compare(aLabel) == 0) {
				compareFields(fFields[i], aFields[j], tp, fp, tn, fn);
			}
		}
	}
}
