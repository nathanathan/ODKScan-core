/*
Description of what's being tested
*/

#include "Processor.h"
#include "TestTools.h"

#include <iostream>
#include <string>

using namespace std;

//The reason to use JSON for the bubble-vals files is that other code, like java code can parse them
//and display the results without any hardcoding.
int main(int argc, char *argv[]) {

	// Image to be processed:
	// TODO: This (or a variant of it) should eventually use FileUtils and a predicate to specify multiple images.
	string imagePath("form_images/unbounded_form/A0.jpg");
	//string imagePath("form_images/initial_form/A0.jpg");
	// Template to use:
	string templatePath("form_templates/unbounded_form_refined.json");
	// Location to output results:
	string outputName("output/unbounded_form_A0");
	
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
		if( !myProcessor.markupForm(jsonOutfile.c_str(), markedupFormOutfile.c_str()) ) {
			cout << "Could not markup. Arg: " << markedupFormOutfile << endl;
		}
		
		int tp=0, fp=0, tn=0, fn=0;
		compareFiles(jsonOutfile, jsonOutfile, tp, fp, tn, fn);
		cout << "True positives: "<< tp << endl;
		cout << "False positives: " << fp << endl;
		cout << "True negatives: "<< tn << endl;
		cout << "False negatives: " << fn << endl;
	}
}
