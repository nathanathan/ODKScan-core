#include "configuration.h"
#include "FormAlignment.h"

#ifdef USE_ANDROID_HEADERS_AND_IO
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/imgproc/imgproc.hpp>
#else
#include "highgui.h"
#endif

#define DEBUG_ALIGN_IMAGE 1

#if DEBUG_ALIGN_IMAGE > 0
#include "NameGenerator.h"
NameGenerator alignmentNamer("debug_segment_images/");
#endif

//Image straightening constants
#define DILATION 6
#define BLOCK_SIZE 3
#define DIST_PARAM 500

//image_align constants
#define THRESH_OFFSET_LB -.3
#define THRESH_DECR_SIZE .05

#define EXPANSION_PERCENTAGE 0

using namespace cv;

//Takes a vector of found corners, and an array of destination corners they should map to
//and replaces each corner in the dest_corners with the nearest unmatched found corner.
template <class Tp>
void configCornerArray(vector<Tp>& found_corners, Point2f* dest_corners) {
	float min_dist;
	int min_idx;
	float dist;

	vector<Point2f> corners;

	for(int i = 0; i < found_corners.size(); i++ ){
		corners.push_back(Point2f(float(found_corners[i].x), float(found_corners[i].y)));
	}
	for(int i = 0; i < 4; i++) {
		min_dist = FLT_MAX;
		for(int j = 0; j < corners.size(); j++ ){
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
void expandCorners(Point2f* corners, double expansionPercent){
	Point2f center(0,0);
	for(size_t i = 0; i < 4; i++){
		center += corners[i];
	}
	center *= .25;
	for(size_t i = 0; i < 4; i++){
		corners[i] += expansionPercent * (corners[i] - center);
	}
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
	float prev_area = 0;

	vector <Point> approx;
	
	float arc_len = arcLength(Mat(contour), true);
	while (current_approx_p < 1) {
		approxPolyDP(Mat(contour), approx, arc_len * current_approx_p, true);
		if (approx.size() == 4){
			maxQuad = approx;
			prev_area = area;
			area = fabs(contourArea(Mat(approx)));
			if( area <= prev_area ) {
				break;
			}
		}
		current_approx_p += .01;
	}
	return prev_area;
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
	
	#if DEBUG_ALIGN_IMAGE > 0
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
	
	return quad;
}
Mat getMyTransform(vector<Point>& foundCorners, Size init_image_sz, Size out_image_sz, bool reverse){
	Point2f corners_a[4] = {Point2f(0, 0), Point2f(init_image_sz.width, 0), Point2f(0, init_image_sz.height),
							Point2f(init_image_sz.width, init_image_sz.height)};
	Point2f out_corners[4] = {Point2f(0, 0),Point2f(out_image_sz.width, 0), Point2f(0, out_image_sz.height),
							Point2f(out_image_sz.width, out_image_sz.height)};

	configCornerArray(foundCorners, corners_a);
	//TODO: Think about this.
	expandCorners(corners_a, EXPANSION_PERCENTAGE);
	if(reverse){
		return getPerspectiveTransform(out_corners, corners_a);
	}
	else{
		return getPerspectiveTransform(corners_a, out_corners);
	}
}
//TODO: Refactor this so that it only does the alignment. The else stuff can be added to quad finding,
//		however I'm not sure it's going to be worth keeping at all.
void alignImage(Mat& img, Mat& aligned_image, vector<Point>& maxRect, Size aligned_image_sz){
	
	if ( maxRect.size() == 4 && isContourConvex(Mat(maxRect)) ){
		#if DEBUG_ALIGN_IMAGE > 4
		//TODO:Possibly remove this
		cvtColor(img, dbg_out, CV_GRAY2RGB);
		const Point* p = &maxRect[0];
		int n = (int) maxRect.size();
		polylines(dbg_out, &p, &n, 1, true, 200, 2, CV_AA);
		string segfilename = alignmentNamer.get_unique_name("alignment_debug_");
		segfilename.append(".jpg");
		imwrite(segfilename, dbg_out);
		#endif
		Mat H = getMyTransform(maxRect, img.size(), aligned_image_sz);
		warpPerspective(img, aligned_image, H, aligned_image_sz);
	}
	else{//use the bounding line method if the contour method fails
		int top = 0, bottom = 0, left = 0, right = 0;
		find_bounding_lines(img, &top, &bottom, false);
		find_bounding_lines(img, &left, &right, true);

		#if DEBUG_ALIGN_IMAGE > 5
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
		#endif
		
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
//Aligns a image of a form.
void alignFormImage(Mat& img, Mat& aligned_image, Size aligned_image_sz, int blurSize){
	//Align paper
	//Find where the content begins:
	//Add a parameters to set the expected locations and search radius
	//Also consider weighting this...
	//find_bounding_lines(Mat& img, int* upper, int* lower, bool vertical);
	//Once you have the content rectangle crop. Possibly resize (which will hurt performance)
	//It might help to have a content rectangle in the JSON (which should indicate where the ink starts).
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
	return findQuad(img, 2);
}


//DEPRECATED
//A form straitening method based on finding the corners of the sheet of form paper.
//The form will be resized to the size of output_image.
//It might save some memory to specify a Size object instead of a preallocated Mat.
void straightenImage(const Mat& input_image, Mat& output_image) {
	Point2f orig_corners[4];
	Point2f corners_a[4];
	vector < Point2f > corners;

	Mat tmask, input_image_dilated;

	// Create a mask that limits corner detection to the corners of the image.
	tmask= Mat::zeros(input_image.rows, input_image.cols, CV_8U);
	circle(tmask, Point(0,0), (tmask.cols+tmask.rows)/8, Scalar(255,255,255), -1);
	circle(tmask, Point(0,tmask.rows), (tmask.cols+tmask.rows)/8, Scalar(255,255,255), -1);
	circle(tmask, Point(tmask.cols,0), (tmask.cols+tmask.rows)/8, Scalar(255,255,255), -1);
	circle(tmask, Point(tmask.cols,tmask.rows), (tmask.cols+tmask.rows)/8, Scalar(255,255,255), -1);

	//orig_corners = {Point(0,0),Point(img.cols,0),Point(0,img.rows),Point(img.cols,img.rows)};
	orig_corners[0] = Point(0,0);
	orig_corners[1] = Point(output_image.cols,0);
	orig_corners[2] = Point(0,output_image.rows);
	orig_corners[3] = Point(output_image.cols,output_image.rows);

	// Dilating reduces noise, thin lines and small marks.
	dilate(input_image, input_image_dilated, Mat(), Point(-1, -1), DILATION);

	/*
	Params for goodFeaturesToTrack:
	Source Mat, Dest Mat
	Number of features/interest points to return
	Minimum feature quality
	Min distance between corners (Probably needs parameterization depending on im. res. and form)
	Mask
	Block Size (not totally sure but I think it's similar to aperture)
	Use Harris detector (true) or cornerMinEigenVal() (false)
	Free parameter of Harris detector
	*/
	goodFeaturesToTrack(input_image_dilated, corners, 4, 0.01, DIST_PARAM, tmask, BLOCK_SIZE, false, 0.04);

	// Initialize the value of corners_a to that of orig_corners
	memcpy(corners_a, orig_corners, sizeof(orig_corners));
	configCornerArray(corners, corners_a);

	Mat H = getPerspectiveTransform(corners_a, orig_corners);
	
	warpPerspective(input_image, output_image, H, output_image.size());

}
