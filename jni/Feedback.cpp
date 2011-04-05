#include "Feedback.h"
#include <sys/stat.h>
#include "log.h"
#define LOG_COMPONENT "BubbleFeedback"

using namespace cv;

/* Feedback
 *
 * This class receives an image of a preview frame and detects the form
 * in the image. It will overlay visual feedback on the image and returns
 * that to the caller.
 */
Feedback::Feedback() {
}

Feedback::~Feedback() {
}

// How far do we allow a point to be away from the corner
// markers to consider it touches the template
const int c_cornerRange = 20;

// Specify the size of the form template
const int c_templateHeight = 360;
const int c_templateWidth = c_templateHeight / 5.5 * 8.5;

const int c_borderMargin = 10;
const int c_borderThreshold = 50;

const char* const c_pszCorrectPos = "Correct Position";
const char* const c_pszRotateLeft = "ROTATE LEFT";
const char* const c_pszRotateRight = "ROTATE RIGHT";
const char* const c_pszMoveCloser = "CENTER THE FORM";
const char* const c_pszScore = "SCORE:";

int g_iScore = 0;

enum Side {
	None = -1, Top, Right, Bottom, Left
};

// Threshold for proper form area
const int c_minOutlineArea = 180000;
// Number of rows of the camera image
static int g_maxRows = -1, g_maxCols = -1;
static Point g_ptUpperLeft, g_ptLowerRight;
long g_nAcross, g_nTopLeft, g_nTopRight, g_nBottomLeft, g_nBottomRight;
bool g_fMoveForm;

// Calculate the angle between 2 lines
float angle(Point pt1, Point pt2, Point pt0) {
	float dx1 = pt1.x - pt0.x;
	float dy1 = pt1.y - pt0.y;
	float dx2 = pt2.x - pt0.x;
	float dy2 = pt2.y - pt0.y;
	return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2
			+ dy2 * dy2) + 1e-10);
}

// Evaluate whether a point is close to the corner markers
bool isNear(const Point &p, int &iDist) {
	int iDistUL = sqrt(pow(p.x - g_ptUpperLeft.x, 2) + pow(p.y
			- g_ptUpperLeft.y, 2));
	int iDistLR = sqrt(pow(p.x - g_ptLowerRight.x, 2) + pow(p.y
			- g_ptLowerRight.y, 2));

	iDist = min(iDistUL, iDistLR);
	return (iDist < c_cornerRange);
}

// This function checks whether a point is close to one of the 4 sides of the screen.
Side checkBorder(const Point &p) {
	Side result = None;

	if (p.y < c_borderMargin)
		result = Top;
	else if (p.y > (g_maxRows - c_borderMargin))
		result = Bottom;
	else if (p.x < c_borderMargin)
		result = Left;
	else if (p.x > (g_maxCols - c_borderMargin))
		result = Right;
	return result;
}

