/*
This program tests the end to end image processing pipeline on a single image.
In the future it will vary a parameter and record performance to a csv file.
*/

#include "Processor.h"
#include "TestTools.h"
#include "MarkupForm.h"

#include <iostream>
#include <fstream>
#include <string>

#ifndef EIGENBUBBLES
int EIGENBUBBLES = 7;
#endif

using namespace std;

//The reason to use JSON for the bubble-vals files is that other code, like java code can parse them
//and display the results without any hardcoding.
int main(int argc, char *argv[]) {

	MarkupForm marker;

	#if 1
	string imageDir("form_images/booklet_form/");
	string imageName("G11");
	#else
	string imageDir("form_images/using_box/");
	string imageName("C0");
	#endif
	
	string jsonTrueVals(imageDir + imageName.substr(0, imageName.find_first_of("0123456789")) + ".json");
	
	string imagePath = imageDir + imageName +".jpg";
	
	// Template to use:
	string templatePath("form_templates/SIS-A01");
	// Location to output results:
	string outputName("output/" + imageName);
	
	ofstream outfile("test_data.csv", ios::out | ios::binary);
	outfile << "parameter_name, parameter_value, true_positives, ";
	outfile << "false_positives, true_negatives, false_negatives" << endl;
	

	string jsonOutfile(outputName+"_vals.json");
	// Location to write the aligned form to:
	string alignedFormOutfile(outputName+"_straightened.jpg");
	string markedupFormOutfile(outputName+"_markedup.jpg");

	Processor myProcessor;

	// testing loop, currently iterates over weight_param
	#ifndef EIGENBUBBLES
	for (int i = 3; i <= 20; i ++) {
		EIGENBUBBLES = i;
		outfile << "eigenbubbles, " << i << ", ";
	#else
	for (int i = 0; i < 1; i ++) {
	#endif
		if( !myProcessor.loadForm(imagePath.c_str()) ) {
			cout << "Could not load. Arg: " << imagePath << endl;
			return 0;
		}
		if( !myProcessor.loadTemplate(templatePath.c_str()) ) {
			cout << "Could not load. Arg: " << templatePath << endl;
			return 0;
		}
		if( !myProcessor.alignForm(alignedFormOutfile.c_str()) ) {
			cout << "Could not align. Arg: " << alignedFormOutfile << endl;
			return 0;
		}
		myProcessor.trainClassifier("training_examples/android_training_examples");
		if( !myProcessor.processForm(jsonOutfile.c_str()) ) {
			cout << "Could not process. Arg: " << jsonOutfile << endl;
			return 0;
		}
		if( !marker.markupForm(jsonOutfile.c_str(), alignedFormOutfile.c_str(), markedupFormOutfile.c_str()) ) {
			cout << "Could not markup. Arg: " << markedupFormOutfile << endl;
		}
		
		int tp=0, fp=0, tn=0, fn=0;
		compareFiles(jsonOutfile, jsonTrueVals, tp, fp, tn, fn);
		printData(tp, fp, tn, fn);
		
		outfile << tp << ", " << fp << ", ";
		outfile << tn << ", " << fn << endl;
	}
	
	outfile.close();
}
