#include "StatCollector.h"
#include "Addons.h"
#include <iostream>

#define DEBUG 0
//#define PRINT_MISS_LOCATIONS

using namespace std;

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
void StatCollector::print(ostream& myOut) const{

	float porportionSuccessfulForms;
	float porportionSuccessfulSegments;
	float porportionCorrectClassifications;

	const string linebreak = "________________________________________________________";

	myOut << linebreak << endl << endl;
	
	if(numImages > 0){
		myOut << "Form alignment stats: "<< endl;
		myOut << "Errors: " << errors << endl;
		myOut << "Images Tested: " << numImages << endl;
		porportionSuccessfulForms =  1.f * (numImages - errors) / numImages;
		myOut << "Percent Success: " << 100.f * porportionSuccessfulForms << "%" << endl;
		
		if(numSegments > 0){
			myOut << "\tSegment alignment stats for successful form alignments: "<< endl;
			myOut << "\tMissed Segments: " << missedSegments << endl;
			myOut << "\tSegments Attempted: " << numSegments << endl;
			porportionSuccessfulSegments = 1.f * (numSegments - missedSegments) / numSegments;
			myOut << "\tPercent Success: " << 100.f * porportionSuccessfulSegments << "%" << endl;
		}
	}
	else{
		myOut << "\t\tBubble classification stats: "<< endl;
	}
	
	myOut << "\t\tBubble classification stats for successful segment alignments: "<< endl;
	myOut << "\t\tTrue positives: "<< tp << endl;
	myOut << "\t\tFalse positives: " << fp << endl;
	myOut << "\t\tTrue negatives: "<< tn << endl;
	myOut << "\t\tFalse negatives: " << fn << endl;
	porportionCorrectClassifications =  1.f * (tp + tn) / (tp+fp+tn+fn);
	myOut << "\t\tPercent Correct: " << 100.f * porportionCorrectClassifications << "%" << endl;
	
	if(numImages > 0){
		myOut << endl << "Total success rate: " << 100.f *
												  (porportionSuccessfulForms ? : 1.0) *
												  (porportionSuccessfulSegments ? : 1.0) *
												  porportionCorrectClassifications << "%" << endl;
	}
	myOut << linebreak << endl;
}

ostream& operator<<(ostream& os, const StatCollector& sc){
	sc.print(os);
	return os;
}

