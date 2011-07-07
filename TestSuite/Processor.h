/*
Header file for image processing functions.
*/
#ifndef IMAGEPROCESSING_H
#define IMAGEPROCESSING_H
#include "configuration.h"

#ifdef USE_ANDROID_HEADERS_AND_IO
#include <opencv2/core/core.hpp>
#else
#include "cv.h"
#endif

#include <json/value.h>
#include "PCA_classifier.h"

class Processor{
	//TODO: Modify this class to use pimpl idiom as advised in effective c++
	Json::Value root;
	cv::Mat formImage;
	PCA_classifier classifier;
	public:
		Processor(const char* templatePath);
		virtual ~Processor();
		
		//These two probably don't need to be exposed except for in the testing suite
		bool trainClassifier();
		void setClassifierWeight(float weight);
		
		bool loadForm(const char* imagePath);
		//Maybe instead of exposing alignForm I should just expose a method
		//for finding and displaying the contour.
		bool alignForm(const char* alignedImageOutputPath);
		bool processForm(const char* outPath);
	private:
		Json::Value classifySegment(const Json::Value &bubbleLocations, cv::Mat &segment);
	 	void getAlignedSegment(const Json::Value &segmentTemplate, cv::Mat &alignedSegment);
};


#endif
