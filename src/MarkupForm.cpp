#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <json/json.h>

#include <iostream>
#include <fstream>

#include "MarkupForm.h"
#include "Addons.h"
#include "AlignmentUtils.h"
#include "TemplateProcessor.h"

using namespace std;
using namespace cv;

bool markupFormHelper(const char* bvPath, Mat& markupImage, bool drawCounts) {	
	Json::Value bvRoot;
	
	if( !parseJsonFromFile(bvPath, bvRoot) ) return false;
	
	const Json::Value fields = bvRoot["fields"];
	for ( size_t i = 0; i < fields.size(); i++ ) {
		const Json::Value field = fields[i];
		
		//TODO: For multi-choice forms were going to want something a little different.
		//	This function is getting pretty complicated and should probably be split up.
		size_t filledItems = 0;
		float avgWidth = 0;
		float avgY = 0;
		int endOfField = 0;
		
		#ifdef SHOW_MIN_ERROR_CUT
			//Should this be specified in the template??
			int cutIdx = minErrorCut(computedFilledIntegral(field));
			int ItemNum = 0;
		#endif
		
		string fieldName = field.get("label", "Unlabeled").asString();
		Scalar boxColor = getColor(int(i));
	
		const Json::Value segments = field["segments"];
		
		for ( size_t j = 0; j < segments.size(); j++ ) {
			const Json::Value segment = segments[j];

			//TODO: unaligned segments could be an issue

			//Draw segment rectangles:
			vector<Point> quad = orderCorners( jsonArrayToQuad(segment["quad"]) );
			const Point* p = &quad[0];
			int n = (int) quad.size();
			polylines(markupImage, &p, &n, 1, true, boxColor, 2, CV_AA);
			
			if( segment.get("notFound", false).asBool() ) {
				polylines(markupImage, &p, &n, 1, true, .25 * boxColor, 2, CV_AA);
			}
			else{
				polylines(markupImage, &p, &n, 1, true, boxColor, 2, CV_AA);
			}
			
			//Compute some stuff to figure out where to draw output on the form
			avgWidth += norm(quad[0] - quad[1]);
			if(endOfField < quad[1].x){
				endOfField = quad[1].x;
			}
			avgY += quad[3].y;// + quad[1].y + quad[2].y + quad[3].y) / 4;
			

			const Json::Value items = segment["items"];
			for ( size_t k = 0; k < items.size(); k++ ) {
				const Json::Value Item = items[k];
				Point ItemLocation(jsonToPoint(Item["absolute_location"]));
				Json::Value classification = Item["classification"];

				if(classification.isBool()){
					circle(markupImage, ItemLocation, 2, 	getColor(classification.asBool()), 1, CV_AA);
				}
				else if(classification.isInt()){
					circle(markupImage, ItemLocation, 2, 	getColor(classification.asInt()), 1, CV_AA);
				}
				else{
					cout << "Don't know what this is" << endl;
				}
				
			}
		}
		if(field.isMember("value")){
			Point textBoxTL;
			if(drawCounts && avgWidth > 0){
				avgWidth /= segments.size();
				avgY /= segments.size();
				textBoxTL = Point(endOfField + 5, (int)avgY - 5);
			}
			if(field.isMember("markup_location")){
				textBoxTL = Point(field["markup_location"]["x"].asInt(), field["markup_location"]["y"].asInt());
			}
			stringstream ss;
			ss << field.get("value", "");
			putText(markupImage, ss.str(), textBoxTL,
			        FONT_HERSHEY_SIMPLEX, 1., Scalar::all(0), 3, CV_AA);
			putText(markupImage, ss.str(), textBoxTL,
			        FONT_HERSHEY_SIMPLEX, 1., boxColor, 2, CV_AA);
		}
		
	}
	return true;
}
//Marks up formImage based on the specifications of a Item-vals JSON file at the given path.
//Then output the form to the given locaiton.
bool MarkupForm::markupForm(const char* markupPath, const char* formPath, const char* outputPath, bool drawCounts){
	Mat markupImage = imread(formPath);
	if(markupImage.empty()) return false;
	if(!markupFormHelper(markupPath, markupImage, drawCounts)) return false;
	return imwrite(outputPath, markupImage);
}
