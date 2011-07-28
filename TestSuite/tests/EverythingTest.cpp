/*
This program tests the full images processing pipeline on all the images in a specified directory.
*/
#include "Processor.h"
#include "FileUtils.h"
#include "TestTools.h"
#include "MarkupForm.h"

#include <iostream>
#include <string>

using namespace std;

bool extraPred(const string& filename){
	return filename.find("G") != string::npos;
}

//The reason to use JSON for the bubble-vals files is that other code, like java code can parse them
//and display the results without any hardcoding.
int main(int argc, char *argv[]) {

	MarkupForm marker;

	int tp=0, fp=0, tn=0, fn=0;
	int errors = 0;
	int numImages = 0;

	string inputDir("form_images/booklet_form/");
	string outputDir("aligned_forms/booklet_form/");

	string templatePath("form_templates/SIS-A01.json");

	Processor myProcessor(templatePath.c_str());
	myProcessor.trainClassifier("training_examples/android_training_examples");

	vector<string> filenames;
	CrawlFileTree(inputDir, filenames);

	vector<string>::iterator it;
	for(it = filenames.begin(); it != filenames.end(); it++) {
		if(isImage((*it)) && extraPred((*it))){
			numImages++;
			string relativePathMinusExt((*it).substr((*it).find_first_of(inputDir) + inputDir.length(),
																		(*it).length()-inputDir.length()-4));
			string imgOutputPath(outputDir + relativePathMinusExt + ".jpg");
			string markedupFormOutfile(outputDir + relativePathMinusExt + "_marked.jpg");
			string jsonOutfile(outputDir + relativePathMinusExt + ".json");
			cout << "Processing image: " << (*it) << endl;
			if( !myProcessor.loadForm((*it).c_str()) ) {
				cout << "\E[31m" <<  "Could not load. Arg: " << "\e[0m" << (*it) << endl;
				errors++;
				continue;
			}
			cout << "Outputting aligned image to: " << imgOutputPath << endl;
			if( !myProcessor.alignForm(imgOutputPath.c_str()) ) {
				cout << "\E[31m" <<  "Could not align. Arg: " << "\e[0m" << imgOutputPath  << endl;
				errors++;
				continue;
			}
			if( !myProcessor.processForm(jsonOutfile.c_str()) ) {
				cout << "\E[31m" << "Could not process. Arg: " << "\e[0m" << jsonOutfile << endl;
				errors++;
				continue;
			}
			if( !marker.markupForm(jsonOutfile.c_str(), imgOutputPath.c_str(), markedupFormOutfile.c_str()) ) {
				cout << "\E[31m" <<  "Could not markup. Arg: " << "\e[0m" << markedupFormOutfile  << endl;
				errors++;
				continue;
			}
			
			cout << "\E[32m" << "Apparent success!" << "\e[0m" << endl;
			
			//This might be a bug if I have numbers in directory names.
			string expectedJsonFile(outputDir + 
									relativePathMinusExt.substr(0, relativePathMinusExt.find_first_of("0123456789")) +
									".json");
			
			//TODO: To compare quads I'll need a bubble val file for each image.
			/*
			string expectedJsonFile(outputDir +  relativePathMinusExt + ".json");
			*/			
			if( fileExists(expectedJsonFile) ) {
				compareFiles(jsonOutfile, expectedJsonFile, tp, fp, tn, fn);
			}
			else{
				cout << "Could not find: " << expectedJsonFile << endl;
			}
		}
	}
	
	printData(tp, fp, tn, fn, errors, numImages);
	
}
