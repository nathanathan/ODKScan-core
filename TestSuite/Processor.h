/*
Header file for image processing functions.
*/
#ifndef IMAGEPROCESSING_H
#define IMAGEPROCESSING_H
#include "configuration.h"

#include <json/value.h>
#include <tr1/memory>

class Processor{
	public:
		Processor(const char* templatePath);
		
		//These two probably don't need to be exposed except for in the testing suite
		bool trainClassifier(const char* trainingImageDir);
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
		//I think using the shared pointer make it so the deconstructor will automatically be called for the implementation
		//when it is called for the procesor class.
    	std::tr1::shared_ptr<ProcessorImpl> processorImpl;
};


#endif
