/*
Description of what's being tested
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

	string inputDir("form_images/");
	string outputDir("aligned_forms/");

	string templatePath("form_templates/unbounded_form_refined.json");

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
				errors++;
				continue;
			}
		}
	}
	cout << endl;
	printData(errors, numImages);
}
