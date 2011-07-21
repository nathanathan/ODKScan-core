#include "configuration.h"
#include "FormAlignment.h"
#include "Addons.h"

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

#define FH_REPROJECT_THRESH 8.0

using namespace std;
using namespace cv;

//Order a 4 point vector clockwise with the 0 index at the most top-left corner
vector<Point> orderCorners(const vector<Point>& corners){
	
	vector<Point> orderedCorners;
	
	Moments m = moments(Mat(corners));
	//Point center(m.m10/m.m00, m.m01/m.m00);
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
	
	//TODO: Turn on at checking...
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

Mat makeGCMask(const Size& mask_size, float buffer){
	Mat mask(mask_size, CV_8UC1, Scalar::all(GC_BGD));
	Point mask_sz_pt( mask_size.width-1, mask_size.height-1);
	
	float bgd_width = .01;
	float pr_bgd_width = buffer - .01;
	float pr_fgd_width = .1;
	
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
	
	grabCut( img2, mask, Rect(), bgdModel, fgdModel, 2, GC_INIT_WITH_MASK );
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

//TODO: The blurSize param is sort of a hacky solution to the problem of contours being too large
//		in certain cases. It fixes the problem on some form images because they can be blurred a lot
//		and produce good results for contour finding. This is not the case with segments.
//		I think a better solution might be to alter maxQuad somehow... I haven't figured out how though.
//TODO: I'm considering an alternative implementation where I do segmentation seeded around the center of the image.
//		then find a bounding rectangle.
vector<Point> findQuad(const Mat& img, int blurSize){
	Mat imgThresh, temp_img, temp_img2;
	
	//Shrink the image down for efficiency
	//and so we don't have to worry about filters behaving differently on large images
	int multiple = img.cols / 256;
	if(multiple > 1){
		resize(img, temp_img, Size(img.cols/multiple, img.rows/multiple), INTER_AREA);
	}
	else{
		multiple = 1;
		img.copyTo(temp_img);
	}
	
	float actual_width_multiple = float(img.rows) / temp_img.rows;
	float actual_height_multiple = float(img.cols) / temp_img.cols;
	
	temp_img.copyTo(temp_img2);
	blur(temp_img2, temp_img, Size(2*blurSize+1, 2*blurSize+1));
	//This threshold might be tweakable
	imgThresh = temp_img2 - temp_img > 0;
	
	#ifdef OUTPUT_DEBUG_IMAGES
	
	Mat dbg_out, dbg_out2;
	imgThresh.copyTo(dbg_out);
	//img.copyTo(dbg_out2);
	string segfilename = alignmentNamer.get_unique_name("alignment_debug_");
	segfilename.append(".jpg");
	/*
    vector<Vec2f> lines;
    HoughLines(dbg_out, lines, 1, CV_PI/180, 10, 0, 0 );
    
    for( size_t i = 0; i < 4; i++ )
    {
        float rho = lines[i][0], theta = lines[i][1];
        Point pt1, pt2;
        double a = cos(theta), b = sin(theta);
        double x0 = a*rho, y0 = b*rho;
        pt1.x = cvRound(x0 + 1000*(-b));
        pt1.y = cvRound(y0 + 1000*(a));
        pt2.x = cvRound(x0 - 1000*(-b));
        pt2.y = cvRound(y0 - 1000*(a));
        line( dbg_out2, pt1, pt2, 255, 2, CV_AA);
    }*/
	
	imwrite(segfilename, dbg_out);
	//img.copyTo(dbg_out);
	
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
//TODO: move to file utils
bool fileexists(const string& filename){
  ifstream ifile(filename.c_str());
  return (bool) ifile;
}
//Tries to read feature data (presumably for the template) from featureDataPath + ".yml" .
//If none is found it is generated for the image featureDataPath + ".jpg"
bool loadFeatureData(const string& featureDataPath, Ptr<FeatureDetector>& detector,
					Ptr<DescriptorExtractor>& descriptorExtractor, vector<KeyPoint>& templKeypoints,
					Mat& templDescriptors, Size& templImageSize) {
			
	//detector = Ptr<FeatureDetector>(new SurfFeatureDetector(300));
	//MSER is pretty fast. Grid seems to help limit number but messes up more.
	detector = FeatureDetector::create( "SURF" ); 
	descriptorExtractor = DescriptorExtractor::create( "SURF" );
	
	bool checkForSavedFeatures = true;
	#ifdef ALWAYS_COMPUTE_TEMPLATE_FEATURES
	checkForSavedFeatures = false;
	#endif
	if( checkForSavedFeatures && fileexists(featureDataPath + ".yml") ) {
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
		detector->detect( templImage, templKeypoints );
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
		
		//imwrite("t2.jpg", templImage);
	}
	if( detector.empty() || descriptorExtractor.empty() )
	{
		cerr << "Can not create/load detector or descriptor extractor" << endl;
		return false;
	}	
	
	return true;
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
//Aligns a image of a form.
bool alignFormImage(const Mat& img, Mat& aligned_img, const string& featureDataPath, 
					const Size& aligned_img_sz, float efficiencyScale ){
					
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
	//cvtColor(temp, img1, CV_GRAY2RGB);
	//GaussianBlur(temp, img1, Size(1, 1), 1.0);
	
	//imwrite("t1.jpg", img_resized);
	
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
vector<Point> findBoundedRegionQuad(const Mat& img){
	return findQuad(img, 4);
}
