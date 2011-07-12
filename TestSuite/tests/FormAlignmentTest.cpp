/*
Description of what's being tested
*/

#include <iostream>
#include <string>

#include "Processor.h"
#include "FileUtils.h"

using namespace std;

bool isImage(const string& filename){
	return filename.find(".jpg") != string::npos;
}
//The reason to use JSON for the bubble-vals files is that other code, like java code can parse them
//and display the results without any hardcoding.
int main(int argc, char *argv[]) {

	string inputDir("form_images/");
	string outputDir("aligned_forms/");

	string templatePath("form_templates/unbounded_form_shreddr_w_fields.json");
	

	Processor myProcessor(templatePath.c_str());

	vector<string> filenames;
	CrawlFileTree(inputDir, filenames);

	vector<string>::iterator it;
	for(it = filenames.begin(); it != filenames.end(); it++) {
		if(isImage((*it))){
			string outputPath(outputDir + (*it).substr((*it).find_first_of(inputDir) + inputDir.length()));
			cout << "Processing form from: " << (*it) << endl << "to: " << outputPath << endl;
			if( !myProcessor.loadForm((*it).c_str()) ) {
				cout << "Could not load. Arg: " << (*it) << endl;
				return 0;
			}
			if( !myProcessor.alignForm(outputPath.c_str()) ) {
				cout << "Could not align. Arg: " << outputPath  << endl;
				return 0;
			}
		}
	}
}
