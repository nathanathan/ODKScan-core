//TODO: renames this since it does more than just marking up forms now.
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/legacy/compat.hpp> //I'm not sure why I need this

#include <json/json.h>

#include <iostream>
#include <fstream>

#include "MarkupForm.h"
#include "FormAlignment.h"
#include "Addons.h"

#define SHOW_MIN_ERROR_CUT

using namespace std;
using namespace cv;

//Indexing is a little bit complicated here.
//The first element of the filledIntegral is always 0.
//A min error cut at 0 means no bubbles are considered filled.
int minErrorCut(const vector<int>& filledIntegral){
	int minErrors = filledIntegral.back();
	int minErrorCutIdx = 0;
	for ( size_t i = 1; i < filledIntegral.size(); i++ ) {
		int errors = (int)i - 2 * filledIntegral[i] + filledIntegral.back();
		if(errors < minErrors){ // saying <= instead would weight things towards more filled bubbles
			minErrors = errors;
			minErrorCutIdx = i;
		}
	}
	return minErrorCutIdx;
}
vector<int> computedFilledIntegral(const Json::Value& field){
	vector<int> filledIntegral(1,0);
	const Json::Value segments = field["segments"];
	for ( size_t i = 0; i < segments.size(); i++ ) {
		const Json::Value segment = segments[i];
		const Json::Value bubbles = segment["bubbles"];
		
		for ( size_t j = 0; j < bubbles.size(); j++ ) {
			const Json::Value bubble = bubbles[j];
			if(bubble["value"].asBool()){
				filledIntegral.push_back(filledIntegral.back()+1);
			}
			else{
				filledIntegral.push_back(filledIntegral.back());
			}
		}
	}
	return filledIntegral;
}
Scalar getColor(bool filled){
	if(filled){
		return Scalar(20, 20, 255);
	}
	else{
		return Scalar(255, 20, 20);
	}
}

//Marks up formImage based on the specifications of a bubble-vals JSON file at the given path.
//Then output the form to the given locaiton.
//Could add functionality for alignment markup to this but is it worth it?
bool markupFormHelper(const char* bvPath, Mat& markupImage) {
	Scalar colors[6] = {Scalar(0,   0,   255),
						Scalar(0,   255, 255),
						Scalar(255, 0,   255),
						Scalar(0,   255, 0),
						Scalar(255, 255, 0),
						Scalar(255, 0,   0)};
	
	Json::Value bvRoot;
	
	if( !parseJsonFromFile(bvPath, bvRoot) ) return false;
	
	const Json::Value fields = bvRoot["fields"];
	for ( size_t i = 0; i < fields.size(); i++ ) {
		const Json::Value field = fields[i];
		
		#ifdef SHOW_MIN_ERROR_CUT
		//Should this be specified in the template??
		int cutIdx = minErrorCut(computedFilledIntegral(field));
		int bubbleNum = 0;
		#endif
		
		string fieldName = field.get("label", "Unlabeled").asString();
		Scalar boxColor = colors[i%6];
		
		const Json::Value segments = field["segments"];
		for ( size_t j = 0; j < segments.size(); j++ ) {
			const Json::Value segment = segments[j];
			if(segment.isMember("quad")){//If we're dealing with a bubble-vals JSON file
				//Draw segment rectangles:
				vector<Point> quad = jsonArrayToQuad(segment["quad"]);
				const Point* p = &quad[0];
				int n = (int) quad.size();
				polylines(markupImage, &p, &n, 1, true, boxColor, 2, CV_AA);
			
				const Json::Value bubbles = segment["bubbles"];
				for ( size_t k = 0; k < bubbles.size(); k++ ) {
					const Json::Value bubble = bubbles[k];
					
					// Draw dots on bubbles colored blue if empty and red if filled
					Point bubbleLocation(jsonToPoint(bubble["location"]));
					
					#ifdef SHOW_MIN_ERROR_CUT
					circle(markupImage, bubbleLocation, 4, 	getColor(bubbleNum < cutIdx), 1, CV_AA);
					bubbleNum++;
					#endif
					
					circle(markupImage, bubbleLocation, 2, 	getColor(bubble["value"].asBool()), 1, CV_AA);
				}
			}
			else{//If we're dealing with a regular form template
				//TODO: Watch out for templates with double type points.
				Point tl(segment["x"].asInt(), segment["y"].asInt());
				rectangle(markupImage, tl, tl + Point(segment["width"].asInt(), segment["height"].asInt()),
							boxColor, 2);
			
				const Json::Value bubble_locations = segment["bubble_locations"];
				for ( size_t k = 0; k < bubble_locations.size(); k++ ) {
					Point bubbleLocation(jsonToPoint(bubble_locations[k]));
					circle(markupImage, tl + bubbleLocation, 3, Scalar(0, 255, 20), -1);
				}
			}
		}
	}
	return true;
}
/*
//Makes a JSON file that contains only the field counts.
bool outputFieldCounts(const char* bubbleVals, const char* outputPath) {

	Json::Value bvRoot;
	
	if( !parseJsonFromFile(bubbleVals, bvRoot) ) return false;
	
	Json::Value fields = bvRoot["fields"];
	for ( size_t i = 0; i < fields.size(); i++ ) {

		Json::Value field = fields[i];
		Json::Value segments = field["segments"];
	
		for ( size_t j = 0; j < segments.size(); j++ ) {
			
			Json::Value segment = segments[j];
			const Json::Value bubbles = segment["bubbles"];
			
			for ( size_t k = 0; k < bubbles.size(); k++ ) {
				const Json::Value bubble = bubbles[k];
				
				circle(markupImage, bubbleLocation, 2, 	getColor(bubble["value"].asBool()), 1, CV_AA);
			}
			segment["count"] = X;
			segment["bubbles"].remove();

		}
	}
	
	ofstream outfile(outputPath, ios::out | ios::binary);
	outfile << bvRoot;
	outfile.close();
	return true;
}*/
bool MarkupForm::markupForm(const char* markupPath, const char* formPath, const char* outputPath) {
	Mat markupImage = imread(formPath);
	if(markupImage.empty()) return false;
	if(!markupFormHelper(markupPath, markupImage)) return false;
	return imwrite(outputPath, markupImage);
}
