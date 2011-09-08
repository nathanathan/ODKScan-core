/*
Header file for image processing functions.
*/
#ifndef IMAGEPROCESSING_H
#define IMAGEPROCESSING_H
#include "configuration.h"
#include <tr1/memory>

/*
This class handles most of the JSON parsing and provides an interface to the image processing pipeline.
*/
class Processor{
	public:
		//Functions are specified in the order they should be invoked.
		
		Processor();
		/*
		hello world.
		*/
		bool loadFormImage(const char* imagePath, bool undistort = false);
		/**
		loadFeatureData - hello world 2.
		*/
		bool loadFeatureData(const char* templatePath);
		int detectForm();
		bool setTemplate(const char* templatePath);
		bool alignForm(const char* alignedImageOutputPath, int templateIdx = 0);
		bool processForm(const char* outPath) const;
		bool writeFormImage(const char* outputPath) const;

	private:
		class ProcessorImpl;
    	std::tr1::shared_ptr<ProcessorImpl> processorImpl;
};


#endif
