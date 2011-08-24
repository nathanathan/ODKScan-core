#include "configuration.h"
#include "Aligner.h"
#include "AlignmentUtils.h"
#include "Addons.h"
#include "FileUtils.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <iostream>
#include <fstream>

//vaying the reproject threshold can make a big difference in performance.
#define FH_REPROJECT_THRESH 4.0

#define ALWAYS_COMPUTE_TEMPLATE_FEATURES
//#define SHOW_MATCHES_WINDOW
#define MASK_CENTER_AMOUNT .4

#ifdef OUTPUT_DEBUG_IMAGES
#include "NameGenerator.h"
NameGenerator dbgNamer("debug_form_images/", true);
#endif

using namespace std;
using namespace cv;

#ifdef SHOW_MATCHES_WINDOW
	#define ALWAYS_COMPUTE_TEMPLATE_FEATURES
	Mat featureSource, featureDest;
#endif

//This is from the OpenCV descriptor matcher example.
void crossCheckMatching( Ptr<DescriptorMatcher>& descriptorMatcher,
                         const Mat& descriptors1, const Mat& descriptors2,
                         vector<DMatch>& filteredMatches12, int knn=1 )
{
    filteredMatches12.clear();
    vector<vector<DMatch> > matches12, matches21;
    descriptorMatcher->knnMatch( descriptors1, descriptors2, matches12, knn );
    descriptorMatcher->knnMatch( descriptors2, descriptors1, matches21, knn );
    for( size_t m = 0; m < matches12.size(); m++ )
    {
        bool findCrossCheck = false;
        for( size_t fk = 0; fk < matches12[m].size(); fk++ )
        {
            DMatch forward = matches12[m][fk];

            for( size_t bk = 0; bk < matches21[forward.trainIdx].size(); bk++ )
            {
                DMatch backward = matches21[forward.trainIdx][bk];
                if( backward.trainIdx == forward.queryIdx )
                {
                    filteredMatches12.push_back(forward);
                    findCrossCheck = true;
                    break;
                }
            }
            if( findCrossCheck ) break;
        }
    }
}
//I think the proper way to do this would be to make a FormAlignment class
//then pass in the mask from Processor.cpp
//Maybe not because I'll want to do this for all the templates... maybe I could crawl the files from Processor though?
Mat makeFieldMask(const string& jsonTemplate){
	
	cout << jsonTemplate;
	//Should there be a file that has all the JSON functions?
	Json::Value myRoot;
	parseJsonFromFile(jsonTemplate, myRoot);
	
	Mat mask(myRoot.get("height", 0).asInt(), myRoot.get("width", 0).asInt(),
			CV_8UC1, Scalar::all(255));
	const Json::Value fields = myRoot["fields"];
	for ( size_t i = 0; i < fields.size(); i++ ) {
		const Json::Value field = fields[i];
		const Json::Value segments = field["segments"];
		for ( size_t j = 0; j < segments.size(); j++ ) {
			const Json::Value segmentTemplate = segments[j];

			Rect segRect( Point( segmentTemplate.get("x", INT_MIN ).asInt(),
								   segmentTemplate.get("y", INT_MIN).asInt()),
							Size(segmentTemplate.get("width", INT_MIN).asInt(),
							segmentTemplate.get("height", INT_MIN).asInt()));
			rectangle(mask, segRect.tl(), segRect.br(), Scalar::all(0), -1);				
		}
	}
	return mask;
}

