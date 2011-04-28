/*
 * Accepts a form image and a form description. Draws the segmentation boxes
 * on top of the form.
 * 
 * run as:
 * ./segment_form image_filename
 *
 * Steve Geluso
 */

#include "cv.h"
#include "highgui.h"

using namespace std;
using namespace cv;

// Tests for arguments. Exits program
bool args_ok(char* argv[])
{
    if (argv[1] == NULL) {
        printf("Missing image argument. Should be: \n");
        printf("%s image_file\n", argv[0]);
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    // Ensure the proper arguments have been passed to the program.
    if (!args_ok(argv)) {
        return -1;
    }

    // Yoink the image filename passed as argument.
    char* image = argv[1];
    char* json = argv[2];

    // Initialize matrices that will contain images.
    Mat form, out;
    
    // Read the input image
	form = imread(image);
    int form_width = form.cols;
    int form_height = form.rows;

    // Report the dimensions of the image.
    printf("width: %d, height: %d\n", form_width, form_height);

	if (form.data == NULL) {
        printf("No image data. Exiting.\n");
		return false;
	}

	// Make window with given name.
    const char* name = "JSON Form Segmentation";
	namedWindow(name, CV_WINDOW_KEEPRATIO);

    // Draw rectangles on image.
    //           img, CvPoint, CvPoint, CvScalar color, int thickness, int type, int shift
    Point2f top_left = Point2f(0, 0);
    Point2f bottom_right = Point2f(2047, 1535);
    Scalar color = Scalar(0, 255, 0); // Green!

    // Define segments for this form, in px.
    int x = 216;
    int y;
    int width = 1346;
    int height = 160;

    // Draw a rectangle around the entire form.
    rectangle(form, Point2f(0, 0), Point2f(form_width, form_height), color, 1, 8, 0);

/*
    Cropping example.
	// Crop the image to focus on the area where the bubbles are.
	// This significantly speeds up processing time since there are
	// usually a lot of noise on other parts of the image.
	Size cropSize(c_rightMargin - c_leftMargin, img.rows);
	Point cropCenter(c_leftMargin + (c_rightMargin - c_leftMargin) / 2,
			img.rows / 2);
	getRectSubPix(img, cropSize, cropCenter, imgCropped);
*/

    // Draw a rectangle around each segment.
    y = 140;
    rectangle(form, Point2f(x, y), Point2f(x + width, y + height), color, 1, 8, 0);
    y = 345;
    rectangle(form, Point2f(x, y), Point2f(x + width, y + height), color, 1, 8, 0);
    y = 534;
    rectangle(form, Point2f(x, y), Point2f(x + width, y + height), color, 1, 8, 0);
    y = 735;
    rectangle(form, Point2f(x, y), Point2f(x + width, y + height), color, 1, 8, 0);
    y = 935;
    rectangle(form, Point2f(x, y), Point2f(x + width, y + height), color, 1, 8, 0);
    
    // Show image within the window.
    imshow(name, form);

    // Save the image of the segmented form.
    imwrite("segmented_form.jpg", form);

    // Run the application until the user presses ESC.
    while(true) {
        if(cvWaitKey(15) == 27) {
            break;
        }
    }

    // Destroy the window and exit gracefully.
	cvDestroyWindow(name);
	return 0;
}

