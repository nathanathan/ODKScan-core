/*
Header file for image processing functions.
*/
#ifndef IMAGEPROCESSING_H
#define IMAGEPROCESSING_H
#include "configuration.h"
#include <tr1/memory>
/*
This class handles most of the JSON parsing and provides an interface to the overall process.
*/
class Processor{
	public:
		//Functions are specified in the order they should be invoked.
		Processor();
		bool setForm(const char* imagePath, int rotate90 = 0);
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
