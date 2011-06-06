#include "cv.h"
#include "cxcore.h"
#include "highgui.h"
#include "ImageProcessing.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "./ImageProcessing.h"
#include "./FileUtils.h"

using namespace std;
using namespace cv;

/*
 * processing constants
 */
#define DILATION 6
#define BLOCK_SIZE 3
#define DIST_PARAM 500

// how wide is the segment in pixels
#define SEGMENT_WIDTH 144

// how tall is the segment in pixels
#define SEGMENT_HEIGHT 200

// buffer around segment in pixels
#define SEGMENT_BUFFER 10

#define DEBUG 1

Mat comparison_vectors;
PCA my_PCA;
vector <bubble_val> training_bubble_values;
vector <Point2f> training_bubbles_locations;
float weight_param;

void configCornerArray(vector<Point2f>& corners, Point2f* corners_a);
void straightenImage(const Mat& input_image, Mat& output_image);
bubble_val checkBubble(Mat& det_img_gray, Point2f& bubble_location);
void getSegmentLocations(vector<Point2f> &segmentcorners, string segfile);
vector<bubble_val> processSegment(Mat &segment, string bubble_offsets, int *top, int *left);
Mat getSegmentMat(Mat &img, Point2f &corner);
void find_bounding_lines(Mat& img, int* upper, int* lower, bool vertical);

vector<vector<bubble_val> > ProcessImage(string &imagefilename, string &bubblefilename, float &weight) {
  cout << "debug level is: " << DEBUG << endl;
  string seglocfile("segment-offsets.txt");
  string buboffsetfile("bubble-offsets.txt");
  weight_param = weight;
  vector < Point2f > corners, segment_locations;
  vector<bubble_val> bubble_vals;
  vector<Mat> segmats;
  vector<vector<bubble_val> > segment_results;
  Mat img, imgGrey, out, warped;

  // Read the input image
  img = imread(imagefilename);
  if (img.data == NULL) {
    return vector<bubble_val>();
  }

  #if DEBUG > 0
  cout << "converting to grayscale" << endl;
  #endif
  cvtColor(img, imgGrey, CV_RGB2GRAY);

  Mat straightened_image(3300, 2550, CV_16U);

  #if DEBUG > 0
  cout << "straightening image" << endl;
  #endif
  straightenImage(imgGrey, straightened_image);
  
  #if DEBUG > 0
  cout << "writing to output image" << endl;
  imwrite("straightened_" + imagefilename, straightened_image);
  #endif
  
  #if DEBUG > 0
  cout << "getting segment locations" << endl;
  #endif
  getSegmentLocations(segment_locations, seglocfile);

  #if DEBUG > 0
  cout << "grabbing individual segment images" << endl;
  #endif
  for (vector<Point2f>::iterator it = segment_locations.begin();
       it != segment_locations.end(); it++) {
    segmats.push_back(getSegmentMat(straightened_image, *it));
  }

  #if DEBUG > 0
  cout << "processing all segments" << endl;
  #endif
  for (vector<Mat>::iterator it = segmats.begin(); it != segmats.end(); it++) {
    int top, bottom, left, right;

    #if DEBUG > 0
    cout << "finding top and bottom bounding lines" << endl;
    #endif
    find_bounding_lines(*it, &top, &bottom, true);
    #if DEBUG > 0
    cout << "finding left and right bounding lines" << endl;
    #endif
    find_bounding_lines(*it, &left, &right, false);

    #if DEBUG > 0
    cout << "processing segment" << endl;
    #endif
    segment_results.push_back(processSegment(*it, buboffsetfile, &top, &left));
  }

  //rectangle(straightened_image, (*it)-Point2f(7,9), (*it)+Point2f(7,9), color);

  #if DEBUG > 0
  imwrite("withbubbles_" + imagefilename, straightened_image);
  #endif

  return segment_results;
}

