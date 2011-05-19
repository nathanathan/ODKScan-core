#include "cv.h"
#include "cxtypes.h"
#include "highgui.h"
#include <string>

using namespace std;
using namespace cv;

#define DILATION 1
#define BLOCK_SIZE 3
#define DIST_PARAM 500

void configCornerArray(vector<Point2f>& corners, Point2f* corners_a);

int ProcessImage(string &filename) {
  vector < Point2f > corners;
  Mat img, imgGrey, out, warped;

  // Read the input image
	img = imread(filename);
  if (img.data == NULL) {
    return false;
  }

  cout << "converting to grayscale" << endl;
  cvtColor(img, imgGrey, CV_RGB2GRAY);

  cout << "straightening image" << endl;
  straightenImage(imgGrey, img, img.size());
  
  cout << "writing to output image" << endl;
  imwrite("w_" + filename, img);
}

void straightenImage(const Mat& input_image, Mat& output_image, Size dsize) {
  Point2f orig_corners[4];
  Point2f corners_a[4];

  Mat tmask, input_image_dilated;

	// Create a mask that limits corner detection to the corners of the image.
	tmask= Mat::zeros(input_image.rows, input_image.cols, CV_8U);
	circle(tmask, Point(0,0), (tmask.cols+tmask.rows)/8, Scalar(255,255,255), -1);
	circle(tmask, Point(0,tmask.rows), (tmask.cols+tmask.rows)/8, Scalar(255,255,255), -1);
	circle(tmask, Point(tmask.cols,0), (tmask.cols+tmask.rows)/8, Scalar(255,255,255), -1);
	circle(tmask, Point(tmask.cols,tmask.rows), (tmask.cols+tmask.rows)/8, Scalar(255,255,255), -1);

  //orig_corners = {Point(0,0),Point(img.cols,0),Point(0,img.rows),Point(img.cols,img.rows)};
  orig_corners[0] = Point(0,0);
  orig_corners[1] = Point(output_image.cols,0);
  orig_corners[2] = Point(0,output_image.rows);
  orig_corners[3] = Point(output_image.cols,output_image.rows);

  // Dilating reduces noise, thin lines and small marks.
  dilate(input_image, input_image_dilated, Mat(), Point(-1, -1), DILATION);

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
  goodFeaturesToTrack(input_image_dilated, corners, 4, 0.01, DIST_PARAM, tmask, BLOCK_SIZE, false, 0.04);

  // Initialize the value of corners_a to that of orig_corners
  memcpy(corners_a, orig_corners, sizeof(orig_corners));
  configCornerArray(corners, corners_a);
  
  Mat H = getPerspectiveTransform(corners_a , orig_corners);
  warpPerspective(input_image, output_image, H, dsize);
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
