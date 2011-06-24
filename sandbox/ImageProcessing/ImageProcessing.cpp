#include "cv.h"
#include "highgui.h"

#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "./ImageProcessing.h"
#include "./formAlignment.h"
//#include "./FileUtils.h"

// how wide is the segment in pixels
#define SEGMENT_WIDTH 144
// how tall is the segment in pixels
#define SEGMENT_HEIGHT 200

#if 0
#define SCALEPARAM 1.1
#else
#define SCALEPARAM 0.55
#endif

// buffer around segment in pixels
#define SEGMENT_BUFFER 70

#define DEBUG 1

using namespace cv;

#include "nameGenerator.h"
NameGenerator namer("");

string imgfilename;

void getSegmentLocations(vector<Point2f> &segmentcorners, string segfile);
Mat getSegmentMat(Mat &img, Point2f &corner);

vector<bubble_val> processSegment(Mat &segment, string bubble_offsets, PCA_classifier& myClassifier) {
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
	
		Point refined_location = myClassifier.bubble_align(segment, *it);
		bubble_val current_bubble = myClassifier.classifyBubble(segment, refined_location);
		retvals.push_back(current_bubble);
		
		#if DEBUG > 0
		Scalar color(0, 0, 0);
		switch(current_bubble){
			case EMPTY_BUBBLE:
				color = Scalar(0, 0, 255);
				break;
			case PARTIAL_BUBBLE:
				color = Scalar(255, 0, 255);
				break;
			case FILLED_BUBBLE:
				color = Scalar(255, 0, 0);
				break;
		}
		rectangle(dbg_out, (*it)-Point2f(EXAMPLE_WIDTH/2,EXAMPLE_HEIGHT/2),
			(*it)+Point2f(EXAMPLE_WIDTH/2,EXAMPLE_HEIGHT/2), color);
		circle(dbg_out, refined_location, 1, Scalar(255, 2555, 255), -1);
		#endif
	}

	#if DEBUG > 0
	string directory = "debug_segment_images";
	directory.append("/");
	string segfilename = namer.get_unique_name("marked_");
	segfilename.append(".jpg");
	imwrite(directory+segfilename, dbg_out);
	#endif

	return retvals;
}

//TODO:	1. Ged rid of the bubble/segment-offsets files and start reading from template_filename.
//		2. Remove the weight arguement and make a setter function for adjusting different parameters.
//		   I'm not sure how the organize the parameters for the aligmener and classifier, I guess they should
//		   all go in the same file.
vector< vector<bubble_val> > ProcessImage(string &imagefilename, string &template_filename, PCA_classifier& myClassifier) {
	#if DEBUG > 0
	cout << "debug level is: " << DEBUG << endl;
	#endif
	string seglocfile("segment-offsets-tmp.txt");
	string buboffsetfile("bubble-offsets.txt");
	
	vector < Point2f > corners, segment_locations;
	vector<bubble_val> bubble_vals;
	vector<vector<bubble_val> > segment_results;
	Mat imgGrey, out, warped;
	imgfilename = imagefilename;

	// Read the input image
	imgGrey = imread(imagefilename, 0);
	assert(imgGrey.data != NULL);

	#if DEBUG > 0
	cout << "straightening image" << endl;
	#endif
	Mat straightened_image(1, 1, CV_8U);
	//straightenImage(imgGrey, straightened_image);
	align_image(imgGrey, straightened_image, Size(2550 * SCALEPARAM, 3300 * SCALEPARAM));
	

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
		align_image(segment, aligned_segment, aligned_segment.size());
		segment_results.push_back(processSegment(aligned_segment, buboffsetfile, myClassifier));
	}

	return segment_results;
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
