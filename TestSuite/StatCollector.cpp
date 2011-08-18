#include "StatCollector.h"
#include "Addons.h"
#include <iostream>

#define DEBUG 0
//#define PRINT_MISS_LOCATIONS

using namespace std;

bool isImage(const std::string& filename){
	return filename.find(".jpg") != std::string::npos;
}

//Compares 2 segments
//returns false if the found segment wasn't fount
void StatCollector::compareSegments(const Json::Value& foundSeg, const Json::Value& actualSeg){
	#if DEBUG > 0
	cout << "Comparing segments..." << endl;
	#endif
	
	numSegments++;
	if( foundSeg.get("notFound", false).asBool() ) {
		missedSegments++;
		return;
	}
	
	const Json::Value fBubbles = foundSeg["bubbles"];
	const Json::Value aBubbles = actualSeg["bubbles"];
	for( size_t i = 0; i < fBubbles.size(); i++){
		bool found = fBubbles[i]["value"].asBool();
		bool actual = aBubbles[i]["value"].asBool();
		
		if(found && actual){
			tp++;
		}
		else if(found && !actual){
			#ifdef PRINT_MISS_LOCATIONS
			cout << "false positive at:" << endl;
			cout << "\t" << fBubbles[i]["location"][0u].asInt() << ", " << fBubbles[i]["location"][1u].asInt() << endl;
			#endif
			fp++;
		}
		else if(!found && actual){
			#ifdef PRINT_MISS_LOCATIONS
			cout << "false negative at:" << endl;
			cout << "\t" << fBubbles[i]["location"][0u].asInt() << ", " << fBubbles[i]["location"][1u].asInt() << endl;
			#endif
			fn++;
		}
		else{
			tn++;
		}
	}
}
void StatCollector::compareFields(const Json::Value& foundField, const Json::Value& actualField){
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
				compareSegments(fSegments[i], aSegments[j]);
			}
		}
	}
}
void StatCollector::compareFiles(const string& foundPath, const string& actualPath){
	Json::Value foundRoot, actualRoot;
	parseJsonFromFile(foundPath.c_str(), foundRoot);
	parseJsonFromFile(actualPath.c_str(), actualRoot);
	#if DEBUG > 0
	cout << "Files parsed." << endl;
	#endif
	
	const Json::Value fFields = foundRoot["fields"];
	const Json::Value aFields = actualRoot["fields"];
	for( size_t i = 0; i < fFields.size(); i++){
		const int fFieldKey = fFields[i].get("key", -1).asInt();
		for( size_t j = i; j < aFields.size(); j++){
			const int aFieldKey = aFields[j].get("key", -2).asInt();
			if(fFieldKey == aFieldKey) {
				compareFields(fFields[i], aFields[j]);
			}
		}
	}
}
void StatCollector::printData(){

	float porportionSuccessfulForms;
	float porportionSuccessfulSegments;
	float porportionCorrectClassifications;

	cout << endl << "________________________________________________________" << endl << endl;
	if(numImages > 0){
		cout << "Errors: " << errors << endl;
		cout << "Images Tested: " << numImages << endl;
		porportionSuccessfulForms = (numImages - errors) / numImages;
		cout << "Percent Success: " << 100.f * porportionSuccessfulForms << "%" << endl;
		cout << "Segment alignment stats for successful form alignments: "<< endl;
		if(numSegments > 0){
			cout << "\tMissed Segments: " << missedSegments << endl;
			cout << "\tSegments Attempted: " << numSegments << endl;
			porportionSuccessfulSegments = (numSegments - missedSegments) / numSegments;
			cout << "\tPercent Success: " << 100.f * porportionSuccessfulSegments << "%" << endl;
			cout << "\tBubble classification stats for successful tests alignments: "<< endl;
		}
	}
	else{
		cout << "\t\tBubble classification stats: "<< endl;
	}
	cout << "\t\tTrue positives: "<< tp << endl;
	cout << "\t\tFalse positives: " << fp << endl;
	cout << "\t\tTrue negatives: "<< tn << endl;
	cout << "\t\tFalse negatives: " << fn << endl;
	porportionCorrectClassifications = (tp + tn) / (tp+fp+tn+fn);
	cout << "\t\tPercent Correct: " << 100.f * porportionCorrectClassifications << "%" << endl;
	
	if(numImages > 0){
		cout << "Total success rate: " << 100.f * (tp + tn) * (numImages - errors) / ((tp+fp+tn+fn) * numImages) << "%" << endl;
	}
	cout << "________________________________________________________" << endl;
}
