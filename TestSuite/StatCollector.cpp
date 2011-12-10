#include "StatCollector.h"
#include "Addons.h"
#include "TemplateProcessor.h"

#include <iostream>

#include <opencv2/core/core.hpp>

#define DEBUG 0
//#define PRINT_MISS_LOCATIONS

using namespace std;
using namespace cv;

//Compares 2 segments
//returns false if the found segment wasn't fount
void StatCollector::compareSegmentBubbleVals(const Json::Value& foundSeg, const Json::Value& actualSeg){
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
vector<Point> StatCollector::compareSegmentBubbleOffsets(const Json::Value& foundSeg, const Json::Value& actualSeg) const{
	#if DEBUG > 0
	cout << "Comparing segment bubble offsets..." << endl;
	#endif
	vector<Point> out;
	if( foundSeg.get("notFound", false).asBool() ) {
		return out;
	}
	
	Point segmentOffset(actualSeg.get("x", -1).asInt(),
						actualSeg.get("y", -1).asInt());
	
	const Json::Value fBubbles = foundSeg["bubbles"];
	const Json::Value expectedLocations = actualSeg["bubble_locations"];
	for( size_t i = 0; i < fBubbles.size(); i++){
		Point expectedLocation = segmentOffset + jsonToPoint(expectedLocations[i]);
		Point actualLocation = jsonToPoint(fBubbles[i]["location"]);
		out.push_back(expectedLocation - actualLocation);
	}
	return out;
}
void StatCollector::compareFields(const Json::Value& foundField, const Json::Value& actualField, ComparisonMode mode){
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
				if(mode == COMP_BUBBLE_VALS){
					compareSegmentBubbleVals(fSegments[i], aSegments[j]);
				}
				else{
					vector<Point> segmentOffsets = compareSegmentBubbleOffsets(fSegments[i], aSegments[j]);
					offsets.insert(offsets.end(), segmentOffsets.begin(), segmentOffsets.end());
				}
			}
		}
	}
}
void StatCollector::compareFiles(const string& foundPath, const string& actualPath, ComparisonMode mode){
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
				compareFields(fFields[i], aFields[j], mode);
			}
		}
	}
}
double vecSum(vector <double> v){
	double sum = 0;
	for(size_t i = 0; i < v.size(); i++){
		sum += v[i];
	}
	return sum;
}
void StatCollector::print(ostream& myOut) const{

	myOut << linebreak << endl << endl;
	
	if(numImages > 0){
		myOut << "Form alignment stats: "<< endl;
		myOut << "Errors: " << errors << endl;
		myOut << "Images Tested: " << numImages << endl;
		myOut << "Percent Success: " << 100.f * formAlignmentRatio() << "%" << endl;
		if(!offsets.empty()){
			myOut << "Accuracy\n(Differences of found bubble positions from expected bubble postions): " << endl;
			Scalar mean, stddev;
			meanStdDev(Mat(offsets), mean, stddev);
			myOut << "Mean:" << norm(mean) << "\t\t" << "Std. Deviation:" << norm(stddev) << endl;
		}
		if(numSegments > 0){
			myOut << "\tSegment alignment stats for successful form alignments: "<< endl;
			myOut << "\tMissed Segments: " << missedSegments << endl;
			myOut << "\tSegments Attempted: " << numSegments << endl;
			myOut << "\tPercent Success: " << 100.f * segmentAlignmentRatio() << "%" << endl;
			myOut << "\t\tBubble classification stats for successful segment alignments: "<< endl;
		}
	}
	else{
		myOut << "\t\tBubble classification stats: "<< endl;
	}
	
	myOut << "\t\tTrue positives: "<< tp << endl;
	myOut << "\t\tFalse positives: " << fp << endl;
	myOut << "\t\tTrue negatives: "<< tn << endl;
	myOut << "\t\tFalse negatives: " << fn << endl;
	myOut << "\t\tPercent Correct: " << 100.f * correctClassificationRatio() << "%" << endl;
	
	if(numImages > 0){
		myOut << endl << "Total success rate: " << 100.f *
		                                           (numImages > 0 ? formAlignmentRatio() : 1.0) *
		                                           (numSegments > 0 ? segmentAlignmentRatio() : 1.0) *
		                                           correctClassificationRatio() << "%" << endl;
	}
	myOut << "Average image processing time: "<< vecSum(times) / times.size() << " seconds" << endl;
	myOut << linebreak << endl;
}
void StatCollector::printAsRow(ostream& myOut) const{

	if(numImages <= 0 || offsets.empty() || numSegments <= 0){
		myOut << "error" << endl;
		return;
	}
	//Form Alignment
	myOut << numImages - errors << ", " << errors << ", " << formAlignmentRatio() << ", ";
	
	//Segment Alignment
	myOut << numSegments - missedSegments << ", " << missedSegments << ", " << segmentAlignmentRatio() << ", ";

	//Bubble Classification
	myOut << fn << ", " << fp << ", " << tn << ", " << tp << ", " << correctClassificationRatio();
	myOut << endl;
}
ostream& operator<<(ostream& os, const StatCollector& sc){
	sc.print(os);
	return os;
}
