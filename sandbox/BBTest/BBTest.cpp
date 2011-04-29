/*
A program for demonstrating various for Bubble Bot related algorithms.
*/

#include "cv.h"
#include "cxtypes.h"
#include "highgui.h"

using namespace std;
using namespace cv;

int step_switch_value = 0;
int step = 0;//which step of the process to show

int dilation_switch_value = 0;
int init_dilation = 0;
int bf_dilation = 0;

int d_switch_value = 0;
int distParam = 0;
int ap_switch_value = 0;
int block_size = 5;
int thresh_ap = 5;
int save_switch_value = 0;
int save = 0;

string file_names[] = {"VR_ns0.jpg", "VR_ns1.jpg", "VR_ns2.jpg"};
int p0_switch_value = 0;
string param0 = file_names[0];
void switch_callback_p0 ( int position ){
  param0 = file_names[position];
}

void switch_callback_save( int position ){
  save = 1;
}

void switch_callback_step( int position ){
	step = position;
}
/*
Depending on the step some switches will controll parameters of different functions
*/
void switch_callback_di( int position ){
  if( step < 3 ){
    init_dilation = position * 4;
	}
	else{
	  bf_dilation = position;
  }
}
void switch_callback_d( int position ){
	distParam = position * 6;
}
void switch_callback_a( int position ){
  int t;
  switch( position ){
    case 0:
			t = 5;
			break;
		case 1:
			t = 7;
			break;
		case 2:
			t = 9;
			break;
	  case 3:
			t = 11;
			break;
	  case 4:
			t = 13;
			break;
	}
	if( step < 3 ){
	  block_size = t;
  }
  else{
    thresh_ap = t;
  }
}

/*
Expects greyscale image
Returns bubble locations and radius
*/
vector< Vec3f > findBubbles(Mat& origImage) {
	vector < Vec3f > circles;
	Mat pImage;
	
	GaussianBlur(origImage, pImage, Size(3, 3), 2, 2);
	dilate(pImage, pImage, Mat(), Point(-1, -1), bf_dilation);
  adaptiveThreshold(pImage, pImage, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, thresh_ap, -1.5);
  
  /*
  HoughCircles Parameters:
  Source Mat, Dest Mat, CV_HOUGH_GRADIENT
  Resolution (makes a huge impact on how many cicles it finds, probably needs to vary with image size)
  Min-distance between circles
  Canny high thresh
  Accumulator thresh
  Min circle radius, Max circle radius
  */
	HoughCircles(pImage, circles, CV_HOUGH_GRADIENT, 5, 50, 1000, 80, 15, 22);
  
	return circles;
}

/*
Takes a vector of corners and converts it into a properly formatted corner array.
Warning: destroys the corners vector in the process.
*/
void configCornerArray(vector<Point2f>& corners, Point2f* corners_a){
  float min_dist;
  int min_idx;
  float dist;
  
  //Make sure the form corners map to the correct image corner
  //by snaping the nearest form corner to each image corner.
  for(int i = 0; i < 4; i++ ){
    min_dist = FLT_MAX;
    for(int j = 0; j < corners.size(); j++ ){
      dist = norm(corners[j]-corners_a[i]);
      if(dist < min_dist){
        min_dist = dist;
        min_idx = j;
      }
    }
    corners_a[i]=corners[min_idx];
    //Two relatively minor reasons for doing this,
    //1. Small efficiency gain
    //2. If 2 corners share a closest point this resolves the conflict.
    corners.erase(corners.begin()+min_idx);
  }
}

