/*
Header file for image processing functions.
*/
#ifndef IMAGEPROCESSING_H
#define IMAGEPROCESSING_H
#include "configuration.h"

#include <json/value.h>
#include <tr1/memory>

class Processor{
	//TODO: Modify this class to use pimpl idiom as advised in effective c++

	public:
		Processor(const char* templatePath);
		//virtual ~Processor();
		
		//These two probably don't need to be exposed except for in the testing suite
		bool trainClassifier();
		void setClassifierWeight(float weight);
		
		bool loadForm(const char* imagePath);
		//Maybe instead of exposing alignForm I should just expose a method
		//for finding and displaying the contour.
		bool alignForm(const char* alignedImageOutputPath);
		bool processForm(const char* outPath);
		bool markupForm(const char* bvPath, const char* outputPath);
		bool writeFormImage(const char* outputPath);
		
	private:
		class ProcessorImpl;
    	std::tr1::shared_ptr<ProcessorImpl> processorImpl;
		//Json::Value root;
		//cv::Mat formImage;
		//PCA_classifier classifier;
		//Json::Value classifySegment(const Json::Value &bubbleLocations, cv::Mat &segment, cv::Mat &transformation, cv::Point offset);
	 	//Json::Value getAlignedSegment(const Json::Value &segmentTemplate, cv::Mat &alignedSegment, cv::Mat &transformation, cv::Point& offset);
};


#endif
