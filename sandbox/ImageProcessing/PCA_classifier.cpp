#include "PCA_classifier.h"
#include "highgui.h"
#include <iostream>

#include "FileUtils.h"

#define EIGENBUBBLES 5

//#define NORMALIZE
//Normalizing everything that goes into the PCA *might* help with lighting problems
//But so far just seems to hurt accuracy.

#define USE_GET_RECT_SUB_PIX
//getRectSubPix might slow things down, but provides 2 advanges over the roi method
//1. If the example images have an odd dimension it will linearly interpolate the
//   half pixel error.
//2. If the rectangle crosses the image boundary (because of a large search window)
//   it won't give an error.

//TODO: Add a sigma constant for the search window weighting function and determine approriate value

#define OUTPUT_BUBBLE_IMAGES
#include "nameGenerator.h"
NameGenerator classifierNamer("bubble_images/");

using namespace cv;

Mat comparison_vectors;
PCA my_PCA;
vector <bubble_val> training_bubble_values;

float weight_param = .5;

//Point search_window(6, 8);
Point search_window(0, 0);
//The search window slows things down by a factor
//of the number of pixels it contains.

//Sets the weight to a value between 0 and 1 to bias
//the classifier towards filled 0? or empty 1?
void set_weight(float weight){
	assert(weight >= 0 && weight <= 1);
	weight_param = weight;
}
void PCA_set_add(Mat& PCA_set, Mat img){
	Mat PCA_set_row;
	img.convertTo(PCA_set_row, CV_32F);
	#ifdef NORMALIZE
	normalize(PCA_set_row, PCA_set_row);
	#endif
	if(PCA_set.data == NULL){
		PCA_set_row.reshape(0,1).copyTo(PCA_set);
	}
	else{
		PCA_set.push_back(PCA_set_row.reshape(0,1));
	}
}
void PCA_set_add(Mat& PCA_set, string filename){
	Mat example = imread(filename, 0);
	if (example.data == NULL) {
        cout << "could not read " << filename << endl;
        return;
    }
    Mat aptly_sized_example;
	resize(example, aptly_sized_example, Size(EXAMPLE_WIDTH, EXAMPLE_HEIGHT));
	PCA_set_add(PCA_set, aptly_sized_example);
}

bool isFilled(string filename){
	return filename.find("filled") != string::npos;
}
bool isEmpty(string filename){
	return filename.find("empty") != string::npos;
}