vector<bubble_val> processSegment(Mat &segment, string bubble_offsets, int *top, int *left) {
  vector<Point2f> bubble_locations;
  vector<bubble_val> retvals;
  string line;
  float bubx, buby;
  ifstream offsets(bubble_offsets.c_str());

  if (offsets.is_open()) {
    while (getline(offsets, line)) {
      if (line != "") {
        stringstream ss(line);

        ss << bubx;
        ss << buby;
        Point2f bubble(bubx + *top, buby + *left);
        bubble_locations.push_back(bubble);
      }
    }
  }

  vector<Point2f>::iterator it;
  for (it = bubble_locations.begin(); it != bubble_locations.end(); it++) {
    bubble_val current_bubble = checkBubble(segment, *it);
    retvals.push_back(current_bubble);
  }

  return retvals;
}

Mat getSegmentMat(Mat &img, Point2f &corner) {
  Mat segment;
  Point2f segcenter;
  segcenter += corner;
  segcenter.x += SEGMENT_WIDTH/2;
  segcenter.y += SEGMENT_HEIGHT/2;
  Size segsize(SEGMENT_WIDTH + SEGMENT_BUFFER, SEGMENT_HEIGHT + SEGMENT_BUFFER);
  getRectSubPix(img, segsize, segcenter, segment);

  #if DEBUG > 0
  string pcorner;
  pcorner.push_back(corner.x);
  pcorner.append("-");
  pcorner.push_back(corner.y);
  //imwrite("segment_" + pcorner, segment);
  #endif

  return segment;
}

void getSegmentLocations(vector<Point2f> &segmentcorners, string segfile) {
  string line;
  float segx, segy;

  ifstream segstream(segfile.c_str());
  if (segstream.is_open()) {
    while (getline(segstream, line)) {
      if (line != "") {
        stringstream ss(line);

        ss << segx;
        ss << segy;
        Point2f corner(segx, segy);
        segmentcorners.push_back(corner);
      }
    }
  }
}

void find_bounding_lines(Mat& img, int* upper, int* lower, bool vertical) {
  Mat grad_img, out;
  Sobel(img, grad_img, 0, int(!vertical), int(vertical));
  multiply(grad_img, img/100, grad_img);//seems to yield improvements on bright images
  reduce(grad_img, out, int(!vertical), CV_REDUCE_SUM, CV_32F);
  GaussianBlur(out, out, Size(1,3), 1.0);

  if( vertical )
    transpose(out,out);

  Point min_location_top;
  Point min_location_bottom;
  minMaxLoc(out(Range(0, out.rows/2), Range(0,1)), NULL,NULL,&min_location_top);
  minMaxLoc(out(Range(out.rows/2 ,out.rows), Range(0,1)), NULL,NULL,&min_location_bottom);
  *upper = min_location_top.y;
  *lower = min_location_bottom.y + out.rows/2;
}

void straightenImage(const Mat& input_image, Mat& output_image) {
  #if DEBUG > 0
  cout << "entering StraightenImage" << endl;
  #endif
  Point2f orig_corners[4];
  Point2f corners_a[4];
  vector < Point2f > corners;

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

  #if DEBUG > 0
  cout << "dilating image" << endl;
  #endif
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
  #if DEBUG > 0
  cout << "finding corners of the paper" << endl;
  #endif
  goodFeaturesToTrack(input_image_dilated, corners, 4, 0.01, DIST_PARAM, tmask, BLOCK_SIZE, false, 0.04);

  // Initialize the value of corners_a to that of orig_corners
  memcpy(corners_a, orig_corners, sizeof(orig_corners));
  configCornerArray(corners, corners_a);
  
  Mat H = getPerspectiveTransform(corners_a , orig_corners);
  #if DEBUG > 0
  cout << "resizing and warping" << endl;
  #endif
  warpPerspective(input_image, output_image, H, output_image.size());

  #if DEBUG > 0
  cout << "exiting StraightenImage" << endl;
  #endif
}

