/*
Description of what's being tested
*/

#include "Processor.h"
#include "TestTools.h"
#include "MarkupForm.h"

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

//The reason to use JSON for the bubble-vals files is that other code, like java code can parse them
//and display the results without any hardcoding.
int main(int argc, char *argv[]) {

	MarkupForm marker;

	string imageDir("form_images/unbounded_form/");
	// Image to be processed:
	string imageName("A0");
	string imagePath = imageDir + imageName +".jpg";
	//string imagePath("form_images/initial_form/A0.jpg");
	
	// Template to use:
	string templatePath("form_templates/unbounded_form_refined.json");
	// Location to output results:
	string outputName("output/" + imageName);
	
	ofstream outfile("test_data.csv", ios::out | ios::binary);
	outfile << "parameter_name, parameter_value, true_positives, ";
	outfile << "false_positives, true_negatives, false_negatives" << endl;
	

	string jsonOutfile(outputName+"_vals.json");
	// Location to write the aligned form to:
	string alignedFormOutfile(outputName+"_straightened.jpg");
	string markedupFormOutfile(outputName+"_markedup.jpg");

	Processor myProcessor(templatePath.c_str());
	myProcessor.trainClassifier("training_examples/android_training_examples");

	float i;
	// testing loop, currently iterates over weight_param
	#if 1
	for (i = 0.5; i <= .5; i += 0.05) {
	#else
	for (i = 0.0; i <= 1.0; i += 0.05) {
	#endif
		
		myProcessor.setClassifierWeight(i);
		if( !myProcessor.loadForm(imagePath.c_str()) ) {
			cout << "Could not load. Arg: " << imagePath << endl;
			return 0;
		}
		if( !myProcessor.alignForm(alignedFormOutfile.c_str()) ) {
			cout << "Could not align. Arg: " << alignedFormOutfile << endl;
			return 0;
		}
		if( !myProcessor.processForm(jsonOutfile.c_str()) ) {
			cout << "Could not process. Arg: " << jsonOutfile << endl;
			return 0;
		}
		if( !marker.markupForm(jsonOutfile.c_str(), alignedFormOutfile.c_str(), markedupFormOutfile.c_str()) ) {
			cout << "Could not markup. Arg: " << markedupFormOutfile << endl;
		}
		
		int tp=0, fp=0, tn=0, fn=0;
		compareFiles(jsonOutfile, jsonOutfile, tp, fp, tn, fn);
		printData(tp, fp, tn, fn);
		
		outfile << "\"weight parameter\", " << i << ", ";
		outfile << tp << ", " << fp << ", ";
		outfile << tn << ", " << fn << endl;
	}
	
	outfile.close();
}
