/*
This program tests the full images processing pipeline on all the images in a specified directory.
*/
#include "Processor.h"
#include "FileUtils.h"
#include "StatCollector.h"
#include "MarkupForm.h"

#include <iostream>
#include <string>
#include <map>

using namespace std;

string getLabel(const string& filepath){
	int start = filepath.find_last_of("/") + 1;
	return	filepath.substr(start, filepath.find_last_of("_") - start);
}

//The reason to use JSON for the bubble-vals files is that other code, like java code can parse them
//and display the results without any hardcoding.
int main(int argc, char *argv[]) {

	//StatCollector statCollector;
	map<string, StatCollector> collectors;

	string experimentDir("experiment_1/");
	string inputDir("form_images/" + experimentDir);
	string outputDir("aligned_forms/" + experimentDir);
	string expectedJsonFile( "form_images/experiment.json" );

	string templatePath("form_templates/SIS-A01.json");

	vector<string> filenames;
	CrawlFileTree(inputDir, filenames);

	vector<string>::iterator it;
	for(it = filenames.begin(); it != filenames.end(); it++) {
		if( isImage((*it)) ){
		
			string label = getLabel(*it);
		
			collectors[label].incrImages();
			string relativePathMinusExt((*it).substr((*it).find_first_of(inputDir) + inputDir.length(),
																		(*it).length()-inputDir.length()-4));
			string imgOutputPath(outputDir + relativePathMinusExt + ".jpg");
			string markedupFormOutfile(outputDir + relativePathMinusExt + "_marked.jpg");
			string jsonOutfile(outputDir + relativePathMinusExt + ".json");
			
			cout << "Processing image: " << (*it) << endl;
			
			Processor myProcessor;
			
			if( !myProcessor.loadForm((*it).c_str()) ) {
				cout << "\E[31m" <<  "Could not load. Arg: " << "\e[0m" << (*it) << endl;
				collectors[label].incrErrors();
				continue;
			}
			if( !myProcessor.loadTemplate(templatePath.c_str()) ) {
				cout << "Could not load. Arg: " << templatePath << endl;
				continue;
			}
			
			myProcessor.trainClassifier("training_examples/android_training_examples");
			cout << "Outputting aligned image to: " << imgOutputPath << endl;
			
			if( !myProcessor.alignForm(imgOutputPath.c_str()) ) {
				cout << "\E[31m" <<  "Could not align. Arg: " << "\e[0m" << imgOutputPath  << endl;
				collectors[label].incrErrors();
				continue;
			}
			if( !myProcessor.processForm(jsonOutfile.c_str()) ) {
				cout << "\E[31m" << "Could not process. Arg: " << "\e[0m" << jsonOutfile << endl;
				collectors[label].incrErrors();
				continue;
			}
			if( !MarkupForm::markupForm(jsonOutfile.c_str(), imgOutputPath.c_str(), markedupFormOutfile.c_str()) ) {
				cout << "\E[31m" <<  "Could not markup. Arg: " << "\e[0m" << markedupFormOutfile  << endl;
				collectors[label].incrErrors();
				continue;
			}
			
			cout << "\E[32m" << "Apparent success!" << "\e[0m" << endl;
			
			if( fileExists(expectedJsonFile) ) {
				collectors[label].compareFiles(jsonOutfile, expectedJsonFile);
			}
			else{
				cout << "Could not find: " << expectedJsonFile << endl;
			}
		}
	}
	
	cout << "________________________________________________________" << endl;
	map<string, StatCollector>::iterator mapit;
	for ( mapit=collectors.begin() ; mapit != collectors.end(); mapit++ ){
    	cout << (*mapit).first << endl << (*mapit).second;
	}
	
}
