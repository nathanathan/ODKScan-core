/*
Header file for image processing functions.
*/
#ifndef IMAGEPROCESSING_H
#define IMAGEPROCESSING_H
#include "configuration.h"
#include <tr1/memory>

class Processor{
	public:
		Processor();
		bool loadTemplate(const char* templatePath);
		
		//These two probably don't need to be exposed except for in the testing suite
		bool trainClassifier(const char* trainingImageDir);
		
		bool loadForm(const char* imagePath, int rotate90 = 0);
		//Maybe instead of exposing alignForm I should just expose a method
		//for finding and displaying the contour.
		bool alignForm(const char* alignedImageOutputPath);
		bool processForm(const char* outPath);
		bool writeFormImage(const char* outputPath);
		
	private:
		class ProcessorImpl;
    	std::tr1::shared_ptr<ProcessorImpl> processorImpl;
};


#endif
