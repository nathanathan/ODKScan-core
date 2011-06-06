#include "cv.h"
#include "cxcore.h"
#include "highgui.h"
#include "ImageProcessing.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "./ImageProcessing.h"

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
}

using namespace std;
using namespace cv;

#define DILATION 6
#define BLOCK_SIZE 3
#define DIST_PARAM 500

#define DEBUG 1

Mat comparison_vectors;
PCA my_PCA;
vector <bubble_val> training_bubble_values;
vector <Point2f> training_bubbles_locations;
float weight_param;

void configCornerArray(vector<Point2f>& corners, Point2f* corners_a);
void straightenImage(const Mat& input_image, Mat& output_image);
bubble_val checkBubble(Mat& det_img_gray, Point2f& bubble_location);

// recursive helper method to crawl directories
static void HandleDir(char *dirpath, DIR *d, vector<string> &filenames);

// crawls a directory rootdir for filenames and appends them to the filenames
// vector parameter
int CrawlFileTree(char *rootdir, vector<string> &filenames);

vector<bubble_val> ProcessImage(string &imagefilename, string &bubblefilename, float &weight) {
  cout << "debug level is: " << DEBUG << endl;
  weight_param = weight;
  vector < Point2f > corners, bubbles;
  vector<bubble_val> bubble_vals;
  Mat img, imgGrey, out, warped;
  string line;
  float bubx, buby;
  int bubble;

  // read the bubble location file for bubble locations
  ifstream bubblefile(bubblefilename.c_str());
  if (bubblefile.is_open()) {
    while (getline(bubblefile, line)) {
      stringstream ss(line);
      // bubble x
      ss >> bubx;
      // bubble y
      ss >> buby;

      // push the bubble location onto the bubbles vector
      Point2f bubble(bubx, buby);
      bubbles.push_back(bubble);
    }
  }

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
  cout << "checking bubbles" << endl;
  #endif

  // process each bubble location to find if it's empty or filled
  vector<Point2f>::iterator it;
  for (it = bubbles.begin(); it != bubbles.end(); it++) {
    Scalar color(0, 0, 0);
    bubble_val current_bubble = checkBubble(straightened_image, *it);
    bubble_vals.push_back(current_bubble);

    #if DEBUG > 0
    // put a rectangle around where we're looking for the bubble (debug purposes)
    rectangle(straightened_image, (*it)-Point2f(7,9), (*it)+Point2f(7,9), color);
    #endif
  }

  #if DEBUG > 0
  imwrite("withbubbles_" + imagefilename, straightened_image);
  #endif

  return bubble_vals;
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

int CrawlFileTree(char *rootdir, vector<string> &filenames) {
  struct stat rootstat;
  int result;
  DIR *rd;

  // Verify that rootdir is a directory.
  result = lstat((char *) rootdir, &rootstat);
  if (result == -1) {
    // We got some kind of error stat'ing the file. Give up
    // and return an error.
    return 0;
  }
  if (!S_ISDIR(rootstat.st_mode)) {
    // It isn't a directory, so give up.
    return 0;
  }

  // Try to open the directory using opendir().  If try but fail,
  // (e.g., we don't have permissions on the directory), return NULL.
  // ("man 3 opendir")
  rd = opendir(rootdir);
  if (rd == NULL) {
    return 0;
  }

  // Begin the recursive handling of the directory.
  HandleDir(rootdir, rd, filenames);

  // All done, free up.
  assert(closedir(rd) == 0);
  return 1;
}


static void HandleDir(char *dirpath, DIR *d, vector<string> &filenames) {
  // Loop through the directory.
  while (1) {
    char *newfile;
    int res, charsize;
    struct stat nextstat;
    struct dirent *dirent = NULL;

    // Use the "readdir()" system call to read the next directory
    // entry. (man 3 readdir).  If we hit the end of the
    // directory (i.e., readdir returns NULL and errno == 0),
    // return back out of this function.  Note that you have
    // access to the errno global variable as a side-effect of
    // us having #include'd <errno.h>.
    errno = 0;

    dirent = readdir(d);

    if (dirent == NULL && errno == 0) return;

    // If the directory entry is named "." or "..",
    // ignore it.  (use the C "continue;" expression
    // to begin the next iteration of the while() loop.)
    // You can find out the name of the directory entry
    // through the "d_name" field of the struct dirent
    // returned by readdir(), and you can use strcmp()
    // to compare it to "." or ".."
    if ((strcmp(dirent->d_name, ".") == 0) ||
        (strcmp(dirent->d_name, "..") == 0))
      continue;

    // We need to append the name of the file to the name
    // of the directory we're in to get the full filename.
    // So, we'll malloc space for:
    //
    //     dirpath + "/" + dirent->d_name + '\0'
    charsize = strlen(dirpath) + 1 + strlen(dirent->d_name) + 1;
    newfile = (char *) malloc(charsize);
    assert(newfile != NULL);
    if (dirpath[strlen(dirpath)-1] == '/') {
      // no need to add an additional '/'
      snprintf(newfile, charsize, "%s%s", dirpath, dirent->d_name);
    } else {
      // we do need to add an additional '/'
      snprintf(newfile, charsize, "%s/%s", dirpath, dirent->d_name);
    }

    // Use the "lstat()" system call to ask the operating system
    // to give us information about the file named by the
    // directory entry.   ("man lstat")
    res = lstat(newfile, &nextstat);
    if (res == 0) {
      // Test to see if the file is a "regular file" using
      // the S_ISREG() macro described in the lstat man page.
      // If so, process the file by invoking the HandleFile()
      // private helper function.
      //
      // On the other hand, if the file turns out to be a
      // directory (which you can find out using the S_ISDIR()
      // macro described on the same page, then you need to
      // open the directory using opendir()  (man 3 opendir)
      // and recursively invoke HandleDir to handle it.
      // Be sure to call the "closedir()" system call
      // when the recursive HandleDir() returns to close the
      // opened directory.

      if (S_ISREG(nextstat.st_mode))
        filenames.push_back(newfile);

      if (S_ISDIR(nextstat.st_mode)) {
        DIR* d = opendir(newfile);
        HandleDir(newfile, d, filenames);
        closedir(d);
      }
    }

    // Done with this file.  Fall back up to the next
    // iteration of the while() loop.
    free(newfile);
  }
}