bool checkForSavedFeatures( const string& featuresFile, Size& templImageSize,
							vector<KeyPoint>& templKeypoints, Mat& templDescriptors) {
	if( fileExists(featuresFile) ) {
		try {
			FileStorage fs(featuresFile, FileStorage::READ);
			if( !fs["templwidth"].empty() && !fs["templheight"].empty() &&
				!fs["templKeypoints"].empty() && !fs["templDescriptors"].empty()){

				fs["templwidth"] >> templImageSize.width;
				fs["templheight"] >> templImageSize.height; 

				read(fs["templKeypoints"], templKeypoints);
				fs["templDescriptors"] >> templDescriptors;
			}
			else{
				return false;
			}
		}
		catch( cv::Exception& e ) {
			const char* err_msg = e.what();
			cout << err_msg << endl;
			return false;
		}
	}
	return !templKeypoints.empty() && !templDescriptors.empty();
}
bool saveFeatures(  const string& featuresFile, const Size& templImageSize,
					const vector<KeyPoint>& templKeypoints, const Mat& templDescriptors) {
	
	if( !fileExists(featuresFile) ) return false;

	try {
		// write feature data to a file.
		FileStorage fs(featuresFile, FileStorage::WRITE);
	
		fs << "templwidth" << templImageSize.width;
		fs << "templheight" << templImageSize.height;
	
		write(fs, "templKeypoints", templKeypoints);
		fs << "templDescriptors" << templDescriptors;
	}
	catch( cv::Exception& e ) {
		const char* err_msg = e.what();
		cout << err_msg << endl;
		return false;
	}
	return true;
}

