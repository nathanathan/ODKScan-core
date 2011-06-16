#include "PCA_classifier.h"
#include "highgui.h"

#define EXAMPLE_WIDTH 14
#define EXAMPLE_HEIGHT 18
#define EIGENBUBBLES 5

using namespace cv;

Mat comparison_vectors;
PCA my_PCA;
vector <bubble_val> training_bubble_values;
float weight_param = .5;
//Point search_window(6, 8);
Point search_window(0, 0);

//Sets the weight to a value between 0 and 1 to bias
//the classifier towards filled 0? or empty 1?
void set_weight(float weight){
	assert(weight >= 0 && weight <= 1);
	weight_param = weight;
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
