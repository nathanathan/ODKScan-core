#ifndef PCA_CLASSIFIER_H
#define PCA_CLASSIFIER_H
#include "configuration.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/ml/ml.hpp>
#include "Addons.h"

class PCA_classifier
{
	private:	
		
		CvSVM statClassifier;
		//TODO: try SVM without PCA
		vector<string> classifications; 
		
		cv::PCA my_PCA;
	
		cv::Size search_window;

		//A matrix for precomputing gaussian weights for the search window
		cv::Mat gaussian_weights;

		//The weights Mat can be used to bias the classifier
		//Each element corresponds to a classification.
		cv::Mat weights;
		
		void update_gaussian_weights();
		
		int getClassificationIdx(const std::string& filepath);
		
		void PCA_set_push_back(cv::Mat& PCA_set, const cv::Mat& img);
		void PCA_set_add(cv::Mat& PCA_set, std::vector<int>& trainingBubbleValues, const std::string& filename, bool flipExamples);
	public:
		cv::Size exampleSize; //Can I make this immutable to clients (without an accesor function)?

		void set_search_window(cv::Size sw);
		double rateBubble(const cv::Mat& det_img_gray, const cv::Point& bubble_location);
		bool train_PCA_classifier(	const std::vector<std::string>& examplePaths,
									const cv::Size& myExampleSize = cv::Size(14,18),
									int eigenvalues = 7,
									bool flipExamples = false);
		cv::Point bubble_align(const cv::Mat& det_img_gray, const cv::Point& bubble_location);
		bool classifyBubble(const cv::Mat& det_img_gray, const cv::Point& bubble_location);
		
		bool trained();

};

#endif
