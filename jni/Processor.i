/*
 * include the headers required by the generated cpp code
 */
%{
#include "Processor.h"

//using namespace cv;
%}

//import the android-cv.i file so that swig is aware of all that has been previous defined
//notice that it is not an include....
%import "android-cv.i"

//make sure to import the image_pool as it is 
//referenced by the Processor java generated
//class
%typemap(javaimports) Processor "
//import com.opencv.jni.Mat;

/** Processor - for processing images that are stored in an image pool
*/"
class Processor{
	//TODO: Modify this class to use pimpl idiom as advised in effective c++

	public:
		Processor(const char* templatePath);
		bool trainClassifier();
		void setClassifierWeight(float weight);
		
		bool loadForm(const char* imagePath);
		bool alignForm(const char* alignedImageOutputPath);
		bool processForm(const char* outPath);
		bool markupForm(const char* bvPath, const char* outputPath);
		bool writeFormImage(const char* outputPath);
		
	private:
		class ProcessorImpl;
    	std::tr1::shared_ptr<ProcessorImpl> processorImpl;
};