int main(int argc, char* argv[])
{

	const char* name = "Feature Detection Window";
  vector < Point2f > corners;
  Mat img, imgGrey, imgGrey_dialated, tmask , out, warped;
  
	Point2f orig_corners[4];
	Point2f corners_a[4];
	
	// Make window
	namedWindow( name, CV_WINDOW_NORMAL);

	// Create trackbars
	cvCreateTrackbar( "Image", name, &p0_switch_value, 2, switch_callback_p0 );
	cvCreateTrackbar( "Step", name, &step_switch_value, 4, switch_callback_step );
	//cvCreateTrackbar( "Block/Aperture Size", name, &ap_switch_value, 4, switch_callback_a );
	cvCreateTrackbar( "Dilation", name, &dilation_switch_value, 10, switch_callback_di );
	cvCreateTrackbar( "Dist", name, &d_switch_value, 200, switch_callback_d );
	//cvCreateTrackbar( "Save", name, &save_switch_value, 1, switch_callback_save );
	
	string param0_pv;
	// Loop for dynamicly altering parameters in window
	while( 1 ) {
	  // Allows exit via Esc
		if( cvWaitKey( 15 ) == 27 ) 
			break;
			
		if (param0 != param0_pv){
	    // Read the input image
    	img = imread(param0);
	    if (img.data == NULL) {
		    return false;
	    }
	    cvtColor(img, imgGrey, CV_RGB2GRAY);
	
			// Create a mask that limits corner detection to the corners of the image.
			tmask= Mat::zeros(imgGrey.rows, imgGrey.cols, CV_8U);
			circle(tmask, Point(0,0), (tmask.cols+tmask.rows)/8, Scalar(255,255,255), -1);
			circle(tmask, Point(0,tmask.rows), (tmask.cols+tmask.rows)/8, Scalar(255,255,255), -1);
			circle(tmask, Point(tmask.cols,0), (tmask.cols+tmask.rows)/8, Scalar(255,255,255), -1);
			circle(tmask, Point(tmask.cols,tmask.rows), (tmask.cols+tmask.rows)/8, Scalar(255,255,255), -1);

	    //orig_corners = {Point(0,0),Point(img.cols,0),Point(0,img.rows),Point(img.cols,img.rows)};
	    orig_corners[0] = Point(0,0);
	    orig_corners[1] = Point(img.cols,0);
	    orig_corners[2] = Point(0,img.rows);
	    orig_corners[3] = Point(img.cols,img.rows);
	    param0_pv = param0;
    }
			
    // Dilating reduces noise, thin lines and small marks.
    dilate(imgGrey, imgGrey_dialated, Mat(), Point(-1, -1), init_dilation);
    
    if (step == 0){
      imshow(name, imgGrey_dialated);
      continue;
    }
    
    /*
    Params for goodFeaturesToTrack:
    Source Mat, Dest Mat
    Number of features/interest points to return
    Minimum feature quality
    Min distance between corners (Probably needs parameterization depending on im. res. and form)
    Mask
    Block Size (not totally sure but I think it's similar to aperture)
    Use Harris detector (true) or cornerMinEigenVal() (false)
    Free parameter of Harris detector
    */
    goodFeaturesToTrack(imgGrey_dialated, corners, 4, 0.01, distParam, tmask, block_size, false, 0.04);

    if (step == 1){
      //draw corner markers
      imgGrey.copyTo(out);
      for(int i = 0; i < corners.size(); i++ ){
        circle( out, corners[i], 16, Scalar(255,255,255), 2, 8);
      }
      imshow(name, out);
      continue;
    }
    
    
    // Initialize the value of corners_a to that of orig_corners
    memcpy(corners_a, orig_corners, sizeof(orig_corners));
    configCornerArray(corners, corners_a);
    
    Mat H = getPerspectiveTransform(corners_a , orig_corners);
    warpPerspective(imgGrey, warped, H, img.size());
    
    if (step == 2){
      imshow(name, warped);
      if(save==1){
        imwrite("w_"+param0, warped);
        save = 0;
      }
      continue;
    }
    
    if (step == 3){
      GaussianBlur(warped, out, Size(3, 3), 2, 2);
      dilate(out, out, Mat(), Point(-1, -1), bf_dilation);
      adaptiveThreshold(out, out, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, thresh_ap, -1.5);
      imshow(name, out);
      continue;
    }
    
    vector < Vec3f > bubbles = findBubbles(warped);
    // Draw suspected bubble locations (might help to draw sizes as well)
    for(int i = 0; i < bubbles.size(); i++ ){
      circle( warped, Point(cvRound( bubbles[i][0]), cvRound( bubbles[i][1])), bubbles[i][2], Scalar(0,255,0), 2, 4);
    } 
    
    imshow(name, warped);
	}

	cvDestroyWindow( name );

	return 0;
}
