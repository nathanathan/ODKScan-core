/*
This program tests the alignment step of the image processing pipeline on all the images in the designated directory.
*/
#include "Processor.h"
#include "FileUtils.h"
#include "TestTools.h"

#include <iostream>
#include <string>

using namespace std;

//The reason to use JSON for the bubble-vals files is that other code, like java code can parse them
//and display the results without any hardcoding.
int main(int argc, char *argv[]) {
	
	int errors = 0, numImages = 0;

	string inputDir("form_images/booklet_form");
	string outputDir("aligned_forms/booklet_form");

	string templatePath("form_templates/SIS-A01.json");

	Processor myProcessor(templatePath.c_str());

	vector<string> filenames;
	CrawlFileTree(inputDir, filenames);

	vector<string>::iterator it;
	for(it = filenames.begin(); it != filenames.end(); it++) {
		if(isImage((*it))){
			numImages++;
			string outputPath(outputDir + (*it).substr((*it).find_first_of(inputDir) + inputDir.length()));
			cout << "Processing form from: " << (*it) << endl << "to: " << outputPath << endl;
			if( !myProcessor.loadForm((*it).c_str()) ) {
				cout << "Could not load. Arg: " << (*it) << endl;
				return 0;
			}
			if( !myProcessor.alignForm(outputPath.c_str()) ) {
				cout << "\E[31m" << "Could not align. Arg: " << "\e[0m" << outputPath << endl;
				errors++;
				continue;
			}
			cout << "\E[32m" << "Apparent success!" << "\e[0m" << endl;
		}
	}
	cout << endl;
	printData(errors, numImages);
}
