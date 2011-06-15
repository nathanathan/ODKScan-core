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

#define EXAMPLE_WIDTH 14
#define EXAMPLE_HEIGHT 18

// how wide is the segment in pixels
#define SEGMENT_WIDTH 144

// how tall is the segment in pixels
#define SEGMENT_HEIGHT 200

#define SCALEPARAM 0.55

// buffer around segment in pixels
#define SEGMENT_BUFFER 70

#define EIGENBUBBLES 5

#define DEBUG 1
#define DEBUG_ALIGN_SEGMENT 1

//This counter is used to generate unique file names;
int global_counter = 0;

Mat comparison_vectors;
PCA my_PCA;
vector <bubble_val> training_bubble_values;
vector <Point2f> training_bubbles_locations;
float weight_param;
string imgfilename;
Point search_window(6, 8);

template <class Tp>
void configCornerArray(vector<Tp>& found_corners, Point2f* dest_corners, float expand = 0);
void straightenImage(const Mat& input_image, Mat& output_image);
double rateBubble(Mat& det_img_gray, Point bubble_location);
Point bubble_align(Mat& det_img_gray, Point bubble_location);
bubble_val checkBubble(Mat& det_img_gray, Point bubble_location);
void getSegmentLocations(vector<Point2f> &segmentcorners, string segfile);
vector<bubble_val> processSegment(Mat &segment, string bubble_offsets);
Mat getSegmentMat(Mat &img, Point2f &corner);
void find_bounding_lines(Mat& img, int* upper, int* lower, bool vertical);
void align_segment(Mat& img, Mat& aligned_segment);


//Takes a vector of found corners, and an array of destination corners they should map to
//and replaces each corner in the dest_corners with the nearest unmatched found corner.
//The optional expand arguement moves each found corner in the direction of the destination
//corner it is mapped to.
template <class Tp>
void configCornerArray(vector<Tp>& found_corners, Point2f* dest_corners, float expand = 0) {
	float min_dist;
	int min_idx;
	float dist;

	vector<Point2f> corners;

	for(int i = 0; i < found_corners.size(); i++ ){
		corners.push_back(Point2f(float(found_corners[i].x), float(found_corners[i].y)));
	}
	for(int i = 0; i < 4; i++) {
		min_dist = FLT_MAX;
		for(int j = 0; j < corners.size(); j++ ){
			dist = norm(corners[j]-dest_corners[i]);
			if(dist < min_dist){
				min_dist = dist;
				min_idx = j;
			}
		}
		dest_corners[i]=corners[min_idx] + expand * (dest_corners[i] - corners[min_idx]);
		corners.erase(corners.begin()+min_idx);
	}
}

void find_bounding_lines(Mat& img, int* upper, int* lower, bool vertical) {
  int center_size = 20 * SCALEPARAM;
  Mat grad_img, out;
  Sobel(img, grad_img, 0, int(!vertical), int(vertical));

  reduce(grad_img, out, int(!vertical), CV_REDUCE_SUM, CV_32F);
  GaussianBlur(out, out, Size(1,(int)(3 * SCALEPARAM)), 1.0 * SCALEPARAM);

  if( vertical )
    transpose(out,out);

  Point min_location_top;
  Point min_location_bottom;
  minMaxLoc(out(Range(3, out.rows/2 - center_size), Range(0,1)), NULL,NULL,&min_location_top);
  minMaxLoc(out(Range(out.rows/2 + center_size,out.rows - 3), Range(0,1)), NULL,NULL,&min_location_bottom);
  *upper = min_location_top.y;
  *lower = min_location_bottom.y + out.rows/2 + center_size;
}