// This function checks whether a contour (a set of points) touches
// multiple sides of the screen. If so, it uses some heuristics to
// decide whether the contour represents a form that is too close
// to the camera or whether the form is skewed.
void checkContour(Mat &mat, const vector<Point> &contour) {
	if (g_fMoveForm)
		return;
	// If there are 2 points in the contour, checks whether it touches
	// 2 opposing sides of the screen. If so, it may indicate that the
	// form is too close. If they touches 2 adjacent sides of the screen,
	// keep track of the sides they touch because they may indicate a skewed form.
	if (contour.size() == 2) {
		Side side1 = checkBorder(contour[0]);
		Side side2 = checkBorder(contour[1]);
		if (side1 != None && side2 != None) {
			long len = (int) arcLength(Mat(contour), false);

			if (len < c_borderThreshold)
				return;

			if ((side1 % 2) == (side2 % 2)) {
				g_nAcross += len;
				const Point* p = &contour[0];
				int n = contour.size();
				polylines(mat, &p, &n, 1, false, Scalar(0xa5, 0x2a, 0x2a), 2);
			} else {
				Side sideTopBottom = (side1 % 2 == 0) ? side1 : side2;
				Side sideLeftRight = (side1 % 2 == 1) ? side1 : side2;
				const Point* p = &contour[0];
				int n = contour.size();
				polylines(mat, &p, &n, 1, false, Scalar(0xa5, 0x2a, 0x2a), 2);
				if (sideTopBottom == Top) {
					if (sideLeftRight == Right) {
						g_nTopRight = max(g_nTopRight, len);
					} else {
						g_nTopLeft = max(g_nTopLeft, len);
					}
				} else {
					if (sideLeftRight == Right) {
						g_nBottomRight = max(g_nBottomRight, len);
					} else {
						g_nBottomLeft = max(g_nBottomLeft, len);
					}
				}
			}
		}
	// If there are 3 or 4 points in the contour, check wether they represents
	// a corner of the form cut off by the screen.
	} else if (contour.size() == 3 || contour.size() == 4) {
		int len = (int) arcLength(Mat(contour), false);
		if (len < c_borderThreshold)
			return;

		Side side1 = checkBorder(contour[0]);
		Side side2 = checkBorder(contour[contour.size() - 1]);
		if (side1 != None && side2 != None) {
			g_fMoveForm = true;

			const Point* p = &contour[0];
			int n = contour.size();
			polylines(mat, &p, &n, 1, false, Scalar(0xa5, 0x2a, 0x2a), 2);
		}
	}
}

// This function resets the score displayed by feedback. It must be called
// before each preview session to make sure the score is initialized to zero.
void Feedback::ResetScore()
{
	g_iScore = 0;
}

