#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "Addons.h"
#include "AlignmentUtils.h"
#include "TemplateProcessor.h"

#include <json/json.h>

#include <iostream>
#include <string>

using namespace std;
using namespace cv;

//#define EDIT_VIEW

class GenerateForm : public TemplateProcessor
{
	private:
	typedef TemplateProcessor super;
	Mat markupImage;

	public:
	GenerateForm(Mat markupImageInit):markupImage(markupImageInit){}
	virtual Json::Value segmentFunction(const Json::Value& segment){
		
		//TODO: What happens if points are doubles?
		Point tl(segment["x"].asInt(), segment["y"].asInt());

		rectangle(markupImage, tl, tl + Point(segment["segment_width"].asInt(), segment["segment_height"].asInt()),
			  Scalar::all(0), 2);

		const Json::Value bubble_locations = segment["bubble_locations"];
		for ( size_t k = 0; k < bubble_locations.size(); k++ ) {
			Point bubbleLocation(jsonToPoint(bubble_locations[k]));
			//Not quite sure what the best way to deal with padding will be.
			Point padding(4,4);
			Point classifer_size = Point(jsonToPoint(segment["classifier_size"])) - padding;

			//cout << classifer_size << endl;
			#ifdef EDIT_VIEW
				circle(markupImage, tl + bubbleLocation, 3, Scalar(0, 255, 20), -1);
			#endif
			if(segment["training_data_uri"].asString() == "bubbles"){
				ellipse(markupImage, tl + bubbleLocation, Size(.5 * classifer_size.x,
				                                               .5 * classifer_size.y ),
				        0, 0, 360, Scalar::all(0), 1, CV_AA);
			}
			else{
				rectangle(markupImage, tl + bubbleLocation - .5 * classifer_size,
						       tl + bubbleLocation + .5 * classifer_size, Scalar::all(0), 1);
			}
		}
		return super::segmentFunction(segment);
	}
	virtual ~GenerateForm(){}
};
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

			//if(!generateForm(templatePath.c_str(), markupImage)) return false;

			GenerateForm g(markupImage);
			if( !g.start(templatePath.c_str()) ) return false;

			imshow( winName, markupImage );

		}
		if( c == 's' )
		{
			imwrite(outPath, markupImage);
		}
	}
}
