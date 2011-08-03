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

//image_align constants
#define THRESH_OFFSET_LB -.3
#define THRESH_DECR_SIZE .05

#define EXPANSION_PERCENTAGE .04

#define USE_FEATURE_BASED_FORM_ALIGNMENT
//vaying the reproject threshold can make a big difference in performance.
#define FH_REPROJECT_THRESH 3.0

//#define ALWAYS_COMPUTE_TEMPLATE_FEATURES
//#define SHOW_MATCHES_WINDOW

#ifdef OUTPUT_DEBUG_IMAGES
#include "NameGenerator.h"
NameGenerator alignmentNamer("debug_segment_images/");
NameGenerator dbgNamer("debug_form_images/", true);
#endif

using namespace std;
using namespace cv;

//Order a 4 point vector clockwise with the 0 index at the most top-left corner
vector<Point> orderCorners(const vector<Point>& corners){

	vector<Point> orderedCorners;

	Moments m = moments(Mat(corners));

	Mat center = (Mat_<double>(1,3) << m.m10/m.m00, m.m01/m.m00, 0 );
	Mat p0 = (Mat_<double>(1,3) << corners[0].x, corners[0].y, 0 );
	Mat p1 = (Mat_<double>(1,3) << corners[1].x, corners[1].y, 0 );

	if((center - p0).cross(p1 - p0).at<double>(0,2) < 0){ //Double-check this math just in case
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
//Sum the pixels that lie on a line starting and ending in the specified rows.
int lineSum(const Mat& img, int start, int end, bool transpose) {

	int hSpan;
	if(transpose){
		hSpan = img.rows - 1;
	}
	else{
		hSpan = img.cols - 1;
	}
	
	int sum = 0;
	double slope = (double)(end - start)/hSpan;
	
	for(int i = 0; i<=hSpan; i++) {
		int j = start + slope*i;

		if(j < 0){
			sum+=127;
		}
		else{
			if(transpose){
				sum += img.at<uchar>(i, j);
			}
			else{
				sum += img.at<uchar>(j, i);
			}
		}
	}
	return sum;
}
void findLinesHelper(const Mat& img, int& start, int& end, const Rect& roi, bool flip, bool transpose) {
	int vSpan, hSpan;
	int range, midpoint;
	float maxSlope = .15;

	if(transpose){
		vSpan = img.cols - 1;
		hSpan = img.rows - 1;
		midpoint = roi.x;
		range = roi.y;
	}
	else{		
		vSpan = img.rows - 1;
		hSpan = img.cols - 1;
		midpoint = roi.y;
		range = roi.y;
	}
	
	//The param limits the weigting to a certain magnitude, in this case 10% of the max.
	int param = .15 * 255 * (hSpan + 1);
	float maxSsdFromMidpoint = 2*range*range;
	
	int minLs = INT_MAX;
	for(int i = midpoint - range; i < midpoint + range; i++) {
		for(int j = MAX(i-hSpan*maxSlope, midpoint - range); j < MIN(i+hSpan*maxSlope, midpoint + range); j++) {

			float ssdFromMidpoint = (i - midpoint)*(i - midpoint) + (j - midpoint)*(j - midpoint);
			int ls = param * ssdFromMidpoint / maxSsdFromMidpoint;
			if(flip){
				ls += lineSum(img, vSpan - i, vSpan - j, transpose);
			}
			else{
				ls += lineSum(img, i, j, transpose);
			}
			if( ls < minLs ){
				start = i;
				end = j;
				minLs = ls;
			}
		}
	}
}
//Find the minimum energy lines crossing the image.
//A and B are the start and end points of the line.
void findLines(const Mat& img, Point& A, Point& B, const Rect& roi, bool flip, bool transpose) {
	int start, end;
	
	findLinesHelper(img, start, end, roi, flip, transpose);
	
	if(flip && transpose){
		A = Point(img.cols - 1 - start, img.rows-1);
		B = Point(img.cols - 1 - end, 0);
	}
	else if(!flip && transpose){
		A = Point(start, 0);
		B = Point(end, img.rows -1);
	}
	else if(flip && !transpose){
		A = Point(0, img.rows - 1 - start);
		B = Point(img.cols-1, img.rows - 1 - end);
	}
	else{
		A = Point(0, start);
		B = Point(img.cols-1, end);
	}
}
inline Point findIntersection(const Point& P1, const Point& P2,
						const Point& P3, const Point& P4){
	// From determinant formula here:
	// http://en.wikipedia.org/wiki/Line_intersection
	int denom = (P1.x - P2.x) * (P3.y - P4.y) - (P1.y - P2.y) * (P3.x - P4.x);
	return Point(
		( (P1.x * P2.y - P1.y * P2.x) * (P3.x - P4.x) -
		  (P1.x - P2.x) * (P3.x * P4.y - P3.y * P4.x) ) / denom,
		( (P1.x * P2.y - P1.y * P2.x) * (P3.y - P4.y) -
		  (P1.y - P2.y) * (P3.x * P4.y - P3.y * P4.x) ) / denom);
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
		resize(img, temp_img, (1.f / multiple) * img.size(), 0, 0, INTER_AREA);
	}
	else{
		multiple = 1;
		img.copyTo(temp_img);
	}
	
	float actual_width_multiple = float(img.rows) / temp_img.rows;
	float actual_height_multiple = float(img.cols) / temp_img.cols;
	
	temp_img.copyTo(temp_img2);
	#if 1
	blur(temp_img2, temp_img, 2*Size(2*blurSize+1, 2*blurSize+1));
	#else
	//Not sure if this had advantages or not...
	//My theory is that it is slower but more accurate,
	//but I don't know if either difference is significant enough to notice.
	//Will need to test.
	GaussianBlur(temp_img2, temp_img, Size(9, 9), 3, 3);
	#endif
	
	//erode(temp_img2, temp_img, (Mat_<uchar>(3,3) << 1,0,1,0,1,0,1,0,1));
	//This threshold might be tweakable
	imgThresh = (temp_img2 - temp_img) > 0;

	float bufferChoke = buffer/(1 + 2*buffer);
	Rect roi(bufferChoke * Point(temp_img.cols, temp_img.rows), (1.f - 2*bufferChoke) * temp_img.size());
	
	//Blocking out a chunk in the middle of the segment can help in cases with densely filled bubbles.
	float extendedChoke = bufferChoke + .2;
	Rect contractedRoi(extendedChoke * Point(temp_img.cols, temp_img.rows), (1.f - 2*extendedChoke) * temp_img.size());
	imgThresh(contractedRoi) = Scalar::all(255);

	Point A1, B1, A2, B2, A3, B3, A4, B4;
	findLines(imgThresh, A1, B1, roi, false, true);
	findLines(imgThresh, A2, B2, roi, false, false);
	findLines(imgThresh, A3, B3, roi, true, true);
	findLines(imgThresh, A4, B4, roi, true, false);
	
	#define USE_INTERSECTIONS
	#ifdef USE_INTERSECTIONS
	vector <Point> quad;
	quad.push_back(findIntersection(A1, B1, A2, B2));
	quad.push_back(findIntersection(A2, B2, A3, B3));
	quad.push_back(findIntersection(A3, B3, A4, B4));
	quad.push_back(findIntersection(A4, B4, A1, B1));
	#else
	line( imgThresh, A1, B1, Scalar(130), 1, 4);
	line( imgThresh, A2, B2, Scalar(130), 1, 4);
	line( imgThresh, A3, B3, Scalar(130), 1, 4);
	line( imgThresh, A4, B4, Scalar(130), 1, 4);
	
	vector<Point> quad = findMaxQuad(imgThresh, 0);
	
	quad = expandCorners(quad, EXPANSION_PERCENTAGE);
	#endif
	
	#if 0 
	//refine corner locations
	//This seems to actually do worse
	vector <Point2f> fquad;
	for(size_t i = 0; i<quad.size(); i++){
		fquad.push_back(Point2f(actual_width_multiple * quad[i].x, actual_height_multiple * quad[i].y));
	}
	TermCriteria termcrit(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03);
	cornerSubPix(img, fquad, Size(3,3), Size(2, 2), termcrit);
	
	for(size_t i = 0; i<quad.size(); i++){
		quad[i] = Point(fquad[i].x, fquad[i].y);
	}
	#else
	//Resize the contours for the full size image:
	for(size_t i = 0; i<quad.size(); i++){
		quad[i] = Point(actual_width_multiple * quad[i].x, actual_height_multiple * quad[i].y);
	}
	#endif
	
	#ifdef OUTPUT_DEBUG_IMAGES
	Mat dbg_out, dbg_out2;
	imgThresh.copyTo(dbg_out);
	string segfilename = alignmentNamer.get_unique_name("alignment_debug_");
	segfilename.append(".jpg");
	imwrite(segfilename, dbg_out);
	#endif

	return quad;
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
//Takes a 3x3 transformation matrix H and a transformed output image size and returns
//a quad representing the location of the output image in a image to be transformed by H.
vector<Point> transformationToQuad(const Mat& H, const Size& out_image_sz){
										
	Mat img_rect = (Mat_<double>(3,4) << 0, out_image_sz.width, (double)out_image_sz.width,	 0,
										 0, 0,					(double)out_image_sz.height, out_image_sz.height,
										 1,	1,					1, 					 1);

	Mat out_img_rect =  H.inv() * img_rect;
	
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
//I think the proper way to do this would be to make a FormAlignment class
//then pass in the mask from Processor.cpp
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
							vector<KeyPoint>& templKeypoints, Mat& templDescriptors){
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
//Tries to read feature data (presumably for the template) from templPath + ".yml" .
//If none is found it is generated for the image templPath + ".jpg"
bool loadFeatureData(const string& templPath, Ptr<FeatureDetector>& detector,
					Ptr<DescriptorExtractor>& descriptorExtractor, vector<KeyPoint>& templKeypoints,
					Mat& templDescriptors, Size& templImageSize) {
			
	bool featuresFound = false;
	#ifndef ALWAYS_COMPUTE_TEMPLATE_FEATURES
		featuresFound = checkForSavedFeatures(templPath + ".yml", templImageSize, templKeypoints, templDescriptors);
	#endif
	
	//detector = Ptr<FeatureDetector>(new SurfFeatureDetector(800, 4, 1));
	//detector = Ptr<FeatureDetector>(new GoodFeaturesToTrackDetector( 800, .2, 10));
	//MSER is pretty fast. Grid seems to help limit number but messes up more.
	//descriptorExtractor = Ptr<DescriptorExtractor>(new SurfDescriptorExtractor( 4, 1, true ));
	//descriptorExtractor = Ptr<DescriptorExtractor>(new SurfDescriptorExtractor( 4, 3, true ));
	//detector = FeatureDetector::create( "SURF" ); 
	//detector = FeatureDetector::create( "GridSURF" );
	
	//#define SHOW_MATCHES_WINDOW
	//#define ALWAYS_COMPUTE_TEMPLATE_FEATURES
	detector = Ptr<FeatureDetector>(new GridAdaptedFeatureDetector(
										new SurfFeatureDetector( 400., 1, 3), //Adding octaves while shrinking the image might speed things up.
										500, 4, 4));//4,4 grid size seems to be bizzarly more effective than other sizes.
	
	descriptorExtractor = DescriptorExtractor::create( "SURF" );
	
	if(detector.empty() || descriptorExtractor.empty()) return false;
	
	if( !featuresFound  ){
		//if there is no file to read descriptors and keypoints from make one.
		Mat templImage, temp;
		templImage = imread( templPath + ".jpg", 0 );
		resize(templImage, temp, templImage.size(), 0, 0, INTER_AREA);
		templImage = temp;
		temp.release();
		templImageSize = templImage.size();

		#ifdef DEBUG_ALIGN_IMAGE
		cout << "Extracting keypoints from template image..." << endl;
		#endif
		
		Mat mask = makeFieldMask(templPath + ".json");
		resize(mask, temp, templImage.size(), 0, 0, INTER_AREA);
		erode(temp, mask, Mat(), Point(-1,-1), 6);
		//mask = temp;
		/*
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
		*/
		
		temp.release();
		detector->detect( templImage, templKeypoints, mask );

		#ifdef DEBUG_ALIGN_IMAGE
		cout << "\t" << templKeypoints.size() << " points" << endl;
		cout << "Computing descriptors for keypoints from template image..." << endl;
		#endif
		descriptorExtractor->compute( templImage, templKeypoints, templDescriptors );
		
		//TODO: Maybe put this in a function
		try {
			// write feature data to a file.
			FileStorage fs(templPath + ".yml", FileStorage::WRITE);
		
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
	}
	return true;
}
bool alignFormImageByFeatures(const Mat& img, Mat& aligned_img, const string& templPath, 
					const Size& aligned_img_sz, float efficiencyScale ) {

	Ptr<FeatureDetector> detector;
	Ptr<DescriptorExtractor> descriptorExtractor;
	
	vector<KeyPoint> templKeypoints;
	Mat templDescriptors;
	
	Size templImageSize;
	
	bool success = loadFeatureData(templPath, detector, descriptorExtractor,
									templKeypoints, templDescriptors, templImageSize);
	if(!success) return false;
	
	// Be careful when resizing, aliasing can completely break this function.
	Mat img_resized;
	resize(img, img_resized, efficiencyScale * img.size(), 0, 0, INTER_AREA);
	Point3d trueEfficiencyScale(double(img_resized.cols) / img.cols,
								double(img_resized.rows) / img.rows,
								1.0);
	
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
	
	#if 1
	Ptr<DescriptorMatcher> descriptorMatcher = DescriptorMatcher::create( "BruteForce" );
	#else
	Ptr<DescriptorMatcher> descriptorMatcher = DescriptorMatcher::create( "FlannBased" );
	#endif
	
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

	Point3d sc = Point3d( ((double)aligned_img_sz.width) / templImageSize.width,
						  ((double)aligned_img_sz.height) / templImageSize.height,
						  1.0);
	
	Mat H = findHomography( Mat(points1), Mat(points2), CV_RANSAC, FH_REPROJECT_THRESH );//CV_LMEDS
	Mat Hscaled = Mat::diag(Mat(sc)) * H * Mat::diag(Mat(trueEfficiencyScale));

	aligned_img = Mat(0,0, CV_8U);
	warpPerspective( img, aligned_img, Hscaled, aligned_img_sz);
	
	vector<Point> quad = transformationToQuad(Hscaled, aligned_img_sz);
	
	bool alignSuccess = false;
	if( testQuad(quad) ) {
		float area = contourArea(Mat(quad));
		float expected_area = (.8 * img.size()).area();
		float tolerence = .4;
		alignSuccess =  area > (1. - tolerence) * expected_area &&
						area < (1. + tolerence) * expected_area;
	}
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
		Mat img2 = imread("lfdImage.jpg");
		drawMatches( img_resized, keypoints1, img2, templKeypoints, filteredMatches, drawImg,
					CV_RGB(0, 255, 0), CV_RGB(255, 0, 255), matchesMask, DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
	
		namedWindow("inliers", CV_WINDOW_NORMAL);
		imshow( "inliers", drawImg );
		
        for( size_t i1 = 0; i1 < matchesMask.size(); i1++ )
            matchesMask[i1] = !matchesMask[i1];
            
        drawMatches( img_resized, keypoints1, img2, templKeypoints, filteredMatches, drawImg,
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
	return findQuad(img, 9, buffer);
}
//Aligns a image of a form.
bool alignFormImage(const Mat& img, Mat& aligned_img, const string& templPath, 
					const Size& aligned_img_sz, float efficiencyScale ) {
	#ifdef USE_FEATURE_BASED_FORM_ALIGNMENT
	return alignFormImageByFeatures(img, aligned_img, templPath,  aligned_img_sz, efficiencyScale );
	#else
	vector<Point> quad = findQuad(formImage);
	Mat transformation = quadToTransformation(quad, aligned_img_sz);
	Mat alignedSegment(0, 0, CV_8U);
	warpPerspective(segment, alignedSegment, transformation, aligned_img_sz);
	return !alignedSegment.empty();
	#endif
}
//GrabCut stuff that doesn't work particularly well
// I will probably get rid of it eventually but it might be help for
// improving feature finding by masking out the form and using a minimum rectangle.
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

	//imwrite(segfilename, mask*50);
	
	mask = mask & GC_FGD;// | (mask & GC_PR_FGD));
	
	#ifdef OUTPUT_DEBUG_IMAGES
	string segfilename = alignmentNamer.get_unique_name("alignment_debug_");
	segfilename.append(".jpg");
	Mat dbg_out;
	img2.copyTo( dbg_out , mask); 
	imwrite(segfilename, dbg_out);
	#endif
	
	return findMaxQuad(mask, 0);
}

