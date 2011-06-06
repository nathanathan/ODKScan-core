#include "cv.h"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include "highgui.h"
#include "FileUtils.h"

using namespace std;
using namespace cv;

#define EXAMPLE_WIDTH 14
#define EXAMPLE_HEIGHT 18

bool make_example_strip(vector<string> example_filenames, Mat& example_strip){
	Mat example_image;
	example_strip.create(EXAMPLE_HEIGHT, example_filenames.size() * EXAMPLE_WIDTH, CV_8UC3);
	
	for(size_t i = 0; i < example_filenames.size(); i+=1) {
		cout << example_filenames[i] << "\n";
		example_image = imread(example_filenames[i]);
		if (example_strip.data == NULL) {
			return false;
		}
		Mat dstroi = example_strip(Rect(i * EXAMPLE_WIDTH, 0, EXAMPLE_WIDTH, EXAMPLE_HEIGHT));
		resize(example_image, dstroi, Size(EXAMPLE_WIDTH, EXAMPLE_HEIGHT));
	}
	return true;
}

int main(int argc, char* argv[])
{
/*
	string file_names[] = {
	"VR_segment1.jpg", "VR_segment2.jpg", "VR_segment3.jpg", "VR_segment4.jpg", "VR_segment5.jpg", "VR_segment6.jpg"
	};
	vector<string> file_names_v (file_names, file_names + sizeof(file_names) / sizeof(string) );*/
	vector<string> file_names_v;
	string training_examples("./training_examples");

	CrawlFileTree(training_examples.c_str(), file_names_v);
	Mat example_strip;
	
	if(!make_example_strip(file_names_v, example_strip)){
		cout << "crash" << "\n";
		return 0;
	}
	//FileStorage fs(test.yml, FileStorage::WRITE);
	//fs<<"example_strip.jpg"<<
	
	imwrite("example_strip.jpg", example_strip);
	
	return 0;
}
