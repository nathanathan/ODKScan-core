#include "AlignmentUtils.h"
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

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
Mat quadToTransformation(const vector<Point>& foundCorners, const Size& out_image_sz) {

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
bool testQuad(const vector<Point>& quad) {

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