/*
Takes a vector of corners and converts it into a properly formatted corner array.
Warning: destroys the corners vector in the process.
*/
void configCornerArray(vector<Point2f>& corners, Point2f* corners_a){
  #if DEBUG > 0
  cout << "in configCornerArray" << endl;
  #endif
  float min_dist;
  int min_idx;
  float dist;
  
  //Make sure the form corners map to the correct image corner
  //by snaping the nearest form corner to each image corner.
  for(int i = 0; i < 4; i++) {
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

  #if DEBUG > 0
  cout << "exiting configCornerArray" << endl;
  #endif
}

//Assuming there is a bubble at the specified location,
//this function will tell you if it is filled, empty, or probably not a bubble.
bubble_val checkBubble(Mat& det_img_gray, Point2f& bubble_location) {
  #if DEBUG > 1
  cout << "in checkBubble" << endl;
  cout << "checking bubble location: " << bubble_location << endl;
  #endif
  Mat query_pixels;
  getRectSubPix(det_img_gray, Point(14,18), bubble_location, query_pixels);
  query_pixels.convertTo(query_pixels, CV_32F);
  //normalize(query_pixels, query_pixels);
  transpose(my_PCA.project(query_pixels.reshape(0,1)), query_pixels);
  Mat M = (Mat_<float>(3,1) << weight_param, 1.0 - weight_param, 1.0);
  query_pixels = comparison_vectors*query_pixels.mul(M);
  Point max_location;
  minMaxLoc(query_pixels, NULL, NULL, NULL, &max_location);
  #if DEBUG > 1
  cout << "exiting checkBubble" << endl;
  #endif
  return training_bubble_values[max_location.y];
}

void train_PCA_classifier() {
  #if DEBUG > 0
  cout << "training PCA classifier" << endl;
  #endif
  //Goes though all the selected bubble locations and puts their pixels into rows of
  //a giant matrix called so we can perform PCA on them (we need at least 3 locations to do this)
  vector<Point2f> training_training_bubbles_locations;
  Mat train_img, train_img_gray;

  train_img = imread("training-image.jpg");
  if (train_img.data == NULL) {
    return;
  }

  cvtColor(train_img, train_img_gray, CV_RGB2GRAY);

  // hardcoded training bubble locations
  training_bubble_values.push_back(EMPTY_BUBBLE);
  training_bubbles_locations.push_back(Point2f(35, 28));
  training_bubble_values.push_back(EMPTY_BUBBLE);
  training_bubbles_locations.push_back(Point2f(99, 35));
  training_bubble_values.push_back(FILLED_BUBBLE);
  training_bubbles_locations.push_back(Point2f(113, 58));
  training_bubble_values.push_back(EMPTY_BUBBLE);
  training_bubbles_locations.push_back(Point2f(187, 11));
  training_bubble_values.push_back(FILLED_BUBBLE);
  training_bubbles_locations.push_back(Point2f(200, 61));
  training_bubble_values.push_back(EMPTY_BUBBLE);
  training_bubbles_locations.push_back(Point2f(302, 58));
  training_bubble_values.push_back(FILLED_BUBBLE);
  training_bubbles_locations.push_back(Point2f(276, 12));
  training_bubble_values.push_back(EMPTY_BUBBLE);
  training_bubbles_locations.push_back(Point2f(385, 36));
  training_bubble_values.push_back(FILLED_BUBBLE);
  training_bubbles_locations.push_back(Point2f(372, 107));
  Mat PCA_set = Mat::zeros(training_bubbles_locations.size(), 18*14, CV_32F);

  for(size_t i = 0; i < training_bubbles_locations.size(); i+=1) {
    Mat PCA_set_row;
    getRectSubPix(train_img_gray, Point(14,18), training_bubbles_locations[i], PCA_set_row);
    PCA_set_row.convertTo(PCA_set_row, CV_32F);
    normalize(PCA_set_row, PCA_set_row); //lighting invariance?
    PCA_set.row(i) += PCA_set_row.reshape(0,1);
  }

  my_PCA = PCA(PCA_set, Mat(), CV_PCA_DATA_AS_ROW, 3);
  comparison_vectors = my_PCA.project(PCA_set);
  for(size_t i = 0; i < comparison_vectors.rows; i+=1){
    comparison_vectors.row(i) /= norm(comparison_vectors.row(i));
  }

  #if DEBUG > 0
  cout << "finished training PCA classifier" << endl;
  #endif
}
