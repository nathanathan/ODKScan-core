#ifndef PCA_CLASSIFIER_H
#define PCA_CLASSIFIER_H
#include "configuration.h"

#ifdef USE_ANDROID_HEADERS_AND_IO
//I don't think i need this....
//#include "image_pool.h"
#endif

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "Addons.h"

//TODO: Add a 4th classification. The middle classification doesn't really help since it has to be entirely
//		lumped with empty or full...
enum bubble_val { EMPTY_BUBBLE = 0, PARTIAL_BUBBLE, FILLED_BUBBLE, NUM_BUBBLE_VALS };

class PCA_classifier
{
	private:	
		int eigenvalues;
		
		cv::Mat comparison_vectors;
		std::vector <bubble_val> training_bubble_values;
		cv::PCA my_PCA;
	
		cv::Size search_window;

		//A matrix for precomputing gaussian weights for the search window
		cv::Mat gaussian_weights;

		//The weights Mat can be used to bias the classifier
		//Each element corresponds to a classification.
		cv::Mat weights;
		
		void update_gaussian_weights();
		void PCA_set_add(cv::Mat& PCA_set, const cv::Mat& img);
		void PCA_set_add(cv::Mat& PCA_set, const std::string& filename, bool flipExamples);
	public:
		//TODO: I'm running into problems with the ex_width+height.
		//		Should the width and height be measured from the edges of the bubble than have a additional buffer?
		//		What if I want to specify arbitrary rectangles that should be resized then classified?
		cv::Size exampleSize;
		PCA_classifier();
		PCA_classifier(int eigvenvalues);
		
		void set_weight(bubble_val classification, float weight);
		void set_search_window(cv::Size sw);
		double rateBubble(const cv::Mat& det_img_gray, const cv::Point& bubble_location);
		bool train_PCA_classifier(const std::string& dirPath, cv::Size myExampleSize = cv::Size(14,18),
									bool flipExamples = false,
									bool (*pred)(std::string& filename) = &returnTrue);
		cv::Point bubble_align(const cv::Mat& det_img_gray, const cv::Point& bubble_location);
		bubble_val classifyBubble(const cv::Mat& det_img_gray, const cv::Point& bubble_location);
		
		bool trained();
		
		virtual ~PCA_classifier() {
			//Not sure if I need to do anything here...
			//Do the class variables above automatically get destructed?
		}

};

#endif
