/*
 * Accepts a form image and a form description. Draws the segmentation boxes
 * on top of the form.
 * 
 * run as:
 * ./json_form image_filename json_filename
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
    if (argv[1] == NULL || argv[2] == NULL) {
        printf("Missing arguments. Should be: \n");
        printf("%s image_file json_file\n", argv[0]);
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

    // Report the dimensions of the image.
    printf("width: %d, height: %d\n", form.cols, form.rows);

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
    rectangle(form, top_left, bottom_right, color, 1, 8, 0);
    
    // Show image within the window.
    imshow(name, form);

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

