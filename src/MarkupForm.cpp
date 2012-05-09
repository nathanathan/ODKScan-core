//TODO: renames this since it does more than just marking up forms now.
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/legacy/compat.hpp> //I'm not sure if I need this

#include <json/json.h>

#include <iostream>
#include <fstream>

#include "MarkupForm.h"
#include "Addons.h"
#include "AlignmentUtils.h"
#include "TemplateProcessor.h"

//#define SHOW_MIN_ERROR_CUT

using namespace std;
using namespace cv;

//TODO: Simplify this using the template processor class
//	Or maybe it's not worth the effort since we might go to marking up photos with JSON
/*
class MarkupForm : public TemplateProcessor
{
	private:
	typedef TemplateProcessor super;

	Json::Value segmentFunction(const Json::Value& segment);
	Json::Value fieldFunction(const Json::Value& field);
	Json::Value formFunction(const Json::Value& templateRoot);
	bool start(const char* templatePath);

};
*/

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
					if(classification.asBool()){
						filledItems++;
					}
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
		if(field.get("type", "input") == "input"){
			//Print the counts on the form:
			if(drawCounts && avgWidth > 0){
				avgWidth /= segments.size();
				avgY /= segments.size();
			
			
				Point textBoxTL(endOfField + 5, (int)avgY - 5);
				if(field.isMember("print_count_location")){
					textBoxTL = jsonToPoint(field["print_count_location"]);
				}
			
				stringstream ss;
				ss << filledItems;
				putText(markupImage, ss.str(), textBoxTL,  FONT_HERSHEY_SIMPLEX, 1., Scalar::all(0), 3, CV_AA);
				putText(markupImage, ss.str(), textBoxTL,  FONT_HERSHEY_SIMPLEX, 1., boxColor, 2, CV_AA);
			}
		}
		
	}
	return true;
}
/*
//DEPRECATED?
//Makes a JSON file that contains only the fieldnames and their corresponding Item counts.
bool MarkupForm::outputFieldCounts(const char* ItemVals, const char* outputPath){
	return false; //I don't think this gets used.
	Json::Value bvRoot;
	
	if( !parseJsonFromFile(ItemVals, bvRoot) ) return false;
	
	Json::Value fields = bvRoot["fields"];
	for ( size_t i = 0; i < fields.size(); i++ ) {

		Json::Value field = fields[i];
		Json::Value segments = field["segments"];
		int counter = 0;
		for ( size_t j = 0; j < segments.size(); j++ ) {
			
			Json::Value segment = segments[j];
			const Json::Value Items = segment["Items"];
			
			for ( size_t k = 0; k < Items.size(); k++ ) {
				const Json::Value Item = Items[k];
				if(Item["value"].asInt()){
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
*/
//Marks up formImage based on the specifications of a Item-vals JSON file at the given path.
//Then output the form to the given locaiton.
bool MarkupForm::markupForm(const char* markupPath, const char* formPath, const char* outputPath, bool drawCounts){
	Mat markupImage = imread(formPath);
	if(markupImage.empty()) return false;
	if(!markupFormHelper(markupPath, markupImage, drawCounts)) return false;
	return imwrite(outputPath, markupImage);
}
