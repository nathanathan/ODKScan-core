#include "configuration.h"
#include "FormAlignment.h"
#include "Addons.h"
#include "FileUtils.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <iostream>
#include <fstream>

#ifdef OUTPUT_DEBUG_IMAGES
#include "NameGenerator.h"
NameGenerator alignmentNamer("debug_segment_images/");
NameGenerator dbgNamer("debug_form_images/", true);
#endif

//image_align constants
#define THRESH_OFFSET_LB -.3
#define THRESH_DECR_SIZE .05

#define EXPANSION_PERCENTAGE .04

#define USE_FEATURE_BASED_FORM_ALIGNMENT

//#define ALWAYS_COMPUTE_TEMPLATE_FEATURES
#define FH_REPROJECT_THRESH 5.0

using namespace std;
using namespace cv;

//Order a 4 point vector clockwise with the 0 index at the most top-left corner
vector<Point> orderCorners(const vector<Point>& corners){

	vector<Point> orderedCorners;

	Moments m = moments(Mat(corners));

	Mat center = (Mat_<double>(1,3) << m.m10/m.m00, m.m01/m.m00, 0 );
	Mat p0 = (Mat_<double>(1,3) << corners[0].x, corners[0].y, 0 );
	Mat p1 = (Mat_<double>(1,3) << corners[1].x, corners[1].y, 0 );

	if((center - p0).cross(p1 - p0).at<double>(0,2) < 0){ //Check this math just in case
		orderedCorners = vector<Point>(corners.begin(), corners.end());
	}
	else{
		orderedCorners = vector<Point>(corners.rbegin(), corners.rend());
	}

	int shift = 0;
	double tlMax = 0;
	Mat B = (Mat_<double>(1,2) << -1, -1);
	for(size_t i = 0; i < orderedCorners.size(); i++ ){
		Mat A = (Mat_<double>(1,2) << orderedCorners[i].x - m.m10/m.m00, orderedCorners[i].y - m.m01/m.m00);
		double tlProj = A.dot(B);
		if(tlProj > tlMax){
			shift = i;
			tlMax = tlProj;
		}
	}

	vector<Point> temp = vector<Point>(orderedCorners.begin(), orderedCorners.end());
	for(size_t i = 0; i < orderedCorners.size(); i++ ){
		orderedCorners[i] = temp[(i + shift) % orderedCorners.size()];
	}
	return orderedCorners;
}
//Creates a new vector with all the points expanded about the average of the first vector.
vector<Point> expandCorners(const vector<Point>& corners, double expansionPercent){
	Point center(0,0);
	for(size_t i = 0; i < corners.size(); i++){
		center += corners[i];
	}
	
	center *= 1.f / corners.size();
	vector<Point> out(corners.begin(), corners.end());
	
	for(size_t i = 0; i < out.size(); i++){
		out[i] += expansionPercent * (corners[i] - center);
	}
	return out;
}

