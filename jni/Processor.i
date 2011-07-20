/*
 * include the headers required by the generated cpp code
 */
%{
#include "Processor.h"
%}

//import the android-cv.i file so that swig is aware of all that has been previous defined
//notice that it is not an include....
%import "android-cv.i"

//make sure to import the image_pool as it is 
//referenced by the Processor java generated
//class
%typemap(javaimports) Processor "

/** Processor - for processing images that are stored in an image pool
*/"
class Processor{
	public:
		Processor(const char* templatePath);
		bool trainClassifier(const char* exampleDir);
		void setClassifierWeight(float weight);
		bool loadForm(const char* imagePath, int rotate90 = 0);
		bool alignForm(const char* alignedImageOutputPath);
		bool processForm(const char* outPath);
		bool writeFormImage(const char* outputPath);
		
	private:
		class ProcessorImpl;
    	std::tr1::shared_ptr<ProcessorImpl> processorImpl;
};
