#include "configuration.h"
#include "Aligner.h"
#include "AlignmentUtils.h"
#include "Addons.h"
#include "FileUtils.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <math.h>

#include <iostream>
#include <fstream>

//vaying the reproject threshold can make a big difference in performance.
#define FH_REPROJECT_THRESH 4.0 //4 might be the best

//#define ALWAYS_COMPUTE_TEMPLATE_FEATURES
//#define SHOW_MATCHES_WINDOW
#define MASK_CENTER_AMOUNT .41

#ifdef OUTPUT_DEBUG_IMAGES
	#include "NameGenerator.h"
	NameGenerator dbgNamer("debug_form_images/", true);
#endif

using namespace std;
using namespace cv;

#ifdef SHOW_MATCHES_WINDOW
	Mat featureSource;
	vector<Mat> templateImages;
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
void loadFeatures( const string& featuresFile, Size& templImageSize,
					vector<KeyPoint>& templKeypoints, Mat& templDescriptors) throw(cv::Exception) {
							
	if( !fileExists(featuresFile) ) CV_Error(CV_StsError, "Specified feature file does not exist.");
	
	FileStorage fs(featuresFile, FileStorage::READ);
	
	if( !fs["templwidth"].empty() && !fs["templheight"].empty() &&
		!fs["templKeypoints"].empty() && !fs["templDescriptors"].empty()){

		fs["templwidth"] >> templImageSize.width;
		fs["templheight"] >> templImageSize.height; 

		read(fs["templKeypoints"], templKeypoints);
		fs["templDescriptors"] >> templDescriptors;
	}
	else{
		CV_Error(CV_StsError, "Required field is missing.");
	}
	
	if(templKeypoints.empty() || templDescriptors.empty()) CV_Error(CV_StsError, "Keypoints or descriptors not loaded.");
}
void saveFeatures(  const string& featuresFile, const Size& templImageSize,
					const vector<KeyPoint>& templKeypoints, const Mat& templDescriptors) throw(cv::Exception) {
					
	// write feature data to a file.
	FileStorage fs(featuresFile, FileStorage::WRITE);

	fs << "templwidth" << templImageSize.width;
	fs << "templheight" << templImageSize.height;

	write(fs, "templKeypoints", templKeypoints);
	fs << "templDescriptors" << templDescriptors;
}