// Try to distil the maximum quad (4 point contour) from a convex contour of many points.
// if none is found maxQuad will not be altered.
// TODO: Find out what happens when you try to simplify a contour that already has just 4 points.
float maxQuadSimplify(vector <Point>& contour, vector<Point>& maxQuad, float current_approx_p){
	
	float area = 0;
	
	float arc_len = arcLength(Mat(contour), true);
	while (current_approx_p < 1) {
		vector <Point> approx;
		approxPolyDP(Mat(contour), approx, arc_len * current_approx_p, true);
		if (approx.size() == 4){
			maxQuad = approx;
			area = fabs(contourArea(Mat(approx)));
			break;
		}
		current_approx_p += .01;
	}
	return area;
}
//Find the largest quad contour in img
//Warning: destroys img
vector<Point> findMaxQuad(Mat& img, float approx_p_seed = 0){
	vector<Point> maxRect;
	vector < vector<Point> > contours;
	// Find all external contours of the image
	findContours(img, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	float maxContourArea = 0;
	// Iterate through all detected contours
	for (size_t i = 0; i < contours.size(); ++i) {
		vector<Point> contour, quad;
		convexHull(Mat(contours[i]), contour);
		float area = maxQuadSimplify(contour, quad, approx_p_seed);

		if (area > maxContourArea) {
			maxRect = quad;
			maxContourArea = area;
		}
	}
	return maxRect;
}
int lineSum(const Mat& img, int start, int end){
	int sum = 0;
	double slope = (double)(end - start)/(img.cols - 1);
	for(int i = 0; i<img.cols; i++) {
		sum += (int) (img.at<uchar>(start + slope*i, i) >= 1);
	}
	return sum;
}
void findLines(Mat& img) {
	Mat grad_img, out;
	int start, end;
	
	int range = 12;

	int minLs = INT_MAX;
	for(int i = 0; i<img.rows/2; i++) {
		for(int j = MAX(i-range, 0); j< MIN(i+range, img.rows/2); j++) {
			int ls = lineSum(img, i, j);
			if( ls < minLs ){
				start = i;
				end = j;
				minLs = ls;
			}
		}
	}
	line( img, Point(0, start), Point(img.cols-1, end), Scalar(0), 2);
}
//TODO: The blurSize param is sort of a hacky solution to the problem of contours being too large
//		in certain cases. It fixes the problem on some form images because they can be blurred a lot
//		and produce good results for contour finding. This is not the case with segments.
//		I think a better solution might be to alter maxQuad somehow... I haven't figured out how though.
vector<Point> findQuad(const Mat& img, int blurSize, float buffer = 0.0){
	Mat imgThresh, temp_img, temp_img2;
	
	//Shrink the image down for efficiency
	//and so we don't have to worry about filters behaving differently on large images
	int multiple = img.cols / 256;
	if(multiple > 1){
		resize(img, temp_img, (1.f / multiple) * img.size(), INTER_AREA);
	}
	else{
		multiple = 1;
		img.copyTo(temp_img);
	}
	//resize(img, temp_img, 2*Size(img.cols, img.rows), INTER_AREA);
	
	float actual_width_multiple = float(img.rows) / temp_img.rows;
	float actual_height_multiple = float(img.cols) / temp_img.cols;
	
	temp_img.copyTo(temp_img2);
	//blur(temp_img, temp_img2, Size(3, 3));
	blur(temp_img2, temp_img, 2*Size(2*blurSize+1, 2*blurSize+1));
	//erode(temp_img2, temp_img, (Mat_<uchar>(3,3) << 1,0,1,0,1,0,1,0,1));
	//This threshold might be tweakable
	imgThresh = temp_img2 - temp_img > 0;
	

	/*
	//Subtracting the laplacian is a hacky but sometimes effective way to
	//improve segmentation in cases when separate segments are connected by a handful of pixels.
	Laplacian(temp_img2, temp_img, -1, 9);
	imgThresh = imgThresh - (temp_img > 40);
	*/
	
	//Blocking out a chunk in the middle of the segment can help in cases with densely filled bubbles.
	float choke = buffer/(1 + 2*buffer) + .15;
	Rect roi(choke * Point(temp_img.cols, temp_img.rows), (1.f - 2*choke) * temp_img.size());
	imgThresh(roi) = Scalar(255);
	
	//This will draw a few lines across the image to split up the contour
	{
		Mat temp;
	
		findLines(imgThresh);
		flip(imgThresh, temp, 0);
		findLines(temp);
		transpose(temp, imgThresh);
		findLines(imgThresh);
		flip(imgThresh, temp, 0);
		findLines(temp);
		flip(temp, imgThresh, -1);
		transpose(imgThresh, temp);
		imgThresh = temp;
	}
	
	#ifdef OUTPUT_DEBUG_IMAGES
	
	Mat dbg_out, dbg_out2;
	imgThresh.copyTo(dbg_out);
	//img.copyTo(dbg_out2);
	string segfilename = alignmentNamer.get_unique_name("alignment_debug_");
	segfilename.append(".jpg");
    vector < vector<Point> > contours;
    Mat temp;
	//findContours(dbg_out, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	//img.copyTo(dbg_out);
	//drawContours(dbg_out, contours, -1, 255);
	imwrite(segfilename, dbg_out);
	
	#endif
	
	vector<Point> quad = findMaxQuad(imgThresh, 0);
	
	imgThresh.release();
	
	//Resize the contours for the full size image:
	for(size_t i = 0; i<quad.size(); i++){
		quad[i] = Point(actual_width_multiple * quad[i].x, actual_height_multiple * quad[i].y);
	}
	
	return expandCorners(quad, EXPANSION_PERCENTAGE);
}
Mat quadToTransformation(const vector<Point>& foundCorners, const Size& out_image_sz){

	Point2f out_corners[4] = {Point2f(0, 0), Point2f(out_image_sz.width, 0),
							  Point2f(out_image_sz.width, out_image_sz.height), Point2f(0, out_image_sz.height)};

	vector<Point> orderedCorners = orderCorners(foundCorners);
	
	Point2f corners_a[4];
	
	for(size_t i = 0; i<orderedCorners.size(); i++){
		corners_a[i] = Point2f(orderedCorners[i].x, orderedCorners[i].y);
	}
	
	return getPerspectiveTransform(corners_a, out_corners);
}
//Takes a 3x3 transformation matrix H and a output image size and returns
//a quad representing the region in a image to be transfromed by H the transformed image will contain.
vector<Point> transformationToQuad(const Mat& H, const Size& out_image_sz){
										
	Mat img_rect = (Mat_<double>(3,4) << 0, out_image_sz.width, (double)out_image_sz.width,	 0,
										 0, 0,					(double)out_image_sz.height, out_image_sz.height,
										 1,	1,					1, 					 1);
	//cout << img_rect.at<double>(3, 0) <<", " << img_rect.at<double>(3, 1) << endl;
	//cout << img_rect << endl;

	Mat out_img_rect =  H.inv() * img_rect;

	//cout << img_rect << endl;
	
	vector<Point> quad;
	for(size_t i = 0; i < 4; i++){
		double sc = 1. / out_img_rect.at<double>(2, i);
		quad.push_back( sc * Point(out_img_rect.at<double>(0, i), out_img_rect.at<double>(1, i)) );
		//cout << out_img_rect.at<double>(0, i) <<", " << out_img_rect.at<double>(1, i) << endl;
		//cout << out_img_rect.at<double>(2, i) << endl;
	}
	return quad;
}
//Check if the contour has four points, does not self-intersect and is convex.
bool testQuad(const vector<Point>& quad){

	if(quad.size() != 4) return false;
	
	Mat quadMat;
	for(size_t i = 0; i < 4; i++){
		Mat Z = (Mat_<double>(1,3) << quad[i].x, quad[i].y, 0 );
		if(quadMat.empty()){
			quadMat = Z;
		}
		else{
			quadMat.push_back( Z );
		}
	}
	
	Mat A = quadMat.row(0)-quadMat.row(1);
	Mat B = quadMat.row(2)-quadMat.row(1);
	Mat C = quadMat.row(0)-quadMat.row(2);
	Mat D = quadMat.row(3)-quadMat.row(2);
	Mat E = quadMat.row(3)-quadMat.row(1);
	
	int sign = -1;
	if(A.cross(B).at<double>(0, 2) > 0){
		sign = 1;
	}
	
	return	sign*E.cross(B).at<double>(0, 2) > 0 &&
			sign*C.cross(D).at<double>(0, 2) > 0 &&
			sign*A.cross(E).at<double>(0, 2) > 0;
}
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
//Tries to read feature data (presumably for the template) from featureDataPath + ".yml" .
//If none is found it is generated for the image featureDataPath + ".jpg"
bool loadFeatureData(const string& featureDataPath, Ptr<FeatureDetector>& detector,
					Ptr<DescriptorExtractor>& descriptorExtractor, vector<KeyPoint>& templKeypoints,
					Mat& templDescriptors, Size& templImageSize) {
			
	//detector = Ptr<FeatureDetector>(new SurfFeatureDetector(800, 4, 1));
	//detector = Ptr<FeatureDetector>(new GoodFeaturesToTrackDetector( 800, .2, 10));
	//MSER is pretty fast. Grid seems to help limit number but messes up more.
	//descriptorExtractor = Ptr<DescriptorExtractor>(new SurfDescriptorExtractor( 4, 1, true ));
	//detector = FeatureDetector::create( "MSER" );
	//descriptorExtractor = Ptr<DescriptorExtractor>(new SurfDescriptorExtractor( 4, 3, true ));
	detector = FeatureDetector::create( "SURF" ); 
	descriptorExtractor = DescriptorExtractor::create( "SURF" );
	
	bool checkForSavedFeatures = true;
	#ifdef ALWAYS_COMPUTE_TEMPLATE_FEATURES
	checkForSavedFeatures = false;
	#endif
	
	if( checkForSavedFeatures && fileExists(featureDataPath + ".yml") ) {
		try
		{
			FileStorage fs(featureDataPath + ".yml", FileStorage::READ);
			/*if( !fs["fdtype"].empty () && !fs["detype"].empty() && !fs["detector"].empty() && 
				!fs["descriptorExtractor"].empty() && !fs["templKeypoints"].empty() && !fs["templDescriptors"].empty() ){
			*/
				detector->read(fs["detector"]);
				descriptorExtractor->read(fs["descriptorExtractor"]);
				
				fs["templwidth"] >> templImageSize.width;
				fs["templheight"] >> templImageSize.height; 

				read(fs["templKeypoints"], templKeypoints);
				fs["templDescriptors"] >> templDescriptors;
			//}
		}
		catch( cv::Exception& e )
		{
			const char* err_msg = e.what();
			cerr << err_msg << endl;
		}
	}
	if( detector.empty() || descriptorExtractor.empty() || templKeypoints.empty() || templDescriptors.empty()){
		//if there is no file to read descriptors and keypoints from make one.
		//cout << featureDataPath + ".yml " << fileexists(featureDataPath + ".yml") << endl;
		
		Mat templImage, temp;
		templImage = imread( featureDataPath + ".jpg", 0 );
		resize(templImage, temp, templImage.size(), 0, 0, INTER_AREA);
		templImage  = temp;
		templImageSize = templImage.size();

		#ifdef DEBUG_ALIGN_IMAGE
		cout << "Extracting keypoints from template image..." << endl;
		#endif
		//TODO: Make a mask that covers everything that someone might fill in using the template.
		detector->detect( templImage, templKeypoints); //makeFieldMask(featureDataPath + ".json") );
		#ifdef DEBUG_ALIGN_IMAGE
		cout << "\t" << templKeypoints.size() << " points" << endl;
		cout << "Computing descriptors for keypoints from template image..." << endl;
		#endif
		descriptorExtractor->compute( templImage, templKeypoints, templDescriptors );
		
		// write feature data to a file.
		FileStorage fs(featureDataPath + ".yml", FileStorage::WRITE);
		fs << "detector" << "{:"; detector->write(fs); fs << "}";
		fs << "descriptorExtractor" << "{:"; descriptorExtractor->write(fs); fs << "}";
		
		fs << "templwidth" << templImageSize.width;
		fs << "templheight" << templImageSize.height;
		
		write(fs, "templKeypoints", templKeypoints);
		fs << "templDescriptors" << templDescriptors;
		
		#ifdef DEBUG_ALIGN_IMAGE
		imwrite("lfdImage.jpg", templImage);
		#endif
	}
	if( detector.empty() || descriptorExtractor.empty() )
	{
		cerr << "Can not create/load detector or descriptor extractor" << endl;
		return false;
	}	
	
	return true;
}
bool alignFormImageByFeatures(const Mat& img, Mat& aligned_img, const string& featureDataPath, 
					const Size& aligned_img_sz, float efficiencyScale ) {

	Ptr<FeatureDetector> detector;
	Ptr<DescriptorExtractor> descriptorExtractor;
	
	vector<KeyPoint> templKeypoints;
	Mat templDescriptors;
	
	Size templImageSize;
	
	bool success = loadFeatureData(featureDataPath, detector, descriptorExtractor,
									templKeypoints, templDescriptors, templImageSize);
	if(!success) return false;
	
	// Be careful when resizing, aliasing can completely break this function.
	Mat img_resized;
	resize(img, img_resized, efficiencyScale*img.size(), 0, 0, INTER_AREA);
	Point3d trueEfficiencyScale(double(img_resized.cols) / img.cols, double(img_resized.rows) / img.rows, 1);
	
	#ifdef DEBUG_ALIGN_IMAGE
	cout << "Extracting keypoints from unaligned image..." << endl;
	#endif
	
	vector<KeyPoint> keypoints1;
	detector->detect( img_resized, keypoints1 );
	
	#ifdef DEBUG_ALIGN_IMAGE
	cout << "\t" << keypoints1.size() << " points" << endl;
	cout << "Computing descriptors for keypoints from unaligned image..." << endl;
	#endif
	
	Mat descriptors1;
	descriptorExtractor->compute( img_resized, keypoints1, descriptors1 );

	#ifdef DEBUG_ALIGN_IMAGE
	cout << "Matching descriptors..." << endl;
	#endif
	
	Ptr<DescriptorMatcher> descriptorMatcher = DescriptorMatcher::create( "BruteForce" );
	// "FlannBased" is another option but doesn't perform well out of the box
	
	if( descriptorMatcher.empty()  ) {
		cerr << "Can not create descriptor matcher of given type" << endl;
		return false;
	}
	vector<DMatch> filteredMatches;
	crossCheckMatching( descriptorMatcher, descriptors1, templDescriptors, filteredMatches, 1 );

	vector<int> queryIdxs( filteredMatches.size() ), trainIdxs( filteredMatches.size() );
	for( size_t i = 0; i < filteredMatches.size(); i++ )
	{
		queryIdxs[i] = filteredMatches[i].queryIdx;
		trainIdxs[i] = filteredMatches[i].trainIdx;
	}

	vector<Point2f> points1; KeyPoint::convert(keypoints1, points1, queryIdxs);
	vector<Point2f> points2; KeyPoint::convert(templKeypoints, points2, trainIdxs);
	
	Point3d sc = Point3d( double(aligned_img_sz.width) / templImageSize.width,
						  double(aligned_img_sz.height) / templImageSize.height,
						  1);
	Mat H = findHomography( Mat(points1), Mat(points2), CV_RANSAC, FH_REPROJECT_THRESH );
	Mat ScalingMat = (Mat::diag(Mat(trueEfficiencyScale)) * Mat::diag(Mat(sc)));
	
	aligned_img = Mat(0,0, CV_8U);
	warpPerspective( img, aligned_img, H*ScalingMat, aligned_img_sz); //, INTER_LINEAR, BORDER_CONSTANT, Scalar(255));
	
	vector<Point> quad = transformationToQuad(H*ScalingMat, aligned_img_sz);
	
	bool alignSuccess = false;
	if( testQuad(quad) ){ 
		float area = contourArea(Mat(quad));
		float expected_area = .8 * .8 * img.size().area();
		float tolerence = .4;
		alignSuccess =  area > (1. - tolerence) * expected_area &&
						area < (1. + tolerence) * expected_area;
	}
	
	#ifdef OUTPUT_DEBUG_IMAGES
		/*
		//This code creates a window to show matches:
		vector<char> matchesMask( filteredMatches.size(), 0 );
		Mat points1t; perspectiveTransform(Mat(points1), points1t, H);
		for( size_t i1 = 0; i1 < points1.size(); i1++ )
		{
		    if( norm(points2[i1] - points1t.at<Point2f>((int)i1,0)) < 4 ) // inlier
		        matchesMask[i1] = 1;
		}
	
		Mat drawImg;
		Mat img2 = imread("lfdImage.jpg");
		drawMatches( img_resized, keypoints1, img2, templKeypoints, filteredMatches, drawImg,
					CV_RGB(0, 255, 0), CV_RGB(255, 0, 255), matchesMask, DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
	
		namedWindow("inliers", CV_WINDOW_NORMAL);
		imshow( "inliers", drawImg );
		
        for( size_t i1 = 0; i1 < matchesMask.size(); i1++ )
            matchesMask[i1] = !matchesMask[i1];
        drawMatches( img_resized, keypoints1, img2, templKeypoints, filteredMatches, drawImg,
        			CV_RGB(0, 0, 255), CV_RGB(255, 0, 0), matchesMask, DrawMatchesFlags::DRAW_OVER_OUTIMG | DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
		
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
		}*/
		
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
	return alignSuccess;
}
vector<Point> findFormQuad(const Mat& img){
	return findQuad(img, 12);
}
vector<Point> findBoundedRegionQuad(const Mat& img, float buffer){
	//Turning the blur up really seems to help...
	return findQuad(img, 8, buffer);
}
//Aligns a image of a form.
bool alignFormImage(const Mat& img, Mat& aligned_img, const string& featureDataPath, 
					const Size& aligned_img_sz, float efficiencyScale ) {
	#ifdef USE_FEATURE_BASED_FORM_ALIGNMENT
	return alignFormImageByFeatures(img, aligned_img, featureDataPath,  aligned_img_sz, efficiencyScale );
	#else
	vector<Point> quad = findQuad(formImage);
	Mat transformation = quadToTransformation(quad, aligned_img_sz);
	Mat alignedSegment(0, 0, CV_8U);
	warpPerspective(segment, alignedSegment, transformation, aligned_img_sz);
	return !alignedSegment.empty();
	#endif
}
//GrabCut stuff that doesn't work particularly well:
Mat makeGCMask(const Size& mask_size, float buffer){
	Mat mask(mask_size, CV_8UC1, Scalar::all(GC_BGD));
	Point mask_sz_pt( mask_size.width-1, mask_size.height-1);
	
	/*
	float bgd_width = .01;
	float pr_bgd_width = buffer - .01;
	float pr_fgd_width = .1;
	*/
	float bgd_width = buffer + .08;
	float pr_bgd_width = .02;
	float pr_fgd_width = .05;
	
	rectangle( mask, bgd_width * mask_sz_pt, (1.f - bgd_width) * mask_sz_pt, GC_PR_BGD, -1);
	rectangle( mask, (bgd_width + pr_bgd_width) * mask_sz_pt,
					 (1.f - bgd_width - pr_bgd_width) * mask_sz_pt, GC_PR_FGD, -1);
 	rectangle( mask, (bgd_width + pr_bgd_width + pr_fgd_width) * mask_sz_pt,
					 (1.f - bgd_width - pr_bgd_width - pr_fgd_width) * mask_sz_pt, GC_FGD, -1);
	return mask;
}

//Looks for a bounded rectanular quadrilateral
vector<Point> findSegment(const Mat& img, float buffer){
	Mat img2, fgdModel, bgdModel;
	cvtColor(img, img2, CV_GRAY2RGB);
	/*
	vector<Mat> components;
	components.push_back(img);
	Mat temp, temp2;
	erode(img, temp, Mat());
	components.push_back(temp);
	Laplacian(img, temp2, -1, 3, 5);
	components.push_back(temp2);
	//merge(components, img2);
	cvtColor(temp2, img2, CV_GRAY2RGB);
	*/
	Mat mask = makeGCMask(img.size(), buffer/(1 + 2*buffer));
	
	grabCut( img2, mask, Rect(), bgdModel, fgdModel, 1, GC_INIT_WITH_MASK );
	//watershed(img2, mask);
	
	
	string segfilename = alignmentNamer.get_unique_name("alignment_debug_");
	segfilename.append(".jpg");
	
	//imwrite(segfilename, mask*50);
	
	mask = mask & GC_FGD;// | (mask & GC_PR_FGD));
	
	Mat dbg_out;
	img2.copyTo( dbg_out , mask); 
	imwrite(segfilename, dbg_out);
	
	return findMaxQuad(mask, 0);
}

