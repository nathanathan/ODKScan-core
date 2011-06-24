#include "configuration.h"
#include "PCA_classifier.h"
#include "FileUtils.h"
#include "nameGenerator.h"
#include <iostream>

//Number of eigenvalues to generate for the PCA
#define EIGENBUBBLES 5

//Normalizing everything that goes into the PCA *might* help with lighting problems
//But so far just seems to hurt accuracy.
//#define NORMALIZE

//getRectSubPix might slow things down, but provides 2 advanges over the roi method
//1. If the example images have an odd dimension it will linearly interpolate the
//   half pixel error.
//2. If the rectangle crosses the image boundary (because of a large search window)
//   it won't give an error.
#define USE_GET_RECT_SUB_PIX


//TestSuite.h will be the file that defines this:
#ifdef USE_ANDROID_HEADERS_AND_IO

	#include <opencv2/highgui/highgui.hpp>
	//I suspect the following path will need to change
	#define TRAINING_EXAMPLE_DIRECTORY "training_examples"

#else //If we are compiling for the test suite
	
	#include "highgui.h"

	#define TRAINING_EXAMPLE_DIRECTORY "training_examples"

	#define OUTPUT_BUBBLE_IMAGES
	#define OUTPUT_EXAMPLES

	#ifdef OUTPUT_BUBBLE_IMAGES
	NameGenerator classifierNamer("bubble_images/");
	#endif

	#ifdef OUTPUT_EXAMPLES
	NameGenerator exampleNamer("example_images_used/");
	#endif

#endif


using namespace cv;

void PCA_classifier::update_gaussian_weights(){
	//This precomputes the gaussian for subbbubble alignment.
	int height = 2*search_window.y + 1;
	int width = 2*search_window.x + 1;
	Mat v_gauss = getGaussianKernel(height, float(height)*.5, CV_32F);
	Mat h_gauss;
	transpose(getGaussianKernel(width, float(width)*.5, CV_32F), h_gauss);
	v_gauss = repeat(v_gauss, 1, width);
	h_gauss = repeat(h_gauss, height, 1);
	Mat temp = v_gauss.mul(h_gauss);
	double temp_max;
	minMaxLoc(temp, NULL, &temp_max);
	gaussian_weights = temp_max - temp + .001;//.001 is to avoid roundoff problems, it might not be necessiary.
}
PCA_classifier::PCA_classifier() {
	search_window = Point(6, 8);
	weights = (Mat_<float>(3,1) << 1, 1, 1);
	update_gaussian_weights();
}

void PCA_classifier::set_weight(bubble_val classification, float weight) {
	weights.at<float>((int)classification, 0) = weight;
}
void PCA_classifier::set_search_window(Point sw) {
	search_window = sw;
	update_gaussian_weights();
}
//Add an image Mat to a PCA_set performing the 
//necessairy reshaping and type conversion.
void PCA_classifier::PCA_set_add(Mat& PCA_set, Mat& img) {
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
//Loads a image with the specified filename and adds it to the PCA set.
//Classifications are inferred from the filename and added to training_bubble_values.
void PCA_classifier::PCA_set_add(Mat& PCA_set, string& filename) {
	Mat example = imread(filename, 0);
	if (example.data == NULL) {
        cout << "could not read " << filename << endl;
        return;
    }
    Mat aptly_sized_example;
	resize(example, aptly_sized_example, Size(EXAMPLE_WIDTH, EXAMPLE_HEIGHT), INTER_CUBIC);

	#ifdef OUTPUT_EXAMPLES
	string outfilename = exampleNamer.get_unique_name("bubble_");
	outfilename.append(".jpg");
	imwrite(outfilename, aptly_sized_example);
	#endif
	
	PCA_set_add(PCA_set, aptly_sized_example);
	if( filename.find("filled") != string::npos ) {
		training_bubble_values.push_back(FILLED_BUBBLE);
	}
	else if( filename.find("partial") != string::npos ) {
		training_bubble_values.push_back(PARTIAL_BUBBLE);

	}
	else if( filename.find("empty") != string::npos ) {
		training_bubble_values.push_back(EMPTY_BUBBLE);
	}
}
//This trains the PCA classifer by query.
//A predicate can be supplied for filtering out undesireable filenames
void PCA_classifier::train_PCA_classifier(bool (*pred)(string&)) {
	vector<string> filenames;
	string training_examples(TRAINING_EXAMPLE_DIRECTORY);
	CrawlFileTree(training_examples, filenames);

	Mat PCA_set;
	vector<string>::iterator it;
	for(it = filenames.begin(); it != filenames.end(); it++) {
		if( pred((*it)) ) {
			PCA_set_add(PCA_set, (*it));
		}
	}
	my_PCA = PCA(PCA_set, Mat(), CV_PCA_DATA_AS_ROW, EIGENBUBBLES);
	comparison_vectors = my_PCA.project(PCA_set);
}
//Rate a location on how likely it is to be a bubble.
//The rating is the SSD of the queried pixels and their PCA back projection,
//so lower ratings mean more bubble like.
double PCA_classifier::rateBubble(Mat& det_img_gray, Point bubble_location) {
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
Point PCA_classifier::bubble_align(Mat& det_img_gray, Point bubble_location) {
	Mat out = Mat::zeros(Size(search_window.x*2 + 1, search_window.y*2 + 1) , CV_32F);
	Point offset = Point(bubble_location.x - search_window.x, bubble_location.y - search_window.y);
	
	for(size_t i = 0; i <= search_window.x*2; i+=1) {
		for(size_t j = 0; j <= search_window.y*2; j+=1) {
			out.col(i).row(j) += rateBubble(det_img_gray, Point(i,j) + offset);
		}
	}
	out = out.mul(gaussian_weights);
	
	Point min_location;
	minMaxLoc(out, NULL,NULL, &min_location);
	return min_location + offset;
}
//Compare the specified bubble with all the training bubbles via PCA.
bubble_val PCA_classifier::classifyBubble(Mat& det_img_gray, Point bubble_location) {
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
	vector<float> max_responces(NUM_BUBBLE_VALS, 0);
	for(size_t i = 0; i < training_bubble_values.size(); i+=1) {
		float current_responce = sum(out.row(i)).val[0]; //This only uses the sum function for convenience
		if(current_responce > max_responces[training_bubble_values[i]]) {
			max_responces[training_bubble_values[i]] = current_responce;
		}
	}
	//Here we weight and compare the responces for all classifications,
	//returning the classification with the maximum weighted responce.
	//To use neighboring bubbles in classification we will probably want this method
	//to return the scores without comparing them.
	Point max_location;
	minMaxLoc(Mat(max_responces).mul(weights), NULL, NULL, NULL, &max_location);
	return (bubble_val) max_location.y;
}
