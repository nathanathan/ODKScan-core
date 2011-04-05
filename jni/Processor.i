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

class Processor {
public:
	Processor();
	virtual ~Processor();

  	char* ProcessForm(char* filename); 
	bool DetectOutline(char* filename, bool fIgnoreDatFile = false);
  	void warpImage(IplImage* img, IplImage* warpImg, CvPoint * cornerPoints);
	std::vector<cv::Point> findBubbles(IplImage* pImage);
  	CvPoint * findLineValues(IplImage* img);
};
