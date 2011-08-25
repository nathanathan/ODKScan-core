#include "MarkupForm.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <string>

using namespace std;
using namespace cv;

bool markupForm(const char* formPath, const char* markupPath, Mat& markupImage) {
	
	if(!MarkupForm::markupForm(markupPath, formPath, "templateEditor_temp.jpg")) return false;
	markupImage = imread("templateEditor_temp.jpg");
	return !markupImage.empty();
}

int main(int argc, char *argv[]) {

	Mat markupImage;

	string formPath("form_templates/SIS-A01.jpg");
	string templatePath("form_templates/SIS-A01.json");

	const string winName = "editing window";
	namedWindow(winName, CV_WINDOW_NORMAL);
	
	for(;;)
    {
        char c = (char)waitKey(0);
        if( c == '\x1b' ) // esc
        {
            cout << "Exiting ..." << endl;
            return 0;
        }
        if( c == 'l' )
        {
			if( !markupForm(formPath.c_str(), templatePath.c_str(), markupImage) ) return false;
			imshow( winName, markupImage );

        }
    }
}

