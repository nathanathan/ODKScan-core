#ifndef ALIGNER_H
#define ALIGNER_H

#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <exception>

class AlignmentException: public std::exception {
  virtual const char* what() const throw()
  {
	return "Alignment exception";
  }
};

class Aligner
{
	cv::Mat currentImg;
	std::vector<cv::KeyPoint> currentImgKeypoints;
	cv::Mat currentImgDescriptors;
	cv::Point3d trueEfficiencyScale;
	
	cv::Ptr<cv::FeatureDetector> detector;
	cv::Ptr<cv::DescriptorExtractor> descriptorExtractor;
	cv::Ptr<cv::DescriptorMatcher> descriptorMatcher;
	
	std::vector<std::string> templatePaths;
	std::vector< std::vector<cv::KeyPoint> > templKeypointsVec;
	std::vector<cv::Mat> templDescriptorsVec;
	std::vector<cv::Size> templImageSizeVec;
	
	AlignmentException myAlignmentException;
	
	public:
		Aligner();
		/*
		//Loads as well
		bool generateFeatureData(const string& templateDirecotry);
		
		//Returns the index of the feature data to use
		int DetectForm(const cv::Mat& formImg) const; //maybe private
		//If I call this from javaland then I will need to get back the template name to use which might be unplesant...
		//But if I call it from Processor I will need to keep that processor instance around so that it preserves the template name,
		//which might have the advantage of speeding things up a little by avoiding saving/loading the images...
		
		//gets the template name from the index returned by detect from
		std::string getTemplateName(int idx);
		*/
		
		//Potential efficiency gains by loading descriptors and not keypoints.
		void loadFeatureData(const std::string& templPath) throw(AlignmentException);
		
		//This will be a bit slow because it resizes the image and
		//computes its features and descriptors.
		//Calling this is a prerequisite for DetectForm and alignFormImage
		void setImage( const cv::Mat& img );
		
		int DetectForm() { return 0; }
		
		void alignFormImage( cv::Mat& aligned_image,
							 const cv::Size& aligned_image_sz, int fdIdx ) throw(AlignmentException);
		
	private:
		
};

#endif
