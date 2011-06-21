/*
A program for demonstrating various for Bubble Bot related algorithms.
*/
#include "cv.h"
#include "highgui.h"
#include "formAlignment.h"

#define SCALEPARAM 0.55

using namespace std;
using namespace cv;

string file_names[] = {"booklet_form.jpg", "misaligned_segment.jpg"};
int p0_switch_value = 0;
string param0 = file_names[0];

int p2_switch_value = 0;
float thresh_p = 0.0;

int p3_switch_value = 0;
int gauss_p = 1;
void switch_callback_p3 ( int position ){
	gauss_p = 1+position;
}

void switch_callback_p0 ( int position ){
	param0 = file_names[position];
}
void switch_callback_p2 ( int position ){
	thresh_p = .1 * float(position);
}

int main(int argc, char* argv[]) {

	const char* name = "Alignment Window";
	Mat img;

	// Make window
	namedWindow( name, CV_WINDOW_NORMAL);

	// Create trackbars
	cvCreateTrackbar( "Image", name, &p0_switch_value, 1, switch_callback_p0 );
	cvCreateTrackbar( "p2", name, &p2_switch_value, 10, switch_callback_p2 );
	cvCreateTrackbar( "p3", name, &p3_switch_value, 10, switch_callback_p3 );

	string param0_pv;
	// Loop for dynamicly altering parameters in window
	while( 1 ) {
		// Allows exit via Esc
		if( cvWaitKey( 15 ) == 27 ) 
			break;

		if (param0 != param0_pv){
			// Read the input image
			img = imread(param0, 0);
			assert(img.data != NULL);
		}
		
		Mat straightened_image(3300 * SCALEPARAM, 2550 * SCALEPARAM, CV_8U);
		
		Mat out, out2;
		GaussianBlur(img, out, Size(1 + 2*gauss_p, 1 + 2*gauss_p), 1.0 * float(gauss_p));
		GaussianBlur(out, out2, Size(1 + 4*gauss_p, 1 + 4*gauss_p), 2.0 * float(gauss_p));
		img = out - out2;
		
		align_image(img,  straightened_image, 0, .1*thresh_p);
		
		//straightenImage(img,  straightened_image);
		
		imshow(name, straightened_image);
	}
				
	cvDestroyWindow( name );

	return 0;
}
