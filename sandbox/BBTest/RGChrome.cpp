#include "cv.h"
#include "cxtypes.h"
#include "highgui.h"

using namespace std;
using namespace cv;

string file_names[] = {"test1.jpg", "test2.jpg", "test3.jpg", "test4.jpg"};

int p0_switch_value = 0;
int p1_switch_value = 0;

string param0 = file_names[0];
int param1 = 0;


void switch_callback_p0 ( int position ){
  param0 = file_names[position];
}
void switch_callback_p1 ( int position ){
  param1 = position;
}

int main(int argc, char* argv[])
{

	const char* name = "Feature Detection Window";

  Mat img_i, component_sum, out;
  vector<Mat> components;

	// Make window
	namedWindow( name, CV_WINDOW_NORMAL);

	// Create trackbars
	//choose image
	cvCreateTrackbar( "Param 0", name, &p0_switch_value, 3, switch_callback_p0 );
	//permute the channels
	cvCreateTrackbar( "Param 1", name, &p1_switch_value, 2, switch_callback_p1 );
	
	string param0_pv;

	// Loop for dynamicly altering parameters in window
	while( 1 ) {
	  if( cvWaitKey( 15 ) == 27 ) 
			break;
	  // Allows exit via Esc
	  if(param0_pv != param0){
    	img_i = imread(param0);
      if (img_i.data == NULL) {
        return 0;
      }
		  param0_pv = param0;
		}
		split(img_i, components);
		component_sum = components[0]+components[1]+components[2];
		divide(components[0], component_sum, components[0], 255);
		divide(components[1], component_sum, components[1], 255);
		divide(components[2], component_sum, components[2], 255);
		int from_to[] = { 0,param1,  1,(param1+1)%3,  2,(param1+2)%3 };
    mixChannels( components, components, from_to, 3 );
		merge(components, out);
		imshow(name, out);
	}

	cvDestroyWindow( name );

	return 0;
}
