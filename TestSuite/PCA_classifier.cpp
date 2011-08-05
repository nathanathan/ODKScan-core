#include "configuration.h"
#include "PCA_classifier.h"
#include "FileUtils.h"

#include <opencv2/highgui/highgui.hpp>

#include <iostream>

#ifdef OUTPUT_BUBBLE_IMAGES
#include "NameGenerator.h"
extern NameGenerator namer;
#endif

#ifdef OUTPUT_EXAMPLES
#include "NameGenerator.h"
NameGenerator exampleNamer;
#endif

#define NORMALIZE

//getRectSubPix might slow things down, but provides 2 advanges over the roi method
//1. If the example images have an odd dimension it will linearly interpolate the
//   half pixel error.
//2. If the rectangle crosses the image boundary (because of a large search window)
//   it won't give an error.
//#define USE_GET_RECT_SUB_PIX

//#define USE_MASK
//Using a mask seems to slightly help bubble alignment in the average case but cause some additional misses.

//#define DISABLE_PCA
//So PCA definately does help, but only by a few percent and speedwise...


using namespace std;
using namespace cv;

template <class Tp>
int vectorFind(const vector<Tp>& vec, const Tp& element) {
	for(size_t i = 0; i < vec.size(); i++) {
		if( vec[i] == element ) {
			return i;
		}
	}
	return -1;
}
//The gaussian intensity isn't correctly scaling...
void PCA_classifier::update_gaussian_weights() {
	float sigma = .5; //increasing decreases spread.

	//This precomputes the gaussian for subbbubble alignment.
	int height = search_window.height;
	int width = search_window.width;
	if(height == 0 || width == 0){
		gaussian_weights.release();
		return;
	}
	Mat v_gauss = getGaussianKernel(height, float(height) * sigma, CV_32F);
	Mat h_gauss;
	transpose(getGaussianKernel(width, float(width) * sigma, CV_32F), h_gauss);
	v_gauss = repeat(v_gauss, 1, width);
	h_gauss = repeat(h_gauss, height, 1);
	Mat temp = v_gauss.mul(h_gauss);
	double temp_max;
	minMaxLoc(temp, NULL, &temp_max);
	gaussian_weights = temp_max - temp + .001;//.001 is to avoid roundoff problems, it might not be necessiary.
}
void PCA_classifier::set_search_window(Size sw) {
	search_window = sw;
	update_gaussian_weights();
}
int PCA_classifier::getClassificationIdx(const string& filepath) {
	int nameIdx = filepath.find_last_of("/");
	string filename = filepath.substr(nameIdx + 1, filepath.size() - nameIdx);
	string classification = filename.substr(0, filename.find_first_of("_"));
	int classificationIdx = vectorFind(classifications, classification);
	
	if(vectorFind(classifications, classification) < 0) {
		classifications.push_back(classification);
		classificationIdx = classifications.size();
		#ifdef DEBUG_CLASSIFIER
			cout << "Adding classification: " << classification << endl;
		#endif
	}
	
	return classificationIdx;
}
//Add an image Mat to a PCA_set performing the 
//necessairy reshaping and type conversion.
void PCA_classifier::PCA_set_push_back(Mat& PCA_set, const Mat& img) {
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
void PCA_classifier::PCA_set_add(Mat& PCA_set, vector<int>& trainingBubbleValues, const string& filename, bool flipExamples) {

	Mat example = imread(filename, 0);
	if (example.data == NULL) {
        cerr << "could not read " << filename << endl;
        return;
    }
    Mat aptly_sized_example;
	resize(example, aptly_sized_example, Size(exampleSize.width, exampleSize.height), 0, 0, INTER_AREA);

	#ifdef USE_MASK
		aptly_sized_example = aptly_sized_example & cMask;
	#endif

	#ifdef OUTPUT_EXAMPLES
		string outfilename = filename.substr(filename.find_last_of("/"), filename.size() - filename.find_last_of("/"));
		imwrite("example_images_used" + outfilename, aptly_sized_example);
	#endif
	
	int classificationIdx = getClassificationIdx(filename);
	
	PCA_set_push_back(PCA_set, aptly_sized_example);
	trainingBubbleValues.push_back(classificationIdx);
	
	if(flipExamples){
		for(int i = -1; i < 2; i++){
			Mat temp;
			flip(aptly_sized_example, temp, i);
			PCA_set_push_back(PCA_set, temp);
			trainingBubbleValues.push_back(classificationIdx);
		}
	}
}
template <class Tp>
void writeVector(FileStorage& fs, const string& label, const vector<Tp>& vec) {
	fs << label << "[";
	for(size_t i = 0; i < vec.size(); i++) {
		fs << vec[i];
	}
	fs << "]";
}
vector<string> readVector(FileStorage& fs, const string& label) {
	FileNode fn = fs[label];
	vector<string> vec(fn.size());
	if(fn[0].isString()){
		for(size_t i = 0; i < fn.size(); i++) {
			fn[i] >> vec[i];
		}
	}
	return vec;
}
bool PCA_classifier::save(const string& outputPath) const {

	try {
		FileStorage fs(outputPath, FileStorage::WRITE);
		fs << "exampleSizeWidth" << exampleSize.width;
		fs << "exampleSizeHeight" << exampleSize.height;
		fs << "PCAmean" << my_PCA.mean;
		fs << "PCAeigenvectors" << my_PCA.eigenvectors;
		fs << "PCAeigenvalues" << my_PCA.eigenvalues;
		writeVector(fs, "classifications", classifications);
		statClassifier.write(*fs, "classifierData" );
	}
	catch( cv::Exception& e ) {
		const char* err_msg = e.what();
		cout << err_msg << endl;
		return false;
	}
	return true;
}
bool PCA_classifier::load(const string& inputPath, const Size& requiredExampleSize) {
	if( !fileExists(inputPath) ) return false;
	try {
		FileStorage fs(inputPath, FileStorage::READ);
		fs["exampleSizeWidth"] >> exampleSize.width;
		fs["exampleSizeHeight"] >> exampleSize.height;
		if(requiredExampleSize != exampleSize){
			return false; //is this ok in a try expression?
		}
		fs["PCAmean"] >> my_PCA.mean;
		fs["PCAeigenvectors"] >> my_PCA.eigenvectors;
		fs["PCAeigenvalues"] >> my_PCA.eigenvalues;
		
		classifications = readVector(fs, "classifications");
		search_window = exampleSize;
		update_gaussian_weights();
		statClassifier.clear();
		statClassifier.read(*fs, cvGetFileNodeByName(*fs, cvGetRootFileNode(*fs), "classifierData") );
	}
	catch( cv::Exception& e ) {
		const char* err_msg = e.what();
		cout << err_msg << endl;
		return false;
	}
	return true;
}
bool PCA_classifier::train_PCA_classifier(const vector<string>& examplePaths, const Size& myExampleSize,
											int eigenvalues, bool flipExamples) {
	statClassifier.clear();
	weights = (Mat_<float>(3,1) << 1, 1, 1);//TODO: fix the weighting stuff 
	exampleSize = Size(myExampleSize);//the copy constructor might be necessairy.
	search_window = myExampleSize;
	update_gaussian_weights();

	#ifdef USE_MASK
		cMask = gaussian_weights < .002;
		/*
		namedWindow("outliers", CV_WINDOW_NORMAL);
		imshow( "outliers", cMask );
		
		for(;;)
		{
		    char c = (char)waitKey(0);
		    if( c == '\x1b' ) // esc
		    {
		    	cvDestroyWindow("inliers");
		    	cvDestroyWindow("outliers");
		        break;
		    }
		}
		*/
	#endif

	Mat PCA_set;
	vector<int> trainingBubbleValues;
	for(size_t i = 0; i < examplePaths.size(); i++) {
		PCA_set_add(PCA_set, trainingBubbleValues, examplePaths[i], flipExamples);
	}

	if(PCA_set.rows < eigenvalues) return false;//Not completely sure about this...

	my_PCA = PCA(PCA_set, Mat(), CV_PCA_DATA_AS_ROW, eigenvalues);
	Mat comparisonVectors = my_PCA.project(PCA_set);
	
	Mat trainingBubbleValuesMat(1,1,CV_32SC1);
	trainingBubbleValuesMat.at<int>(0) = trainingBubbleValues[0];
	for(size_t i = 1; i < trainingBubbleValues.size(); i++){
		trainingBubbleValuesMat.push_back( trainingBubbleValues[i] );
	}

	#ifdef DISABLE_PCA
		statClassifier.train_auto(PCA_set, trainingBubbleValuesMat, Mat(), Mat(), CvSVMParams());
		return true;
	#endif
	
	statClassifier.train_auto(comparisonVectors, trainingBubbleValuesMat, Mat(), Mat(), CvSVMParams());
	
	return true;
}
//Rate a location on how likely it is to be a bubble.
//The rating is the SSD of the queried pixels and their PCA back projection,
//so lower ratings mean more bubble like.
inline double PCA_classifier::rateBubble(const Mat& det_img_gray, const Point& bubble_location) const {

    Mat query_pixels, pca_components;

    #ifdef USE_GET_RECT_SUB_PIX
		getRectSubPix(det_img_gray, exampleSize, bubble_location, query_pixels);
	#else
		Rect window = Rect(bubble_location - Point(exampleSize.width/2, exampleSize.height/2), exampleSize) & Rect(Point(0,0), det_img_gray.size());
		query_pixels = Mat::zeros(exampleSize, det_img_gray.type());
		query_pixels(Rect(Point(0,0), window.size())) += det_img_gray(window);
	#endif

	#ifdef USE_MASK
		query_pixels = query_pixels & cMask;
	#endif
	
	query_pixels.reshape(0,1).convertTo(query_pixels, CV_32F);

	#ifdef NORMALIZE
		normalize(query_pixels, query_pixels);
	#endif
	
	pca_components = my_PCA.project(query_pixels);

	#if 0
		Mat out;
		matchTemplate(pca_components, my_PCA.backProject(pca_components), out, CV_TM_SQDIFF_NORMED);
		return out.at<float>(0,0);
    #endif
	Mat out = my_PCA.backProject(pca_components) - query_pixels;
	return sum(out.mul(out)).val[0];
}
#define USE_HILLCLIMBING
#ifdef USE_HILLCLIMBING
//This using a hillclimbing algorithm to find the location with the highest bubble rating.
//It might only find a local instead of global minimum but it is much faster.
Point PCA_classifier::bubble_align(const Mat& det_img_gray, const Point& bubble_location) const {
	int iterations = 10;

	Mat sofar = Mat::zeros(Size(2*iterations+1, 2*iterations+1), CV_8UC1);
	Point offset = Point(bubble_location.x - iterations, bubble_location.y - iterations);
	Point loc = Point(iterations, iterations);
	
	double minDirVal = 100.;
	while( iterations > 0 ){
		Point minDir(0,0);
		for(int i = loc.x-1; i <= loc.x+1; i++) {
			for(int j = loc.y-1; j <= loc.y+1; j++) {
				if(sofar.at<uchar>(j,i) != 123) {
					sofar.at<uchar>(j,i) = 123;
					
					double rating = rateBubble(det_img_gray, Point(i,j) + offset);
					rating *= MAX(1, norm(loc - Point(iterations, iterations)));
					
					if(rating <= minDirVal){
						minDirVal = rating;
						minDir = Point(i,j) - loc;
					}
					
				}
			}
		}
		if(minDir.x == 0 && minDir.y == 0){
			break;
		}
		loc += minDir;
		iterations--;
	}
	#if 0
	namedWindow("outliers", CV_WINDOW_NORMAL);
	imshow( "outliers", sofar );
	
	for(;;)
	{
	    char c = (char)waitKey(0);
	    if( c == '\x1b' ) // esc
	    {
	    	cvDestroyWindow("inliers");
	    	cvDestroyWindow("outliers");
	        break;
	    }
	}
	#endif
	return loc + offset;
}

#else
//This bit of code finds the location in the search_window most likely to be a bubble
//then it checks that rather than the exact specified location.
//This section probably slows things down by quite a bit and it might not provide significant
//improvement to accuracy. We will need to run some tests to find out if it's worth using.
Point PCA_classifier::bubble_align(const Mat& det_img_gray, const Point& bubble_location) const {
	if(search_window.width == 0 || search_window.height == 0){
		return bubble_location;
	}

	Mat out(search_window, CV_32F);
	Point offset = Point(bubble_location.x - search_window.width/2, bubble_location.y - search_window.height/2);
	
	for(int i = 0; i < search_window.width; i++) {
		for(int j = 0; j < search_window.height; j++) {
			out.at<float>(j,i) = rateBubble(det_img_gray, Point(i,j) + offset);
		}
	}
	out = out.mul(gaussian_weights);
	
	/*
	out = out.mul(gaussian_weights);
	Mat temp;
	medianBlur(out, temp, 5);
	out = temp;
	*/
	
	Point min_location;
	minMaxLoc(out, NULL,NULL, &min_location);
	
	return min_location + offset;
}
#endif

inline bool isFilled( int val ) {
	return val != 1;
}
//Compare the specified bubble with all the training bubbles via PCA.
bool PCA_classifier::classifyBubble(const Mat& det_img_gray, const Point& bubble_location) const {
	Mat query_pixels;
    #ifdef USE_GET_RECT_SUB_PIX
		getRectSubPix(det_img_gray, Size(exampleSize.width, exampleSize.height), bubble_location, query_pixels);
	#else									 
		Rect window = Rect(bubble_location - Point(exampleSize.width/2, exampleSize.height/2), exampleSize) & Rect(Point(0,0), det_img_gray.size());
		query_pixels = Mat::zeros(exampleSize, det_img_gray.type());
		query_pixels(Rect(Point(0,0), window.size())) += det_img_gray(window);
	#endif
	
	#ifdef USE_MASK
		query_pixels = query_pixels & cMask;
	#endif
	
	#ifdef OUTPUT_BUBBLE_IMAGES
		string bubbleName = namer.get_unique_name("bubble_");
		imwrite("bubble_images/" + bubbleName + ".jpg", query_pixels);
	#endif
	
	query_pixels.convertTo(query_pixels, CV_32F);
	query_pixels = query_pixels.reshape(0,1);
	
	#ifdef NORMALIZE
		normalize(query_pixels, query_pixels);
	#endif
	
	#ifdef DISABLE_PCA
		return isFilled( statClassifier.predict( query_pixels ) );
	#endif
	
	int returnVal = statClassifier.predict( my_PCA.project(query_pixels) );
	return isFilled( returnVal );
}
bool PCA_classifier::trained() {
	return my_PCA.mean.data != NULL;
}
