#ifndef PROCESSOR_H
#define PROCESSOR_H
#include "configuration.h"
#include <tr1/memory>

/*
This class handles most of the JSON parsing and provides an interface to the image processing pipeline.
*/
class Processor{
	public:
		//Note: Functions are specified in the order they should be invoked.
		
		//The constructor takes as an arguement a "root" path that all other paths are relative to.
		//TODO: Make all other paths relative to appRootDir
		//The default constructor sets the root path to ""
		Processor();
		Processor(const char* appRootDir);
		bool loadFormImage(const char* imagePath, bool undistort = false);
		bool loadFeatureData(const char* templatePath);
		int detectForm();
		bool setTemplate(const char* templatePath);
		bool alignForm(const char* alignedImageOutputPath, int templateIdx = 0);
		bool processForm(const char* outPath);
		bool writeFormImage(const char* outputPath) const;

	private:
		class ProcessorImpl;
    		std::tr1::shared_ptr<ProcessorImpl> processorImpl;
};


#endif
