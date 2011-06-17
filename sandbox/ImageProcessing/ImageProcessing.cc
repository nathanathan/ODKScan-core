#include "cv.h"
#include "highgui.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "./ImageProcessing.h"
#include "./formAlignment.h"
#include "./FileUtils.h"

#define DEBUG 1

using namespace cv;

string imgfilename;

template <class Tp>
void configCornerArray(vector<Tp>& found_corners, Point2f* dest_corners, float expand = 0);
void straightenImage(const Mat& input_image, Mat& output_image);
void getSegmentLocations(vector<Point2f> &segmentcorners, string segfile);
string get_unique_name(string prefix);
vector<bubble_val> processSegment(Mat &segment, string bubble_offsets);
Mat getSegmentMat(Mat &img, Point2f &corner);
void find_bounding_lines(Mat& img, int* upper, int* lower, bool vertical);

vector< vector<bubble_val> > ProcessImage(string &imagefilename, string &bubblefilename, float &weight) {
	#if DEBUG > 0
	cout << "debug level is: " << DEBUG << endl;
	#endif
	string seglocfile("segment-offsets-tmp.txt");
	string buboffsetfile("bubble-offsets.txt");
	set_weight(weight);
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
	//align_image(imgGrey, straightened_image, 1.0);

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

		Mat segment = getSegmentMat(straightened_image, loc);
		Mat aligned_segment(segment.rows - (SEGMENT_BUFFER * SCALEPARAM),
				            segment.cols - (SEGMENT_BUFFER * SCALEPARAM), CV_8UC1);
		#if DEBUG > 0
		cout << "aligning segment" << endl;
		#endif
		align_image(segment, aligned_segment, .6);
		segment_results.push_back(processSegment(aligned_segment, buboffsetfile));
	}

	return segment_results;
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
		bubble_val current_bubble = classifyBubble(segment, refined_location);
		retvals.push_back(current_bubble);
		
		#if DEBUG > 0
		Scalar color(0, 255, 0);
		if (current_bubble == 1) {
			color = (255, 0, 255);
		}
		rectangle(dbg_out, (*it)-Point2f(EXAMPLE_WIDTH/2,EXAMPLE_HEIGHT/2),
			(*it)+Point2f(EXAMPLE_WIDTH/2,EXAMPLE_HEIGHT/2), color);
		circle(dbg_out, refined_location, 1, Scalar(255, 2555, 255), -1);
		#endif
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
