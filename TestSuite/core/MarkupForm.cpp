//TODO: renames this since it does more than just marking up forms now.
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/legacy/compat.hpp> //I'm not sure why I need this

#include <json/json.h>

#include <iostream>
#include <fstream>

#include "MarkupForm.h"
#include "Addons.h"

//#define SHOW_MIN_ERROR_CUT

using namespace std;
using namespace cv;

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
//Makes a JSON file that contains only the fieldnames and their corresponding bubble counts.
bool MarkupForm::outputFieldCounts(const char* bubbleVals, const char* outputPath) {

	Json::Value bvRoot;
	
	if( !parseJsonFromFile(bubbleVals, bvRoot) ) return false;
	
	Json::Value fields = bvRoot["fields"];
	for ( size_t i = 0; i < fields.size(); i++ ) {

		Json::Value field = fields[i];
		Json::Value segments = field["segments"];
		int counter = 0;
		for ( size_t j = 0; j < segments.size(); j++ ) {
			
			Json::Value segment = segments[j];
			const Json::Value bubbles = segment["bubbles"];
			
			for ( size_t k = 0; k < bubbles.size(); k++ ) {
				const Json::Value bubble = bubbles[k];
				if(bubble["value"].asBool()){
					counter++;
				}
			}
		}
		field["count"] = counter;
		field.removeMember("segments");
	}
	
	ofstream outfile(outputPath, ios::out | ios::binary);
	outfile << bvRoot;
	outfile.close();
	return true;
}
//Marks up formImage based on the specifications of a bubble-vals JSON file at the given path.
//Then output the form to the given locaiton.
bool MarkupForm::markupForm(const char* markupPath, const char* formPath, const char* outputPath) {
	Mat markupImage = imread(formPath);
	if(markupImage.empty()) return false;
	if(!markupFormHelper(markupPath, markupImage)) return false;
	return imwrite(outputPath, markupImage);
}
