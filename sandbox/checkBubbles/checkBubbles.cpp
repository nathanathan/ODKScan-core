/*
Program for defining bubble locations,
training a PCA based classifier,
and testing the classifier on various images.
*/
#include "cv.h"
#include "highgui.h"
#include <queue>
#include <iostream>

using namespace std;
using namespace cv;

string file_names[] = {
"VR_segment1.jpg", "VR_segment2.jpg", "VR_segment3.jpg", "VR_segment4.jpg", "VR_segment5.jpg", "VR_segment6.jpg"
};

enum bubble_val { FILLED_BUBBLE, EMPTY_BUBBLE, FALSE_POSITIVE };
vector <bubble_val> bubble_values;
vector <Point2f> bubble_locations;
vector <Point2f> query_locations;

int p0_switch_value = 0;
int p1_switch_value = 0;
int p2_switch_value = 0;

string param0 = file_names[0];
int param1 = 0;
string param2 = file_names[0];

void switch_callback_p0 ( int position ){
	param0 = file_names[position];
}
void switch_callback_p1 ( int position ){
	param1 = position;
}
void switch_callback_p2 ( int position ){
	param2 = file_names[position];
}

//Mouse callbacks for defining bubble locations
void my_mouse_callback_train( int event, int x, int y, int flags, void* param){
	switch( event ){
		case CV_EVENT_LBUTTONDOWN:
			if(flags == CV_EVENT_FLAG_SHIFTKEY){
				bubble_values.push_back(FALSE_POSITIVE);
			}
			else{
				bubble_values.push_back(FILLED_BUBBLE);
			}
			bubble_locations.push_back(Point2f(x, y));
			break;
		case CV_EVENT_RBUTTONDOWN:
			bubble_values.push_back(EMPTY_BUBBLE);
			bubble_locations.push_back(Point2f(x, y));
			break;
	}
}
void my_mouse_callback_query( int event, int x, int y, int flags, void* param){
	if( event == CV_EVENT_LBUTTONDOWN){
		query_locations.push_back(Point2f(x, y));
	}
}

//We should probably encapsulate the PCA stuff...
void train_PCA_classifier(Mat& train_img_gray, PCA& my_PCA, Mat& comparison_vectors){
	//Goes though all the selected bubble locations and puts their pixels into rows of
	//a giant matrix called so we can perform PCA on them (we need at least 3 locations to do this)
	Mat PCA_set = Mat::zeros(bubble_locations.size(), 18*14, CV_32F);
	for(size_t i = 0; i < bubble_locations.size(); i+=1) {
		Mat PCA_set_row;
		getRectSubPix(train_img_gray, Point(14,18), bubble_locations[i], PCA_set_row);
		PCA_set_row.convertTo(PCA_set_row, CV_32F);
		PCA_set.row(i) += PCA_set_row.reshape(0,1);
	}

	my_PCA = PCA(PCA_set, Mat(), CV_PCA_DATA_AS_ROW, 5);
	comparison_vectors = my_PCA.project(PCA_set);
}

//Rate a location on how likely it is to be a bubble
double rateBubble(Mat& det_img_gray, Point bubble_location, PCA& my_PCA){
	Mat query_pixels, pca_components;
	getRectSubPix(det_img_gray, Point(14,18), bubble_location, query_pixels);
	query_pixels.reshape(0,1).convertTo(query_pixels, CV_32F);
	pca_components = my_PCA.project(query_pixels);
	//The rating is the SSD of query pixels and their back projection
	Mat out = my_PCA.backProject(pca_components)- query_pixels;
	return sum(out.mul(out)).val[0];
}

//Compare the bubbles with all the bubbles used in the classifier.
bubble_val checkBubble(Mat& det_img_gray, Point bubble_location, PCA& my_PCA, Mat& comparison_vectors, Point search_window=Point(5,5)){
	Mat query_pixels;

	//This bit of code finds the location in the search_window most likely to be a bubble 
	//then it checks that rather than the exact specified location.
	Mat out = Mat::zeros(Size(search_window.y, search_window.x) , CV_32FC1);
	Point offset = Point(bubble_location.x - search_window.x/2, bubble_location.y - search_window.y/2);
	for(size_t i = 0; i < search_window.y; i+=1) {
		for(size_t j = 0; j < search_window.x; j+=1) {
			out.row(i).col(j) += rateBubble(det_img_gray, Point(j,i) + offset, my_PCA);
		}
	}
	Point min_location;
	minMaxLoc(out, NULL,NULL, &min_location);

	getRectSubPix(det_img_gray, Point(14,18), min_location + offset, query_pixels);

	query_pixels.reshape(0,1).convertTo(query_pixels, CV_32F);
	Point max_location;
	matchTemplate(comparison_vectors, my_PCA.project(query_pixels), out, CV_TM_CCOEFF_NORMED);
	minMaxLoc(out, NULL,NULL,NULL, &max_location);
	return bubble_values[max_location.y];
}

