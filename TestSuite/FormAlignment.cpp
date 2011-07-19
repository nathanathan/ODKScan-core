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

using namespace std;
using namespace cv;

//Takes a vector of found corners, and an array of destination corners they should map to
//and replaces each corner in the dest_corners with the nearest unmatched found corner.
template <class Tp>
void configCornerArray(vector<Tp>& found_corners, Point2f* dest_corners) {
	float min_dist;
	int min_idx;
	float dist;

	vector<Point2f> corners;

	for(size_t i = 0; i < found_corners.size(); i++ ){
		corners.push_back(Point2f(float(found_corners[i].x), float(found_corners[i].y)));
	}
	for(size_t i = 0; i < 4; i++) {
		min_dist = FLT_MAX;
		for(size_t j = 0; j < corners.size(); j++ ){
			dist = norm(corners[j]-dest_corners[i]);
			if(dist < min_dist){
				min_dist = dist;
				min_idx = j;
			}
		}
		dest_corners[i]=corners[min_idx]; // + expand * (dest_corners[i] - corners[min_idx]);
		corners.erase(corners.begin()+min_idx);
	}
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
//Finds two vertical or horizontal lines that have the minimal gradient sum.
void find_bounding_lines(Mat& img, int* upper, int* lower, bool vertical) {
	Mat grad_img, out;
	
	int center_size;
	if( vertical ){
		// Watch out, I haven't tested to make sure these aren't backwards.
		center_size = img.cols/4;
	}
	else{
		center_size = img.rows/4;
	}
	
	Sobel(img, grad_img, 0, int(!vertical), int(vertical));

	reduce(grad_img, out, int(!vertical), CV_REDUCE_SUM, CV_32F);
	
	//GaussianBlur(out, out, Size(1, center_size/4), 1.0);

	if( vertical )
		transpose(out,out);

	Point min_location_top;
	Point min_location_bottom;
	minMaxLoc(out(Range(3, out.rows/2 - center_size), Range(0,1)), NULL,NULL,&min_location_top);
	minMaxLoc(out(Range(out.rows/2 + center_size,out.rows - 3), Range(0,1)), NULL,NULL,&min_location_bottom);
	*upper = min_location_top.y;
	*lower = min_location_bottom.y + out.rows/2 + center_size;
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

//TODO: The blurSize param is sort of a hacky solution to the problem of contours being too large
//		in certain cases. It fixes the problem on some form images because they can be blurred a lot
//		and produce good results for contour finding. This is not the case with segments.
//		I think a better solution might be to alter maxQuad somehow... I haven't figured out how though.
//TODO: I'm considering an alternative implementation where I do segmentation seeded around the center of the image.
//		then find a bounding rectangle.
vector<Point> findQuad(Mat& img, int blurSize){
	Mat imgThresh, temp_img, temp_img2;
	
	//Shrink the image down for efficiency
	//and so we don't have to worry about filters behaving differently on large images
	int multiple = img.cols / 256;
	if(multiple > 1){
		resize(img, temp_img, Size(img.cols/multiple, img.rows/multiple));
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
	Mat dbg_out;
	imgThresh.copyTo(dbg_out);
	string segfilename = alignmentNamer.get_unique_name("alignment_debug_");
	segfilename.append(".jpg");
	imwrite(segfilename, dbg_out);
	#endif
	
	vector<Point> quad = findMaxQuad(imgThresh, 0);
	
	imgThresh.release();
	
	//Resize the contours for the full size image:
	for(size_t i = 0; i<quad.size(); i++){
		quad[i] = Point(actual_width_multiple * quad[i].x, actual_height_multiple * quad[i].y);
	}
	/*
	Moments m = moments(Mat(quad));
	//Mat Z = (Mat_<double>(1,3) << m.m20, m.m02, 0 );
	Point center(m.m10/m.m00, m.m01/m.m00);
	vector<Point> quad2(4);
	for(size_t i = 0; i<4; i++){
		Point d = quad[i]-center;
		if(d.y > 0){
			if(d.x < 0){
				quad2[0] = quad[i];
			}
			else{
				quad2[1] = quad[i];
			}
		}
		else{
			if(d.x > 0){
				quad2[2] = quad[i];
			}
			else{
				quad2[3] = quad[i];
			}
		}
		cout << d.x << endl;
	}
	quad = quad2; */
	/*
	Moments m = moments(Mat(quad));
	//Point center(m.m10/m.m00, m.m01/m.m00);
	Mat center = (Mat_<double>(1,3) << m.m10/m.m00, m.m01/m.m00, 0 );
	Mat p0 = (Mat_<double>(1,3) << quad[0].x, quad[0].y, 0 );
	Mat p1 = (Mat_<double>(1,3) << quad[1].x, quad[1].y, 0 );
	if((center - p0).cross(p1 - p0).at<double>(0,2) > 0){
		quad = vector<Point>(quad.rbegin(), quad.rend());
	}
	*/
	
	return expandCorners(quad, EXPANSION_PERCENTAGE);
}
//TODO: This routine could be "simplified" by taking out init_image_sz and using angle from center to order vertices.
//		Reverse is probably also unnecessiary since it can probably be accomplished with inversion.
//		And quadToTransformation might then be a more consistent name.
Mat getMyTransform(vector<Point>& foundCorners, Size init_image_sz, Size out_image_sz, bool reverse){
	Point2f corners_a[4] = {Point2f(0, 0), Point2f(init_image_sz.width, 0), Point2f(0, init_image_sz.height),
							Point2f(init_image_sz.width, init_image_sz.height)};
	Point2f out_corners[4] = {Point2f(0, 0),Point2f(out_image_sz.width, 0), Point2f(0, out_image_sz.height),
							Point2f(out_image_sz.width, out_image_sz.height)};

	configCornerArray(foundCorners, corners_a);
	
	if(reverse){
		return getPerspectiveTransform(out_corners, corners_a);
	}
	else{
		return getPerspectiveTransform(corners_a, out_corners);
	}
}
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
//TODO: Refactor this so that it only does the alignment. The else stuff can be added to quad finding,
//		however I'm not sure it's going to be worth keeping at all.
void alignImage(Mat& img, Mat& aligned_image, vector<Point>& maxRect, Size aligned_image_sz){
	
	if ( maxRect.size() == 4 && isContourConvex(Mat(maxRect)) ){
		/*
		//TODO:Possibly remove this
		cvtColor(img, dbg_out, CV_GRAY2RGB);
		const Point* p = &maxRect[0];
		int n = (int) maxRect.size();
		polylines(dbg_out, &p, &n, 1, true, 200, 2, CV_AA);
		string segfilename = alignmentNamer.get_unique_name("alignment_debug_");
		segfilename.append(".jpg");
		imwrite(segfilename, dbg_out);
		*/
		Mat H = getMyTransform(maxRect, img.size(), aligned_image_sz);
		warpPerspective(img, aligned_image, H, aligned_image_sz);
	}
	else{//use the bounding line method if the contour method fails
		int top = 0, bottom = 0, left = 0, right = 0;
		find_bounding_lines(img, &top, &bottom, false);
		find_bounding_lines(img, &left, &right, true);

		/*
		img.copyTo(dbg_out);
		const Point* p = &maxRect[0];
		int n = (int) maxRect.size();
		polylines(dbg_out, &p, &n, 1, true, 200, 2, CV_AA);

		dbg_out.row(top)+=200;
		dbg_out.row(bottom)+=200;
		dbg_out.col(left)+=200;
		dbg_out.col(right)+=200;
		string segfilename = alignmentNamer.get_unique_name("alignment_debug_");
		segfilename.append(".jpg");
		imwrite(segfilename, dbg_out);
		*/
		
		float bounding_lines_threshold = .2;
		if ((abs((bottom - top) - aligned_image.rows) < bounding_lines_threshold * aligned_image.rows) &&
			(abs((right - left) - aligned_image.cols) < bounding_lines_threshold * aligned_image.cols) &&
			top + aligned_image.rows < img.rows &&  top + aligned_image.cols < img.cols) {

			img(Rect(left, top, aligned_image.cols, aligned_image.rows)).copyTo(aligned_image);

		}
		else{
			int seg_buffer_w = (img.cols - aligned_image.cols) / 2;
			int seg_buffer_h = (img.rows - aligned_image.rows) / 2;
			img(Rect(seg_buffer_w, seg_buffer_h,
				aligned_image.cols, aligned_image.rows)).copyTo(aligned_image);
		}
	}
}

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
bool fileexists(const string& filename){
  ifstream ifile(filename.c_str());
  return ifile;
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
		cout << endl << "Extracting keypoints from template image..." << endl;
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
bool alignFormImage(Mat& img, Mat& aligned_img, const string& featureDataPath, 
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
	cout << endl << "Extracting keypoints from unaligned image..." << endl;
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
	cout << ">" << endl;

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
	Mat H = findHomography( Mat(points1), Mat(points2), CV_RANSAC, 5.0 );
	Mat ScalingMat = (Mat::diag(Mat(trueEfficiencyScale)) * Mat::diag(Mat(sc)));
	
	warpPerspective( img, aligned_img, H*ScalingMat, aligned_img_sz );
	
	#ifdef OUTPUT_DEBUG_IMAGES
	vector<Point> quad = transformationToQuad(H*ScalingMat, aligned_img_sz);
	
	bool alignSuccess = false;
	if( testQuad(quad) ){ 
		float area = contourArea(Mat(quad));
		float expected_area = .8 * img.size().area();
		float tolerence = .2;
		alignSuccess =  area > (1. - tolerence) * expected_area &&
						area < (1. + tolerence) * expected_area;
	}
	
	string qiname = dbgNamer.get_unique_name("alignment_debug_") + ".jpg";
	const Point* p = &quad[0];
	int n = (int) quad.size();
	if( alignSuccess ){
		polylines(img, &p, &n, 1, true, 250, 3, CV_AA);
	}
	else{
		polylines(img, &p, &n, 1, true, 0, 5, CV_AA);
		cout << "Form alignment failed" << endl;
	}
	imwrite(qiname, img);
	#endif
	return true;
}
//Aligns a region bounded by black lines (i.e. a bubble segment)
//It might be necessiary for some of the black lines to touch the edge of the image...
//TODO: see if that's the case, and try to do something about it if it is.
void alignBoundedRegion(Mat& img, Mat& aligned_image, Size aligned_image_sz){
	return;
}
vector<Point> findFormQuad(Mat& img){
	return findQuad(img, 12);
}
vector<Point> findBoundedRegionQuad(Mat& img){
	return findQuad(img, 4);
}
