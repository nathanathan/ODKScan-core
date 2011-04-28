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

// Initialize matrices that will contain images.
Mat form, out;

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

// Accepts a description of a form segment, crops the image to the segment
// and saves the cropped segment to a new file.
void crop_and_draw_rect(int x, int y, int width, int height, string segment_name) {
    // Crop the different segments of the form.
    Mat cropped_image;
	Size size(width, height);
	Point center((x + (x + width)) / 2, (y + (y + height)) / 2);
	getRectSubPix(form, size, center, cropped_image);

    // Save the new crop to a file with the given name.
    imwrite(segment_name + ".jpg", cropped_image);
    
    // Draw a rectangle around the segment in the original image.
    Scalar color = Scalar(0, 255, 0); // Green!
    rectangle(form, Point2f(x, y), Point2f(x + width, y + height), color, 1, 8, 0);
}

int main(int argc, char* argv[])
{
    // Ensure the proper arguments have been passed to the program.
    if (!args_ok(argv)) {
        return -1;
    }

    // Yoink the image filename passed as argument.
    char* image = argv[1];
    char* description = argv[2];

    
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
    const char* name = "Form Segmentation";
	namedWindow(name, CV_WINDOW_KEEPRATIO);

    // Define segments for this form, in px.
    int x = 216;
    int y;
    int width = 1346;
    int height = 160;

    // Draw a rectangle around each segment.
    crop_and_draw_rect(x, 140, width, height, "bcg");
    crop_and_draw_rect(x, 345, width, height, "polio");
    crop_and_draw_rect(x, 534, width, height, "measles");
    crop_and_draw_rect(x, 735, width, height, "hepatitis_b_1");
    crop_and_draw_rect(x, 935, width, height, "hepatitis_b_2");
    
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


