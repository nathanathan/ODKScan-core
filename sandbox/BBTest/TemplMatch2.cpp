/*
Template matching.
Maybe an eigenfaces like approach will work for distinguishing checked from unchecked??
*/
#include <math.h>
#include "cv.h"
#include "cxtypes.h"
#include "highgui.h"

using namespace std;
using namespace cv;

string file_names[] = {"VR0.jpg", "VR1.jpg", "VR2.jpg", "VR3.jpg", "VRform.jpg"};
string tmpl_names[] = {"templ_empty.jpg", "templ_filled.jpg"};

int p0_switch_value = 0;
int p1_switch_value = 0;
int p2_switch_value = 0;
int p3_switch_value = 0;

string param0 = file_names[0];
int param1 = 0;
string param2 = tmpl_names[0];
int param3 = 0;

void switch_callback_p0 ( int position ){
  param0 = file_names[position];
}
void switch_callback_p1 ( int position ){
  param1 = position;
}
void switch_callback_p2 ( int position ){
  param2 = tmpl_names[position];
}
void switch_callback_p3 ( int position ){
  param3 = position;
}

int main(int argc, char* argv[])
{

	const char* name = "Feature Detection Window";

  Mat img_i, img_gray, img_mt, img_th, templ_i, templ, out;
  vector < Vec3f > bubbles;

	// Make window
	namedWindow( name, CV_WINDOW_NORMAL);

	// Create trackbars
	//image
	cvCreateTrackbar( "Param 0", name, &p0_switch_value, 4, switch_callback_p0 );
	//template size
	//cvCreateTrackbar( "Param 1", name, &p1_switch_value, 100, switch_callback_p1 );
	//template type
	cvCreateTrackbar( "Param 2", name, &p2_switch_value, 1, switch_callback_p2 );
	//Threshold
	cvCreateTrackbar( "Param 3", name, &p3_switch_value, 100, switch_callback_p3 );
	
	string param0_pv, param2_pv;
	int param1_pv;
	
	// Loop for dynamicly altering parameters in window
	while( 1 ) {
	  // Allows exit via Esc
		if( cvWaitKey( 15 ) == 27 ) 
			break;
			
	  if (param0 != param0_pv){
	    // Read the input image
    	img_i = imread(param0);
	    if (img_i.data == NULL) {
		    return false;
	    }
	    cvtColor(img_i, img_gray, CV_RGB2GRAY);
	    param1_pv, param2_pv = -1;
	    param0_pv = param0;
	    
    }
    if (param2 != param2_pv){
      templ_i = imread(param2);
	    if (templ_i.data == NULL) {
		    return false;
	    }
	    cvtColor(templ_i, templ, CV_RGB2GRAY);
      matchTemplate(img_gray, templ, img_mt, CV_TM_CCOEFF_NORMED);
      param1_pv = param1;
      param2_pv = param2;
    }

    img_th = img_mt > double(param3)/double(100);
    out = img_i.clone();
    vector<vector<Point> > contours;
    findContours(img_th, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

    for(std::vector<vector<Point> >::iterator it = contours.begin(); it != contours.end(); ++it) {
      Moments m = moments(Mat(*it));
      circle(out, Point(m.m10/m.m00 + templ.cols/2, m.m01/m.m00+ templ.rows/2) , 3, Scalar(0,255,0), 1, CV_AA);
    }
    
    //drawContours(out, contours, -1, Scalar(84,200,55) , 3);

    imshow(name, out);
	}

	cvDestroyWindow( name );

	return 0;
}