Aligner::Aligner(){

	//Sorry the constructor is such a mess, just ignore everything commented out.

	//detector = Ptr<FeatureDetector>(new SurfFeatureDetector(800, 4, 1));
	//detector = Ptr<FeatureDetector>(new GoodFeaturesToTrackDetector( 800, .2, 10));
	//MSER is pretty fast. Grid seems to help limit number but messes up more.
	//descriptorExtractor = Ptr<DescriptorExtractor>(new SurfDescriptorExtractor( 4, 1, true ));
	//descriptorExtractor = Ptr<DescriptorExtractor>(new SurfDescriptorExtractor( 4, 3, true ));
	//detector = FeatureDetector::create( "SURF" ); 
	//detector = FeatureDetector::create( "GridSURF" );
	//#define MATCHER_TYPE "BruteForce"
	
	#define PARAM_SET 1
	#if PARAM_SET == 0
		detector = Ptr<FeatureDetector>(new GridAdaptedFeatureDetector(
											new SurfFeatureDetector( 250., 1, 3),
											3500, 7, 7));
	
		//descriptorExtractor = DescriptorExtractor::create( "SURF" );
		descriptorExtractor = Ptr<DescriptorExtractor>(new SurfDescriptorExtractor( 1, 3 ));
		#define MATCHER_TYPE "FlannBased"
	#elif PARAM_SET == 1
		//Optimal hessian level seems to vary by phone
		detector = Ptr<FeatureDetector>(new GridAdaptedFeatureDetector(
											new SurfFeatureDetector( 395., 1, 3),
											3500, 7, 7));
	
		//descriptorExtractor = DescriptorExtractor::create( "SURF" );
		descriptorExtractor = Ptr<DescriptorExtractor>(new SurfDescriptorExtractor( 1, 3 ));
		#define MATCHER_TYPE "FlannBased"
	#elif PARAM_SET == 2
		//Reduced number of features:
		detector = Ptr<FeatureDetector>(new GridAdaptedFeatureDetector(
											new SurfFeatureDetector( 395., 1, 3),
											500, 5, 5));
	
		//descriptorExtractor = DescriptorExtractor::create( "SURF" );
		descriptorExtractor = Ptr<DescriptorExtractor>(new SurfDescriptorExtractor( 1, 3 ));
		#define MATCHER_TYPE "BruteForce"
	#endif

}
void Aligner::setImage( const cv::Mat& img ){

	#if 0
	//TODO: Figure out a way to set the camera params here.
	Mat dcm = getDefaultNewCameraMatrix((Mat_<float>(3,3) << 612.02,0,319.5,0,612.02,239.5,0,0,1), img.size(), true);
	//cout << dcm << endl;
	undistort(img, currentImg, dcm, (Mat_<double>(5,1) << .0660769, -.32678, -.0011122, -.002264932, 1.5752));
	#endif
	currentImg = img;
	
	Mat currentImgResized;
	//1259712 is the area of a 5 megapixel image scaled by .5.
	//The goal is to keep resizing consistent.
	resize(currentImg, currentImgResized, sqrt(1259712.f / currentImg.size().area()) * currentImg.size(), 0, 0, INTER_AREA);
	
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
void Aligner::loadFeatureData(const string& templPath) throw(cv::Exception) {

	vector<KeyPoint> templKeypoints;
	Mat templDescriptors;
	Size templImageSize;
	string featuresFile = templPath + ".yml";

	try{
		#ifdef ALWAYS_COMPUTE_TEMPLATE_FEATURES
			CV_Error(CV_StsError, "Always compute template features is on.");
		#endif
		loadFeatures( featuresFile, templImageSize, templKeypoints, templDescriptors );
	}
	catch( cv::Exception& e ) {
	
		cout << e.what() << endl;
		cout << "Creating new feature data:" << endl;
		
		if(detector.empty() || descriptorExtractor.empty()) CV_Error(CV_StsError, "Cound not create detector/extractor.");
		
		Mat templImage, temp;
		templImage = imread( templPath + ".jpg", 0 );
		resize(templImage, temp, templImage.size(), 0, 0, INTER_AREA);
		
		/*
		templImage = temp;
		adaptiveThreshold(temp, templImage, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 9, 15);
		equalizeHist(temp, templImage);
		*/
		
		temp.release();
		templImageSize = templImage.size();

		#ifdef DEBUG_ALIGN_IMAGE
		cout << "Extracting keypoints from template image..." << endl;
		#endif
		
		Mat mask = makeFieldMask(templPath + ".json");
		resize(mask, temp, templImage.size(), 0, 0, INTER_AREA);
		erode(temp, mask, Mat(), Point(-1,-1), 1);
		
		/*
		#ifdef MASK_CENTER_AMOUNT
			Rect roi = resizeRect(Rect(Point(0,0), templImage.size()), MASK_CENTER_AMOUNT);
			rectangle(mask, roi.tl(), roi.br(), Scalar::all(0), -1);
		#endif
		*/
		
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
		cout << featuresFile << endl;

		saveFeatures(featuresFile, templImageSize, templKeypoints, templDescriptors);
	}
	#ifdef SHOW_MATCHES_WINDOW
		{
		Mat templImage, temp;
		templImage = imread( templPath + ".jpg", 0 );
		resize(templImage, temp, templImage.size(), 0, 0, INTER_AREA);
		templateImages.push_back( temp );
		}
	#endif
	templKeypointsVec.push_back(templKeypoints);
	templDescriptorsVec.push_back(templDescriptors);
	templImageSizeVec.push_back(templImageSize);
}
size_t Aligner::detectForm() const{
	//TODO: Make this code unterrible
	Ptr<DescriptorMatcher> descriptorMatcher = DescriptorMatcher::create( MATCHER_TYPE );
	size_t formIdx = 0;
	size_t previousBest = 0;
	
	for(size_t i = 0; i < templDescriptorsVec.size(); i++){
		vector<DMatch> filteredMatches;
		size_t inliers = 0;
		crossCheckMatching( descriptorMatcher, currentImgDescriptors, templDescriptorsVec[i], filteredMatches);
		
		vector<int> queryIdxs( filteredMatches.size() ), trainIdxs( filteredMatches.size() );
		for( size_t j = 0; j < filteredMatches.size(); j++ )
		{
			queryIdxs[j] = filteredMatches[j].queryIdx;
			trainIdxs[j] = filteredMatches[j].trainIdx;
		}

		vector<Point2f> points1; KeyPoint::convert(currentImgKeypoints, points1, queryIdxs);
		vector<Point2f> points2; KeyPoint::convert(templKeypointsVec[i], points2, trainIdxs);
		
		Mat H = findHomography( Mat(points1), Mat(points2), CV_RANSAC, FH_REPROJECT_THRESH );
		Mat points1t; perspectiveTransform(Mat(points1), points1t, H);
		
		for( size_t i1 = 0; i1 < points1.size(); i1++ )
		{
		    if( norm(points2[i1] - points1t.at<Point2f>((int)i1,0)) < FH_REPROJECT_THRESH ) // inlier
		       inliers++;
		}
		
		if(inliers > previousBest){
			previousBest = inliers;
			formIdx = i;
		}
	}
	return formIdx;
/*
	Ptr<BOWImgDescriptorExtractor> bowExtractor =
			new BOWImgDescriptorExtractor( descriptorExtractor, DescriptorMatcher::create( MATCHER_TYPE ));
	
	Mat vocabulary = trainVocabulary( "BOWfile", vocData, vocabTrainParams,
                                      detector, descriptorExtractor );
                                      
	CvSVM svm;
	trainSVMClassifier( svm, svmTrainParamsExt, objClasses[classIdx], vocData,
                            bowExtractor, featureDetector, resPath );
                                      
	bowExtractor->setVocabulary( vocabulary );
	
	Mat bowDescriptor;
	bowExtractor->compute( currentImg, currentImgKeypoints, bowDescriptor );
*/
}
void Aligner::alignFormImage(Mat& aligned_img, const Size& aligned_img_sz, size_t featureDataIdx ) throw(cv::Exception) {
	
	vector<KeyPoint> templKeypoints = templKeypointsVec[featureDataIdx];
	Mat templDescriptors = templDescriptorsVec[featureDataIdx];
	Size templImageSize = templImageSizeVec[featureDataIdx];

	#ifdef DEBUG_ALIGN_IMAGE
		cout << "Matching descriptors..." << endl;
	#endif
	Ptr<DescriptorMatcher> descriptorMatcher = DescriptorMatcher::create( MATCHER_TYPE );
	
	if( descriptorMatcher.empty()) CV_Error(CV_StsError, "Can not create descriptor matcher of given type");
	
	vector<DMatch> filteredMatches;
	crossCheckMatching( descriptorMatcher, currentImgDescriptors, templDescriptors, filteredMatches);
	//descriptorMatcher->match( currentImgDescriptors, templDescriptors, filteredMatches );

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
	
	if( points1.size() < 4 || points2.size() < 4) CV_Error(CV_StsError, "Not enough matches.");
	
	Mat H = findHomography( Mat(points1), Mat(points2), CV_RANSAC, FH_REPROJECT_THRESH );//CV_LMEDS
	Mat Hscaled = Mat::diag(Mat(sc)) * H * Mat::diag(Mat(trueEfficiencyScale));

	aligned_img = Mat(0,0, CV_8U);
	warpPerspective( currentImg, aligned_img, Hscaled, aligned_img_sz);
	
	vector<Point> quad = transformationToQuad(Hscaled, aligned_img_sz);

	#ifdef SHOW_MATCHES_WINDOW
		//This code creates a window to show matches:
		vector<char> matchesMask( filteredMatches.size(), 0 );
		Mat points1t; perspectiveTransform(Mat(points1), points1t, H);
		for( size_t i1 = 0; i1 < points1.size(); i1++ )
		{
		    if( norm(points2[i1] - points1t.at<Point2f>((int)i1,0)) < FH_REPROJECT_THRESH ) // inlier
		        matchesMask[i1] = 1;
		}
	
		Mat drawImg;

		drawMatches( featureSource, currentImgKeypoints, templateImages[featureDataIdx], templKeypoints, filteredMatches, drawImg,
					CV_RGB(0, 255, 0), CV_RGB(255, 0, 255), matchesMask, DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
	
		namedWindow("inliers", CV_WINDOW_NORMAL);
		imshow( "inliers", drawImg );
		
        for( size_t i1 = 0; i1 < matchesMask.size(); i1++ )
            matchesMask[i1] = !matchesMask[i1];
            
        drawMatches( featureSource, currentImgKeypoints, templateImages[featureDataIdx], templKeypoints, filteredMatches, drawImg,
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

	#ifdef OUTPUT_DEBUG_IMAGES
		Mat dbg;
		currentImg.copyTo(dbg);
		string qiname = dbgNamer.get_unique_name("alignment_debug_") + ".jpg";
		const Point* p = &quad[0];
		int n = (int) quad.size();
		polylines(dbg, &p, &n, 1, true, 250, 3, CV_AA);

		imwrite(qiname, dbg);
	#endif

	if( ! testQuad(quad, .85 * currentImg.size()) ) CV_Error(CV_StsError, "Invalid quad found.");
}
