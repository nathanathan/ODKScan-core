/*
 * include the headers required by the generated cpp code
 */
%{
#include "Processor.h"

using namespace cv;
%}

//import the android-cv.i file so that swig is aware of all that has been previous defined
//notice that it is not an include....
%import "android-cv.i"

//make sure to import the image_pool as it is 
//referenced by the Processor java generated
//class
%typemap(javaimports) Processor "
import com.opencv.jni.Mat;

/** Processor - for processing images that are stored in an image pool
*/"

class Processor{
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
		bool alignForm(const char* alignedImageOutputPath);
		bool processForm(const char* outPath);
	private:
		Json::Value classifySegment(const Json::Value &bubbleLocations, cv::Mat &segment);
	 	void getAlignedSegment(const Json::Value &segmentTemplate, cv::Mat &alignedSegment);
};
