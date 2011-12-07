#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "Addons.h"
#include "AlignmentUtils.h"

#include <json/json.h>

#include <iostream>
#include <string>

using namespace std;
using namespace cv;

#define EDIT_VIEW


void generateSegment(const Json::Value& segment, Mat& markupImage) {

	//TODO: What happens if points are doubles?
	Point tl(segment["x"].asInt(), segment["y"].asInt());

	rectangle(markupImage, tl, tl + Point(segment["width"].asInt(), segment["height"].asInt()),
	          Scalar::all(0), 2);

	const Json::Value bubble_locations = segment["bubble_locations"];
	for ( size_t k = 0; k < bubble_locations.size(); k++ ) {
		Point bubbleLocation(jsonToPoint(bubble_locations[k]));
		//Not quite sure what the best way to deal with padding will be.
		Point padding(4,4);
		Point classifer_size = Point(jsonToPoint(segment["classifier_size"])) - padding;

		cout << classifer_size << endl;
		#ifdef EDIT_VIEW
			circle(markupImage, tl + bubbleLocation, 3, Scalar(0, 255, 20), -1);
		#endif
		rectangle(markupImage, tl + bubbleLocation - .5 * classifer_size,
		                       tl + bubbleLocation + .5 * classifer_size, Scalar::all(0), 1);
	}
}
void generateField(const Json::Value& field, Mat& markupImage) {
	const Json::Value segments = field["segments"];
		
	for ( size_t j = 0; j < segments.size(); j++ ) {
		Json::Value segment = segments[j];
		inheritMembers(segment, field);
		generateSegment(segment, markupImage);
	}
}

bool generateForm(const char* templatePath, Mat& markupImage) {

	Json::Value templateRoot;
	
	if( !parseJsonFromFile(templatePath, templateRoot) ) return false;

	const Json::Value fields = templateRoot["fields"];
	
	for ( size_t i = 0; i < fields.size(); i++ ) {
		Json::Value field = fields[i];
		inheritMembers(field, templateRoot);
		generateField(field, markupImage);
		
	}
	return true;
}

int main(int argc, char *argv[]) {

	Mat markupImage;

	string formPath("form_images/form_templates/base_fiducial_form.jpg");
	string outPath("form_images/form_templates/checkbox_form.jpg");
	string templatePath("form_images/form_templates/checkbox_form.json");

	const string winName = "editing window";
	namedWindow(winName, CV_WINDOW_NORMAL);
	
	for(;;) {
        	char c = (char)waitKey(0);
		if( c == '\x1b' ) // esc
		{
		    cout << "Exiting ..." << endl;
		    return 0;
		}
		if( c == 'l' )
		{
			markupImage = imread(formPath);
			if(markupImage.empty()) return false;

			if(!generateForm(templatePath.c_str(), markupImage)) return false;
			imshow( winName, markupImage );

		}
		if( c == 's' )
		{
			imwrite(outPath, markupImage);
		}
	}
}