//This trains the PCA classifer by query
//TODO: Clean this up and made it possible to pass in predicate functions
void train_PCA_classifier(vector<string>& include,vector<string>& exclude) {
	vector<string> filenames;
	
	string training_examples("training_examples");
	CrawlFileTree(training_examples, filenames);
	vector<string> filenames_copy(filenames.begin(), filenames.end());
	vector<string>::iterator it_filled = remove_if(filenames.begin(), filenames.end(), isEmpty);
	vector<string>::iterator it_empty = remove_if(filenames_copy.begin(), filenames_copy.end(), isFilled);

	vector<string>::iterator it;
	for(it = filenames_copy.begin(); it != it_empty; it++) {
		cout << (*it) << endl;
	}
	/*
	empty_filenames.push_back("training_examples/empty.png");
	empty_filenames.push_back("training_examples/empty_suspect_dim.png");
	empty_filenames.push_back("training_examples/empty_template_bright.png");
	filled_filenames.push_back("training_examples/filled_barely_blue.png");
	filled_filenames.push_back("training_examples/filled_full_black.png");
	filled_filenames.push_back("training_examples/filled_barely_outside_blue.png");
	filled_filenames.push_back("training_examples/filled_full_outside_black.png");
	filled_filenames.push_back("training_examples/filled_full_outside_blue.png");
	filled_filenames.push_back("training_examples/filled_partial_blue.png");
	filled_filenames.push_back("training_examples/filled_partial_outside_black.png");
	*/
	
	Mat PCA_set;
	for (it = filenames_copy.begin(); it != it_empty; it++) {
		PCA_set_add(PCA_set, (*it));
		training_bubble_values.push_back(EMPTY_BUBBLE);
	}
	for(it = filenames.begin(); it != it_filled; it++) {
		PCA_set_add(PCA_set, (*it));
		training_bubble_values.push_back(FILLED_BUBBLE);
	}
	my_PCA = PCA(PCA_set, Mat(), CV_PCA_DATA_AS_ROW, EIGENBUBBLES);
	comparison_vectors = my_PCA.project(PCA_set);
}
//Train the PCA_classifier using example_strip.jpg
//If example_strip.jpg is altered you must make the corresponding
//changes to training_bubble_values in the code below
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

	Mat example_strip_bw = imread("example_strip.jpg", 0);

	int numexamples = example_strip_bw.cols / EXAMPLE_WIDTH;
	Mat PCA_set;
	for (int i = 0; i < numexamples; i++) {
		PCA_set_add(PCA_set, example_strip_bw(Rect(i * EXAMPLE_WIDTH, 0, EXAMPLE_WIDTH, EXAMPLE_HEIGHT)));
	}
	my_PCA = PCA(PCA_set, Mat(), CV_PCA_DATA_AS_ROW, EIGENBUBBLES);
	comparison_vectors = my_PCA.project(PCA_set);
}
//Rate a location on how likely it is to be a bubble.
//The rating is the SSD of the queried pixels and their PCA back projection,
//so lower ratings mean more bubble like.
double rateBubble(Mat& det_img_gray, Point bubble_location) {
    Mat query_pixels, pca_components;
    
    #ifdef USE_GET_RECT_SUB_PIX
	getRectSubPix(det_img_gray, Size(EXAMPLE_WIDTH, EXAMPLE_HEIGHT), bubble_location, query_pixels);
	query_pixels.reshape(0,1).convertTo(query_pixels, CV_32F);
	#else
	det_img_gray(Rect(bubble_location-Point(EXAMPLE_WIDTH/2, EXAMPLE_HEIGHT/2), Size(EXAMPLE_WIDTH, EXAMPLE_HEIGHT))).convertTo(query_pixels, CV_32F);
	query_pixels = query_pixels.reshape(0,1);
	#endif
	
	#ifdef NORMALIZE
	normalize(query_pixels, query_pixels);
	#endif
    pca_components = my_PCA.project(query_pixels);
    Mat out = my_PCA.backProject(pca_components)- query_pixels;
    return sum(out.mul(out)).val[0];
}
//This bit of code finds the location in the search_window most likely to be a bubble
//then it checks that rather than the exact specified location.
//This section probably slows things down by quite a bit and it might not provide significant
//improvement to accuracy. We will need to run some tests to find out if it's worth keeping.
Point bubble_align(Mat& det_img_gray, Point bubble_location){
	Mat out = Mat::zeros(Size(search_window.x*2 + 1, search_window.y*2 + 1) , CV_32FC1);
	Point offset = Point(bubble_location.x - search_window.x, bubble_location.y - search_window.y);
	
	for(size_t i = 0; i <= search_window.x*2; i+=1) {
		for(size_t j = 0; j <= search_window.y*2; j+=1) {
			out.col(i).row(j) += rateBubble(det_img_gray, Point(i,j) + offset);
		}
	}
	//Multiplying by a 2D gaussian weights the search so that we are more likely to choose
	//locations near the expected bubble location.
	//However, the sigma parameter probably needs to be tweaked.
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
//Compare the specified bubble with all the training bubbles via PCA.
bubble_val classifyBubble(Mat& det_img_gray, Point bubble_location) {
	Mat query_pixels;

    #ifdef USE_GET_RECT_SUB_PIX
	getRectSubPix(det_img_gray, Size(EXAMPLE_WIDTH, EXAMPLE_HEIGHT), bubble_location, query_pixels);
	#else
	query_pixels = det_img_gray(Rect(bubble_location-Point(EXAMPLE_WIDTH/2, EXAMPLE_HEIGHT/2), Size(EXAMPLE_WIDTH, EXAMPLE_HEIGHT)));
	#endif
	#ifdef OUTPUT_BUBBLE_IMAGES
	string segfilename = classifierNamer.get_unique_name("bubble_");
	segfilename.append(".jpg");
	imwrite(segfilename, query_pixels);
	#endif
	query_pixels.convertTo(query_pixels, CV_32F);
	query_pixels = query_pixels.reshape(0,1);
	
	#ifdef NORMALIZE
	normalize(query_pixels, query_pixels);
	#endif
	
	//Here we find the best filled and empty matches in the PCA training set.
	Mat responce, out;
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
