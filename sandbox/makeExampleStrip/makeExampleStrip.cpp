#include "cv.h"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include "highgui.h"
#include "FileUtils.h"

using namespace std;
using namespace cv;

#define EXAMPLE_WIDTH 26
#define EXAMPLE_HEIGHT 32

bool make_example_strip(vector<string> example_filenames, Mat& example_strip){
    Mat example_image;
    example_strip.create(EXAMPLE_HEIGHT, example_filenames.size() * EXAMPLE_WIDTH, CV_8UC3);
   
    for(size_t i = 0; i < example_filenames.size(); i+=1) {
        cout << example_filenames[i] << "\n";
        example_image = imread(example_filenames[i]);
        if (example_image.data == NULL) {
            return false;
        }
        Mat dstroi = example_strip(Rect(i * EXAMPLE_WIDTH, 0, EXAMPLE_WIDTH, EXAMPLE_HEIGHT));
        resize(example_image, dstroi, Size(EXAMPLE_WIDTH, EXAMPLE_HEIGHT));
    }
    return true;
}

bool make_PCA_set(vector<string> example_filenames, Mat& PCA_set){
    Mat example_image;
    //PCA_set.create(1, example_filenames.size() * EXAMPLE_WIDTH * EXAMPLE_HEIGHT, CV_8UC1);
    PCA_set.zeros(1, example_filenames.size() * EXAMPLE_WIDTH * EXAMPLE_HEIGHT, CV_8UC1);
   
    for(size_t i = 0; i < example_filenames.size(); i+=1) {
        Mat PCA_set_row;
        example_image = imread(example_filenames[i]);
        if (example_image.data == NULL) {
            return false;
        }
        cvtColor(example_image, example_image, CV_RGB2GRAY);
        resize(example_image,  PCA_set_row, Size(EXAMPLE_WIDTH * EXAMPLE_HEIGHT, 1));
   //PCA_set_row.convertTo(PCA_set_row, CV_8UC3);
   PCA_set.row(i) += PCA_set_row.reshape(0,1);
    }
    return true;
}

int main(int argc, char* argv[])
{

    vector<string> file_names_v;
    string training_examples("training_examples");

    CrawlFileTree(training_examples.c_str(), file_names_v);
    Mat example_strip;
   
    if(!make_example_strip(file_names_v, example_strip)){
        cout << "crash" << "\n";
        return 0;
    }
   
    imwrite("example_strip.jpg", example_strip);
   
    return 0;
}