int main(int argc, char* argv[])
{
	const char* training_window = "Training Window";
	const char* detection_window = "Feature Detection Window";

	Mat train_img, train_img_gray, train_img_marked;
	Mat det_img, det_img_gray, det_img_marked, comparison_vectors;

	// Make window
	namedWindow( detection_window, CV_WINDOW_NORMAL);
	namedWindow( training_window, CV_WINDOW_NORMAL);

	// Create trackbars
	cvCreateTrackbar( "Training Image", training_window, &p0_switch_value, 5, switch_callback_p0 );
	cvCreateTrackbar( "Show Bubble Ratings", detection_window, &p1_switch_value, 1, switch_callback_p1 );
	cvCreateTrackbar( "Detection Image", detection_window, &p2_switch_value, 5, switch_callback_p2 );

	// Set up the callback
	cvSetMouseCallback( training_window, my_mouse_callback_train, NULL);
	cvSetMouseCallback( detection_window, my_mouse_callback_query, NULL);

	string param0_pv, param2_pv;
	int bubble_locations_psize;
	bool classifier_trained;
	PCA my_PCA;

	// Loop for dynamicly altering parameters in window
	while( 1 ) {
		// Allows exit via Esc
		if( cvWaitKey( 15 ) == 27 ) 
			break;

		if (param0 != param0_pv){
			param0_pv = param0;
			// Read the training image
			train_img = imread(param0);
			if (train_img.data == NULL) {
				return false;
			}
			cvtColor(train_img, train_img_gray, CV_RGB2GRAY);
			train_img.copyTo(train_img_marked);

			//initialize state variables
			classifier_trained = false;
			bubble_locations_psize = -1;
			bubble_locations.clear();
			bubble_values.clear();
		}

		if(int(bubble_locations.size()) > bubble_locations_psize){
			bubble_locations_psize = bubble_locations.size();
			//Mark up the source image with selected bubble locations
			for(size_t i = 0; i < bubble_locations.size(); i+=1) {
				Scalar color;
				switch( bubble_values[i] ){
					case FILLED_BUBBLE:
						color = Scalar(0,255,0);
						break;
					case EMPTY_BUBBLE:
						color = Scalar(0,0,255);
						break;
					case FALSE_POSITIVE:
						color = Scalar(255,0,0);
						break;
				}
				rectangle(train_img_marked, bubble_locations[i]-Point2f(7,9) , bubble_locations[i]+Point2f(7,9), color);
			}
			classifier_trained = false;
			imshow(training_window, train_img_marked);
		}

		if(bubble_locations.size() > 2){
			if( !classifier_trained ){
			train_PCA_classifier(train_img_gray, my_PCA, comparison_vectors);
			classifier_trained = true;
			}
			if( param2 != param2_pv ){
				param2_pv = param2;
				// Read the detection image
				det_img = imread(param2);
				if (det_img.data == NULL){
					return false;
				}
				cvtColor(det_img, det_img_gray, CV_RGB2GRAY);
				det_img.copyTo(det_img_marked);
			}
			if( param1 ){ //Shows whole image responce.
			Mat out = Mat::zeros(det_img_marked.size(), CV_32FC1);

			for(size_t i = 0; i < det_img.rows; i+=1) {
			  for(size_t j = 0; j < det_img.cols; j+=1) {
				out.row(i).col(j) += rateBubble(det_img_gray, Point(j,i), my_PCA);
			  }
			}
			Scalar my_mean;
			Scalar my_stddev;
			meanStdDev(out, my_mean, my_stddev);
			//imshow(detection_window, out < (my_mean.val[0]-2*my_stddev.val[0]));
			imshow(detection_window, out/my_mean.val[0]);
			}
			else{ //Shows classification of selected locations
				while( !query_locations.empty() ){
					Scalar color;
					switch( checkBubble(det_img_gray, CvPoint(query_locations.back()), my_PCA, comparison_vectors) ){
						case FILLED_BUBBLE:
							color = Scalar(0,255,0);
							break;
						case EMPTY_BUBBLE:
							color = Scalar(0,0,255);
							break;
						case FALSE_POSITIVE:
							color = Scalar(255,0,0);
							break;
					}
					rectangle(det_img_marked,  query_locations.back()-Point2f(7,9), query_locations.back()+Point2f(7,9), color);
					query_locations.pop_back();
				}
				imshow(detection_window, det_img_marked);
			}
		}
	}
	return 0;
}