Aligner::Aligner(){
	//detector = Ptr<FeatureDetector>(new SurfFeatureDetector(800, 4, 1));
	//detector = Ptr<FeatureDetector>(new GoodFeaturesToTrackDetector( 800, .2, 10));
	//MSER is pretty fast. Grid seems to help limit number but messes up more.
	//descriptorExtractor = Ptr<DescriptorExtractor>(new SurfDescriptorExtractor( 4, 1, true ));
	//descriptorExtractor = Ptr<DescriptorExtractor>(new SurfDescriptorExtractor( 4, 3, true ));
	//detector = FeatureDetector::create( "SURF" ); 
	//detector = FeatureDetector::create( "GridSURF" );
	
	detector = Ptr<FeatureDetector>(new GridAdaptedFeatureDetector(
										new SurfFeatureDetector( 300., 1, 3), //Adding octaves while shrinking the image might speed things up.
										500, 4, 4));//4,4 grid size seems to be bizzarly more effective than other sizes.
	//Increasing the octave layers might help in some cases
	
	descriptorExtractor = DescriptorExtractor::create( "SURF" );
	
	#define MATCHER_TYPE "BruteForce"
	//#define MATCHER_TYPE "FlannBased"
	descriptorMatcher = DescriptorMatcher::create( MATCHER_TYPE );
}
void Aligner::setImage( const cv::Mat& img ){
	currentImg = img;
	
	// Be careful when resizing, aliasing can completely break this function.
	//If something goes wrong revert to .5 image size
	Mat currentImgResized;
	resize(currentImg, currentImgResized, .5 * currentImg.size(), 0, 0, INTER_AREA);
	trueEfficiencyScale = Point3d(  double(currentImgResized.cols) / img.cols,
									double(currentImgResized.rows) / img.rows,
									1.0);
	
	#if 0
		Mat temp;
		adaptiveThreshold(currentImgResized, temp, 50, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 9, 15);
		//equalizeHist(currentImgResized, temp);
		currentImgResized = currentImgResized + temp;
		currentImgResized.convertTo(currentImgResized, 0);
	#endif
	
	#ifdef SHOW_MATCHES_WINDOW
		featureSource = currentImgResized;
	#endif
	
	#if 0
		namedWindow("outliers", CV_WINDOW_NORMAL);
		imshow( "outliers", currentImgResized );
	
		for(;;)
		{
			char c = (char)waitKey(0);
			if( c == '\x1b' ) // esc
			{
				cvDestroyWindow("outliers");
			    break;
			}
		}
	#endif
	Mat mask(currentImgResized.rows, currentImgResized.cols, CV_8UC1, Scalar::all(255));
	#ifdef MASK_CENTER_AMOUNT
		Rect roi = resizeRect(Rect(Point(0,0), currentImgResized.size()), MASK_CENTER_AMOUNT);
		rectangle(mask, roi.tl(), roi.br(), Scalar::all(0), -1);
	#endif
	
	#ifdef DEBUG_ALIGN_IMAGE
		cout << "Extracting keypoints from current image..." << endl;
	#endif
	
	detector->detect( currentImgResized, currentImgKeypoints, mask);
	
	#ifdef DEBUG_ALIGN_IMAGE
		cout << "\t" << currentImgKeypoints.size() << " points" << endl;
		cout << "Computing descriptors for keypoints from current image..." << endl;
	#endif
	
	descriptorExtractor->compute( currentImgResized, currentImgKeypoints, currentImgDescriptors );
}
//Tries to read feature data (presumably for the template) from templPath + ".yml"
//If none is found it is generated for the image templPath + ".jpg"
void Aligner::loadFeatureData(const string& templPath) throw(AlignmentException) {

	vector<KeyPoint> templKeypoints;
	Mat templDescriptors;
	Size templImageSize;
	string featuresFile = templPath + ".yml";

	bool featuresFound = false;
	#ifndef ALWAYS_COMPUTE_TEMPLATE_FEATURES
		featuresFound = checkForSavedFeatures(  featuresFile, templImageSize, templKeypoints, templDescriptors);
	#endif
	
	if(detector.empty() || descriptorExtractor.empty()) throw myAlignmentException;
	
	if( !featuresFound  ){
		//if there is no file to read descriptors and keypoints from make one.
		Mat templImage, temp;
		templImage = imread( templPath + ".jpg", 0 );
		resize(templImage, temp, templImage.size(), 0, 0, INTER_AREA);
		
		//templImage = temp;
		//adaptiveThreshold(temp, templImage, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 9, 15);
		//equalizeHist(temp, templImage);
		
		temp.release();
		templImageSize = templImage.size();

		#ifdef DEBUG_ALIGN_IMAGE
		cout << "Extracting keypoints from template image..." << endl;
		#endif
		
		Mat mask = makeFieldMask(templPath + ".json");
		resize(mask, temp, templImage.size(), 0, 0, INTER_AREA);
		erode(temp, mask, Mat(), Point(-1,-1), 6);
		#ifdef MASK_CENTER_AMOUNT
			Rect roi = resizeRect(Rect(Point(0,0), templImage.size()), MASK_CENTER_AMOUNT);
			rectangle(mask, roi.tl(), roi.br(), Scalar::all(0), -1);
		#endif
		
		#ifdef SHOW_MATCHES_WINDOW
			featureDest = templImage;
		#endif
		
		#if 0
		imshow( "outliers", templImage & mask);
		for(;;)
		{
		    char c = (char)waitKey(0);
		    if( c == '\x1b' ) // esc
		    {
		    	cvDestroyWindow("inliers");
		    	cvDestroyWindow("outliers");
		        break;
		    }
		}
		#endif
		
		temp.release();
		detector->detect( templImage, templKeypoints, mask );

		#ifdef DEBUG_ALIGN_IMAGE
		cout << "\t" << templKeypoints.size() << " points" << endl;
		cout << "Computing descriptors for keypoints from template image..." << endl;
		#endif
		descriptorExtractor->compute( templImage, templKeypoints, templDescriptors );
		
		if( !saveFeatures(featuresFile, templImageSize, templKeypoints, templDescriptors) ) throw myAlignmentException;
	}
	templKeypointsVec.push_back(templKeypoints);
	templDescriptorsVec.push_back(templDescriptors);
	templImageSizeVec.push_back(templImageSize);
}
void Aligner::alignFormImage(Mat& aligned_img, const Size& aligned_img_sz, int featureDataIdx ) throw(AlignmentException) {
	
	vector<KeyPoint> templKeypoints = templKeypointsVec[featureDataIdx];
	Mat templDescriptors = templDescriptorsVec[featureDataIdx];
	Size templImageSize = templImageSizeVec[featureDataIdx];

	#ifdef DEBUG_ALIGN_IMAGE
		cout << "Matching descriptors..." << endl;
	#endif
	
	if( descriptorMatcher.empty()  ) {
		cout << "Can not create descriptor matcher of given type" << endl;
		throw myAlignmentException;
	}
	
	vector<DMatch> filteredMatches;
	crossCheckMatching( descriptorMatcher, currentImgDescriptors, templDescriptors, filteredMatches, 1 );

	vector<int> queryIdxs( filteredMatches.size() ), trainIdxs( filteredMatches.size() );
	for( size_t i = 0; i < filteredMatches.size(); i++ )
	{
		queryIdxs[i] = filteredMatches[i].queryIdx;
		trainIdxs[i] = filteredMatches[i].trainIdx;
	}

	vector<Point2f> points1; KeyPoint::convert(currentImgKeypoints, points1, queryIdxs);
	vector<Point2f> points2; KeyPoint::convert(templKeypoints, points2, trainIdxs);

	Point3d sc = Point3d( ((double)aligned_img_sz.width) / templImageSize.width,
						  ((double)aligned_img_sz.height) / templImageSize.height,
						  1.0);
	
	if( points1.size() < 4 || points2.size() < 4) throw myAlignmentException;
	
	Mat H = findHomography( Mat(points1), Mat(points2), CV_RANSAC, FH_REPROJECT_THRESH );//CV_LMEDS
	Mat Hscaled = Mat::diag(Mat(sc)) * H * Mat::diag(Mat(trueEfficiencyScale));

	aligned_img = Mat(0,0, CV_8U);
	warpPerspective( currentImg, aligned_img, Hscaled, aligned_img_sz);
	
	vector<Point> quad = transformationToQuad(Hscaled, aligned_img_sz);
	
	if( ! testQuad(quad, .8 * currentImg.size()) ) throw myAlignmentException;

	#ifdef SHOW_MATCHES_WINDOW
		//This code creates a window to show matches:
		vector<char> matchesMask( filteredMatches.size(), 0 );
		Mat points1t; perspectiveTransform(Mat(points1), points1t, H);
		for( size_t i1 = 0; i1 < points1.size(); i1++ )
		{
		    if( norm(points2[i1] - points1t.at<Point2f>((int)i1,0)) < 4 ) // inlier
		        matchesMask[i1] = 1;
		}
	
		Mat drawImg;

		drawMatches( featureSource, currentImgKeypoints, featureDest, templKeypoints, filteredMatches, drawImg,
					CV_RGB(0, 255, 0), CV_RGB(255, 0, 255), matchesMask, DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
	
		namedWindow("inliers", CV_WINDOW_NORMAL);
		imshow( "inliers", drawImg );
		
        for( size_t i1 = 0; i1 < matchesMask.size(); i1++ )
            matchesMask[i1] = !matchesMask[i1];
            
        drawMatches( featureSource, currentImgKeypoints, featureDest, templKeypoints, filteredMatches, drawImg,
        			CV_RGB(0, 0, 255), CV_RGB(255, 0, 0), matchesMask,
        			DrawMatchesFlags::DRAW_OVER_OUTIMG | DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
		
		namedWindow("outliers", CV_WINDOW_NORMAL);
		imshow( "outliers", drawImg );
		
		for(;;)
		{
		    char c = (char)waitKey(0);
		    if( c == '\x1b' ) // esc
		    {
		    	cvDestroyWindow("inliers");
		    	cvDestroyWindow("outliers");
		        break;
		    }
		}
	#endif
	/*
	#ifdef OUTPUT_DEBUG_IMAGES
		Mat dbg;
		img.copyTo(dbg);
		string qiname = dbgNamer.get_unique_name("alignment_debug_") + ".jpg";
		const Point* p = &quad[0];
		int n = (int) quad.size();
		if( alignSuccess ){
			polylines(dbg, &p, &n, 1, true, 250, 3, CV_AA);
		}
		else{
			polylines(dbg, &p, &n, 1, true, 0, 5, CV_AA);
			cout << "Form alignment failed" << endl;
		}
		imwrite(qiname, dbg);
	#endif
	*/
}
