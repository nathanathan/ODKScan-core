/*
 * Accepts a form image and a form description. Draws the segmentation boxes
 * on top of the form.
 *
 * Steve Geluso
 */

#include "cv.h"
#include "highgui.h"

using namespace std;
using namespace cv;

int main(int argc, char* argv[])
{
    // Yoink the image filename passed as argument.
    char* image = argv[1];
    char* json = argv[2];

    printf("image: %s\n", image);
    printf("json : %s\n", json);

    // Initialize matrices that will contain images.
    Mat img, out;
    
    // Read the input image
	img = imread(image);
	if (img.data == NULL) {
        printf("No image data. Exiting.\n");
		return false;
	}

	// Make window with given name.
    const char* name = "JSON Form Segmentation";
	namedWindow(name, CV_WINDOW_NORMAL);

    // Show image within the window.
    imshow(name, img);

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
