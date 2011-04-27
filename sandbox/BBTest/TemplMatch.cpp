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

string file_names[] = {"warped.jpg", "warped2.jpg", "VRform.jpg"};

int p0_switch_value = 0;
int p1_switch_value = 0;
int p2_switch_value = 0;
int p3_switch_value = 0;
int p4_switch_value = 0;

string param0 = file_names[0];
int param1 = 0;
int param2 = 0;
int param3 = 0;
int param4 = 0;

void switch_callback_p0 ( int position ){
  param0 = file_names[position];
}
void switch_callback_p1 ( int position ){
  param1 = position;
}
void switch_callback_p2 ( int position ){
  param2 = position;
}
void switch_callback_p3 ( int position ){
  param3 = position;
}
void switch_callback_p4 ( int position ){
  param4 = position;
}
//rounds integers up to the next odd integer
int next_odd(int n){
  return (n/2)*2+1;
}

int main(int argc, char* argv[])
{

	const char* name = "Feature Detection Window";

  Mat img_i, img_gray, img_mt, img_th, templ, out;
  vector < Vec3f > bubbles;

	// Make window
	namedWindow( name, CV_WINDOW_NORMAL);

	// Create trackbars
	//image
	cvCreateTrackbar( "Param 0", name, &p0_switch_value, 2, switch_callback_p0 );
	//template size
	cvCreateTrackbar( "Param 1", name, &p1_switch_value, 100, switch_callback_p1 );
	//0 for solid filled circles, anything above for empty cirlces with variable border size
	cvCreateTrackbar( "Param 2", name, &p2_switch_value, 100, switch_callback_p2 );
	//Threshold
	cvCreateTrackbar( "Param 3", name, &p3_switch_value, 100, switch_callback_p3 );
	//cvCreateTrackbar( "Param 4", name, &p4_switch_value, 100, switch_callback_p4 );
	
	string param0_pv;
	int param1_pv, param2_pv;
	
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
    int templ_size;
    if (param1 != param1_pv || param2 != param2_pv){
  	  int bubble_rad = param1 + 2;
	    templ_size = 2 * bubble_rad + next_odd((param2  - 1)) + 6;
      templ = Mat(templ_size, templ_size, CV_8UC1, Scalar(255,255,255));
	    circle(templ, Point(templ_size/2, templ_size/2), bubble_rad, Scalar(0,0,0), (param2 - 1));
	    GaussianBlur(templ, templ, Size(next_odd(int(sqrt(bubble_rad))), next_odd(int(sqrt(bubble_rad)))), 0);
      matchTemplate(img_gray, templ, img_mt, CV_TM_CCOEFF_NORMED);
      param1_pv = param1;
      param2_pv = param2;
    }
    //threshold(img_mt, img_th, double(param3)/100, 255, THRESH_BINARY);
    img_th = img_mt > double(param3)/double(100);
    //findContours(img_th, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
    out = img_i.clone();
    vector<vector<Point> > contours;
    findContours(img_th, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

    for(std::vector<vector<Point> >::iterator it = contours.begin(); it != contours.end(); ++it) {
      Moments m = moments(Mat(*it));
      circle(out, Point(m.m10/m.m00 + templ_size/2, m.m01/m.m00+ templ_size/2) , 16, Scalar(0,255,0), 2, 8);
    }
 
    
    //drawContours(out, contours, -1, Scalar(84,200,55) , 3);

    imshow(name, out);
	}

	cvDestroyWindow( name );

	return 0;
}