// Detect whether the largest rectangle of an image is well aligned
// with the corner markers on screen
int Feedback::DetectOutline(int idx, image_pool *pool, double thres1,
		double thres2) {
	Mat img = pool->getImage(idx), imgCanny;
	int result = 0;

	// Leave if there is no image
	if (img.empty()) {
		LOGE("Failed to get image");
		return result;
	}

	vector < Point > approx;
	vector < Point > maxRect;
	vector < vector<Point> > contours;
	vector < vector<Point> > borderContours;
	float maxContourArea = 50000;
	bool fOutlineDetected = false;
	int iScore = 0;

	// Initialize the number of rows (height) of the image
	if (g_maxRows == -1) {
		g_maxRows = img.rows;
		g_maxCols = img.cols;
		g_ptUpperLeft = Point((img.cols - c_templateWidth) / 2, (g_maxRows
				- c_templateHeight) / 2);
		g_ptLowerRight = Point(img.cols - g_ptUpperLeft.x, img.rows
				- g_ptUpperLeft.y);
	}

	g_nAcross = g_nTopLeft = g_nTopRight = g_nBottomLeft = g_nBottomRight = 0;
	g_fMoveForm = false;

	// Do Canny transformation on the image
	Canny(pool->getGrey(idx), imgCanny, thres1, thres2, 3);

	// Find all external contours of the image
	findContours(imgCanny, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	// Iterate through all detected contours
	for (size_t i = 0; i < contours.size(); ++i) {
		int nMatchCorners = 0;

		// Approximate the perimeter of the contours
		approxPolyDP(Mat(contours[i]), approx,
				arcLength(Mat(contours[i]), true) * 0.01, true);

		if (approx.size() <= 4) {
			checkContour(img, approx);
		}

		// Check whether the perimeter forms a quadrilateral
		// Note: absolute value of an area is used because
		// area may be positive or negative - in accordance with the
		// contour orientation
		if (approx.size() == 4 && isContourConvex(Mat(approx))) {
			float area = fabs(contourArea(Mat(approx)));

			// Skip if its area is less that the maximum area
			// that we have detected.
			if (area > maxContourArea) {
				float maxCosine = 0;
				int iDist;
				int iDistLow1 = 10000, iDistLow2 = 10000;

				// Find the maximum cosine of the angle between joint edges
				// and check if the quadrilateral touches the corner markers
				for (int j = 2; j < 6; ++j) {
					float cosine = fabs(angle(approx[j % 4], approx[j - 2],
							approx[j - 1]));
					maxCosine = MAX(maxCosine, cosine);
					if (isNear(approx[j % 4], iDist)) {
						++nMatchCorners;
					}
					if (iDist < iDistLow1) {
						iDistLow1 = iDist;
					} else if (iDist < iDistLow2) {
						iDistLow2 = iDist;
					}
				}

				// If cosines of all angles are small
				// (ie. all angles are between 65-115 degree),
				// this quadrilateral becomes the largest one
				// we have detected so far.
				if (maxCosine < 0.42) {
					maxRect = approx;
					maxContourArea = area;
					iDist = (iDistLow1 + iDistLow2) / 2;
					iScore = max(iScore, 100 - min(100, (int) sqrt((iDist
							- c_cornerRange) * 25.0)));
				}
			}
		}
	}

	if (iScore > 0) {
		g_iScore = (g_iScore + iScore) / 2;
	} else {
		g_iScore *= 0.95;
	}

	if (g_iScore >= 80) {
		fOutlineDetected = true;
		result = 1;
	}

	// Draw corner markers
	const Point* p = &maxRect[0];
	int n = (int) maxRect.size();
	float gradient = exp(-((float) (100 - g_iScore) / 100.0));
	Scalar guideColor = Scalar(255 - 255 * gradient, 220 * gradient, 0);
	polylines(img, &p, &n, 1, true, guideColor, 3, CV_AA);
	rectangle(img, g_ptUpperLeft, g_ptLowerRight, guideColor, 2, CV_AA);

	// Draw Status text
	const char *pszMsg = NULL;
	Scalar color = Scalar(255, 0, 0);
	if (fOutlineDetected) {
		pszMsg = c_pszCorrectPos;
		color = Scalar(0, 255, 0);
	} else {
		long cornerLen = g_nTopLeft + g_nTopRight + g_nBottomLeft
				+ g_nBottomRight;

		if (g_fMoveForm || g_nAcross > cornerLen) {
			pszMsg = c_pszMoveCloser;
		} else if (g_nTopLeft + g_nBottomRight > 0 && g_nTopRight
				+ g_nBottomLeft > 0) {
			if (g_nTopLeft + g_nBottomRight > g_nTopRight + g_nBottomLeft) {
				pszMsg = c_pszRotateLeft;
			} else {
				pszMsg = c_pszRotateRight;
			}
		}
	}
	if (pszMsg != NULL)
		drawText(idx, pool, pszMsg, -2, 0, color, 1.6);

	// Draw Score
	char pszTxt[10];
	drawText(idx, pool, c_pszScore, -6, 1, guideColor);
	sprintf(pszTxt, "%d%%", g_iScore);
	drawText(idx, pool, pszTxt, -1, 1, guideColor, 2.0);

	return result;
}

// Draw text on screen
void Feedback::drawText(int i, image_pool* pool, const char* ctext, int row,
		int hJust, const cv::Scalar &color, double fontScale, double thickness) {
	int fontFace = FONT_HERSHEY_COMPLEX_SMALL;
	Mat img = pool->getImage(i);
	string text = ctext;
	Size textSize = getTextSize(text, fontFace, fontScale, thickness, NULL);

	// Center the text
	int x;
	switch (hJust) {
	case -1:
		x = 10;
		break;
	case 0:
		x = (img.cols - textSize.width) / 2;
		break;
	case 1:
		x = img.cols - textSize.width - 10;
		break;
	}
	Point textOrg(x, (row > 0) ? (textSize.height * row) : (img.rows + (row
			* textSize.height)));

	// Draw a black border for the text and then draw the text
	putText(img, text, textOrg, fontFace, fontScale, Scalar::all(0), thickness
			* 10);
	putText(img, text, textOrg, fontFace, fontScale, color, thickness, CV_AA);
}