void align_segment(Mat& img, Mat& aligned_segment){
	vector < vector<Point> > contours;
	vector < vector<Point> > borderContours;
	vector < Point > approx;
	vector < Point > maxRect;

	//Threshold the image
	//Maybe we should dilate or blur or something first?
	//The ideal image would be black lines and white boxes with nothing in them
	//so if we can filter to get something closer to that, it is a good thing.
	Scalar my_mean;
	Scalar my_stddev;
	meanStdDev(img, my_mean, my_stddev);
	Mat imgThresh = img > (my_mean.val[0]-.05*my_stddev.val[0]);

	// Find all external contours of the image
	findContours(imgThresh, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	float maxContourArea = 0;
	// Iterate through all detected contours
	for (size_t i = 0; i < contours.size(); ++i) {
		// reduce the number of points in the contour
		approxPolyDP(Mat(contours[i]), approx,
				     arcLength(Mat(contours[i]), true) * 0.1, true);

		float area = fabs(contourArea(Mat(approx)));

		if (area > maxContourArea) {
			maxRect = approx;
			maxContourArea = area;
		}
		//Maybe I could refine this by using a corner detector and using
		//the 4 contour points with the highest harris responce?
	}
	if ( maxRect.size() == 4 && isContourConvex(Mat(maxRect)) && maxContourArea > (img.cols/2) * (img.rows/2)) {
		Point2f segment_corners[4] = {Point2f(0,0),Point2f(aligned_segment.cols,0),
		Point2f(0,aligned_segment.rows),Point2f(aligned_segment.cols,aligned_segment.rows)};
		Point2f corners_a[4] = {Point2f(0,0),Point2f(img.cols,0),
		Point2f(0,img.rows),Point2f(img.cols,img.rows)};

		configCornerArray(maxRect, corners_a, .1);
		Mat H = getPerspectiveTransform(corners_a , segment_corners);
		warpPerspective(img, aligned_segment, H, aligned_segment.size());
	}
	else{//use the bounding line method if the contour method fails
		int top = 0, bottom = 0, left = 0, right = 0;
		find_bounding_lines(img, &top, &bottom, false);
		find_bounding_lines(img, &left, &right, true);
      
		#if DEBUG_ALIGN_SEGMENT > 0
		img.copyTo(aligned_segment);
		const Point* p = &maxRect[0];
		int n = (int) maxRect.size();
		polylines(aligned_segment, &p, &n, 1, true, 200, 2, CV_AA);

		img.row(top)+=200;
		img.row(bottom)+=200;
		img.col(left)+=200;
		img.col(right)+=200;
		img.copyTo(aligned_segment);
		#else
		float bounding_lines_threshold = .2;
		if ((abs((bottom - top) - aligned_segment.rows) < bounding_lines_threshold * aligned_segment.rows) &&
			(abs((right - left) - aligned_segment.cols) < bounding_lines_threshold * aligned_segment.cols) &&
			top + aligned_segment.rows < img.rows &&  top + aligned_segment.cols < img.cols) {

			img(Rect(left, top, aligned_segment.cols, aligned_segment.rows)).copyTo(aligned_segment);

		}
		else{
			img(Rect(SEGMENT_BUFFER * SCALEPARAM, SEGMENT_BUFFER * SCALEPARAM,
				aligned_segment.cols, aligned_segment.rows)).copyTo(aligned_segment);
		}
		#endif
	}
}

vector< vector<bubble_val> > ProcessImage(string &imagefilename, string &bubblefilename, float &weight) {
  #if DEBUG > 0
  cout << "debug level is: " << DEBUG << endl;
  #endif
  string seglocfile("segment-offsets-tmp.txt");
  string buboffsetfile("bubble-offsets.txt");
  weight_param = weight;
  vector < Point2f > corners, segment_locations;
  vector<bubble_val> bubble_vals;
  vector<vector<bubble_val> > segment_results;
  Mat img, imgGrey, out, warped;
  imgfilename = imagefilename;

  // Read the input image
  img = imread(imagefilename);
  if (img.data == NULL) {
    return vector<vector<bubble_val> >();
  }

  #if DEBUG > 0
  cout << "converting to grayscale" << endl;
  #endif
  cvtColor(img, imgGrey, CV_RGB2GRAY);

  Mat straightened_image(3300 * SCALEPARAM, 2550 * SCALEPARAM, CV_8U);

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
  cout << "creating and processing segment images" << endl;
  #endif
  for (vector<Point2f>::iterator it = segment_locations.begin();
       it != segment_locations.end(); it++) {
    Point2f loc((*it).x * SCALEPARAM, (*it).y * SCALEPARAM);
    
    //Alternative: (might need some border pixels added)
    //Mat segment = straightened_image(Rect((*it).x - SEGMENT_BUFFER * SCALEPARAM, (*it).y - SEGMENT_BUFFER * SCALEPARAM, SEGMENT_WIDTH + SEGMENT_BUFFER * SCALEPARAM, SEGMENT_HEIGHT + SEGMENT_BUFFER * SCALEPARAM));
    
    Mat segment = getSegmentMat(straightened_image, loc);
    Mat aligned_segment(segment.rows - (SEGMENT_BUFFER * SCALEPARAM),
                        segment.cols - (SEGMENT_BUFFER * SCALEPARAM), CV_8UC1);
    #if DEBUG > 0
    cout << "aligning segment" << endl;
    #endif
    align_segment(segment, aligned_segment);
    segment_results.push_back(processSegment(aligned_segment, buboffsetfile));
    aligned_segment.copyTo(segment);
  }

  return segment_results;
}

string get_unique_name(string prefix) {
	stringstream ss;
	int gc_temp = global_counter;
	while( true ) {
		ss << (char) ((gc_temp % 10) + '0');
		gc_temp /= 10;
		if(gc_temp == 0)
			break;
	}
	global_counter++;
	prefix.append(ss.str());
	return prefix;
}

vector<bubble_val> processSegment(Mat &segment, string bubble_offsets) {
	vector<Point2f> bubble_locations;
	vector<bubble_val> retvals;
	string line;
	float bubx, buby;
	ifstream offsets(bubble_offsets.c_str());
	
	#if DEBUG > 0
	Mat dbg_out;
	cvtColor(segment, dbg_out, CV_GRAY2RGB);
	#endif
	
	if (offsets.is_open()) {
		while (getline(offsets, line)) {
			if (line.size() > 2) {
				stringstream ss(line);

				ss >> bubx;
				ss >> buby;
				Point2f bubble(bubx * SCALEPARAM, buby * SCALEPARAM);
				bubble_locations.push_back(bubble);
			}
		}
	}

	vector<Point2f>::iterator it;
	for (it = bubble_locations.begin(); it != bubble_locations.end(); it++) {
	
		Point refined_location = bubble_align(segment, *it);
		bubble_val current_bubble = checkBubble(segment, refined_location);
		
		#if DEBUG > 0
		Scalar color(0, 255, 0);
		if (current_bubble == 1) {
			color = (255, 0, 255);
		}
		rectangle(dbg_out, (*it)-Point2f(EXAMPLE_WIDTH/2,EXAMPLE_HEIGHT/2),
			(*it)+Point2f(EXAMPLE_WIDTH/2,EXAMPLE_HEIGHT/2), color);
		circle(dbg_out, refined_location, 1, Scalar(255, 2555, 255), -1);
		#endif
		
		retvals.push_back(current_bubble);
	}

	#if DEBUG > 0
	string directory = "debug_segment_images";
	directory.append("/");
	string segfilename = get_unique_name("marked_");
	segfilename.append(".jpg");
	imwrite(directory+segfilename, dbg_out);
	#endif

	return retvals;
}

Mat getSegmentMat(Mat &img, Point2f &corner) {
  Mat segment;
  Point2f segcenter;
  segcenter = corner;
  segcenter.x += (SEGMENT_WIDTH*SCALEPARAM)/2;
  segcenter.y += (SEGMENT_HEIGHT*SCALEPARAM)/2;
  Size segsize((SEGMENT_WIDTH + SEGMENT_BUFFER) * SCALEPARAM,
               (SEGMENT_HEIGHT + SEGMENT_BUFFER) * SCALEPARAM);
  getRectSubPix(img, segsize, segcenter, segment);

  return segment;
}

void getSegmentLocations(vector<Point2f> &segmentcorners, string segfile) {
  string line;
  float segx = 0, segy = 0;

  ifstream segstream(segfile.c_str());
  if (segstream.is_open()) {
    while (getline(segstream, line)) {
      if (line.size() > 2) {
        stringstream ss(line);

        ss >> segx;
        ss >> segy;
        Point2f corner(segx, segy);
        #if DEBUG > 1
        cout << "adding segment corner " << corner << endl;
        #endif
        segmentcorners.push_back(corner);
      }
    }
  }
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
//Rate a location on how likely it is to be a bubble
double rateBubble(Mat& det_img_gray, Point bubble_location) {
    Mat query_pixels, pca_components;
    getRectSubPix(det_img_gray, Size(EXAMPLE_WIDTH,EXAMPLE_HEIGHT), bubble_location, query_pixels);
    query_pixels.reshape(0,1).convertTo(query_pixels, CV_32F);
    pca_components = my_PCA.project(query_pixels);
    //The rating is the SSD of query pixels and their back projection
    Mat out = my_PCA.backProject(pca_components)- query_pixels;
    return sum(out.mul(out)).val[0];
}
Point bubble_align(Mat& det_img_gray, Point bubble_location){
	//This bit of code finds the location in the search_window most likely to be a bubble
	//then it checks that rather than the exact specified location.
	//This section probably slows things down by quite a bit and it might not provide significant
	//improvement to accuracy. We will need to run some tests to find out if it's worth keeping.
	Mat out = Mat::zeros(Size(search_window.x*2 + 1, search_window.y*2 + 1) , CV_32FC1);
	Point offset = Point(bubble_location.x - search_window.x, bubble_location.y - search_window.y);
	for(size_t i = 0; i <= search_window.x*2; i+=1) {
		for(size_t j = 0; j <= search_window.y*2; j+=1) {
			out.col(i).row(j) += rateBubble(det_img_gray, Point(i,j) + offset);
		}
	}
	//Multiplying by a 2D gaussian weights the search so that we are more likely to choose
	//locations near the expected bubble location.
	//However, the sigma parameter probably needs to be teaked.
	Mat v_gauss = 1 - getGaussianKernel(out.rows, 1.0, CV_32F);
	Mat h_gauss;
	transpose(1 - getGaussianKernel(out.cols, 1.0, CV_32F), h_gauss);
	v_gauss = repeat(v_gauss, 1, out.cols);
	h_gauss = repeat(h_gauss, out.rows, 1);
	out = out.mul(v_gauss.mul(h_gauss));
	
	Point min_location;
	minMaxLoc(out, NULL,NULL, &min_location);
	return min_location + offset;
}

//Compare the bubbles with all the bubbles used in the classifier.
bubble_val checkBubble(Mat& det_img_gray, Point bubble_location) {
	Mat query_pixels, out;
	getRectSubPix(det_img_gray, Size(EXAMPLE_WIDTH, EXAMPLE_HEIGHT), bubble_location, query_pixels);
	query_pixels.reshape(0,1).convertTo(query_pixels, CV_32F);

	//Here we find the best filled and empty matches in the PCA training set.
	Mat responce;
	matchTemplate(comparison_vectors, my_PCA.project(query_pixels), responce, CV_TM_CCOEFF_NORMED);
	reduce(responce, out, 1, CV_REDUCE_MAX);
	float max_filled_responce = 0;
	float max_empty_responce = 0;
	for(size_t i = 0; i < training_bubble_values.size(); i+=1) {
		float current_responce = sum(out.row(i)).val[0];
		if( training_bubble_values[i] == FILLED_BUBBLE){
			if(current_responce > max_filled_responce){
				max_filled_responce = current_responce;
			}
		}
		else{
			if(current_responce > max_empty_responce){
			    max_empty_responce = current_responce;
			}
		}
	}
	//Here we compare our filled and empty score with some weighting.
	//To use neighboring bubbles in classification we will probably want this method
	//to return the scores without comparing them.
	if( (weight_param) * max_filled_responce > (1 - weight_param) * max_empty_responce){
		return FILLED_BUBBLE;
    }
    else{
	    return EMPTY_BUBBLE;
    }
}

void train_PCA_classifier() {
	// Set training_bubble_values here
	training_bubble_values.push_back(FILLED_BUBBLE);
	training_bubble_values.push_back(EMPTY_BUBBLE);
	training_bubble_values.push_back(FILLED_BUBBLE);
	training_bubble_values.push_back(EMPTY_BUBBLE);
	training_bubble_values.push_back(FILLED_BUBBLE);
	training_bubble_values.push_back(EMPTY_BUBBLE);
	training_bubble_values.push_back(FILLED_BUBBLE);
	training_bubble_values.push_back(FILLED_BUBBLE);
	training_bubble_values.push_back(EMPTY_BUBBLE);
	training_bubble_values.push_back(FILLED_BUBBLE);
	training_bubble_values.push_back(FILLED_BUBBLE);

	Mat example_strip = imread("example_strip.jpg");
	Mat example_strip_bw;
	cvtColor(example_strip, example_strip_bw, CV_RGB2GRAY);

	int numexamples = example_strip_bw.cols / EXAMPLE_WIDTH;
	Mat PCA_set = Mat::zeros(numexamples, EXAMPLE_HEIGHT*EXAMPLE_WIDTH, CV_32F);

	for (int i = 0; i < numexamples; i++) {
		Mat PCA_set_row = example_strip_bw(Rect(i * EXAMPLE_WIDTH, 0,
				                                EXAMPLE_WIDTH, EXAMPLE_HEIGHT));
		PCA_set_row.convertTo(PCA_set_row, CV_32F);
		PCA_set.row(i) += PCA_set_row.reshape(0,1);
	}

	my_PCA = PCA(PCA_set, Mat(), CV_PCA_DATA_AS_ROW, EIGENBUBBLES);
	comparison_vectors = my_PCA.project(PCA_set);
}
