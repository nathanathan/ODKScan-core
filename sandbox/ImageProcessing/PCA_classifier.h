#ifndef PCA_CLASSIFIER_H
#define PCA_CLASSIFIER_H
#include "configuration.h"

#ifdef USE_ANDROID_HEADERS_AND_IO

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "image_pool.h"

#else
#include "cv.h"
#endif

#if 0
#define EXAMPLE_WIDTH 28
#define EXAMPLE_HEIGHT 36
#else
#define EXAMPLE_WIDTH 14
#define EXAMPLE_HEIGHT 18
#endif

enum bubble_val { EMPTY_BUBBLE = 0, PARTIAL_BUBBLE, FILLED_BUBBLE, NUM_BUBBLE_VALS };

template <class Tp>
bool returnTrue(Tp& filename){
	return true;
}

class PCA_classifier
{
	cv::Mat comparison_vectors;
	vector <bubble_val> training_bubble_values;
	cv::PCA my_PCA;
	
	cv::Point search_window;

	//A matrix for precomputing gaussian weights for the search window
	cv::Mat gaussian_weights;

	//The weights Mat can be used to bias the classifier
	//Each element corresponds to a classification.
	cv::Mat weights;
	public:
		PCA_classifier();
		void set_weight(bubble_val classification, float weight);
		void set_search_window(cv::Point sw);
		double rateBubble(cv::Mat& det_img_gray, cv::Point bubble_location);
		void train_PCA_classifier(bool (*pred)(string& filename) = &returnTrue);
		cv::Point bubble_align(cv::Mat& det_img_gray, cv::Point bubble_location);
		bubble_val classifyBubble(cv::Mat& det_img_gray, cv::Point bubble_location);
		virtual ~PCA_classifier() {
			//Not sure if I need to do anything here...
			//And what does virtual mean?
		}
	private:
		void update_gaussian_weights();
		void PCA_set_add(cv::Mat& PCA_set, cv::Mat& img);
		void PCA_set_add(cv::Mat& PCA_set, string& filename);
};

#endif
