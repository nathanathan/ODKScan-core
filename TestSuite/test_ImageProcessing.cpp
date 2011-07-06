/*
Description of what's being tested
*/

#include <iostream>
#include <string>

#include "Processor.h"

using namespace std;

//The reason to use JSON for the bubble-vals files is that other code, like java code can parse them
//and display the results without any hardcoding.
int main(int argc, char *argv[]) {

	// Image to be processed:
	// TODO: This (or a variant of it) should eventually use FileUtils and a predicate to specify multiple images.
	string imagePath("form_images/unbounded_form/A0.jpg");
	// Template to use:
	string templatePath("form_templates/unbounded_form_shreddr_w_fields.json");
	// Location to output results:
	string outputPath("unbounded_form_A0_vals.json");

	Processor myProcessor(templatePath.c_str());
	myProcessor.trainClassifier();

	float i;
	// testing loop, currently iterates over weight_param
	#if 1
	for (i = 0.5; i <= .5; i += 0.05) {
	#else
	for (i = 0.0; i <= 1.0; i += 0.05) {
	#endif
		myProcessor.setClassifierWeight(i);
		myProcessor.loadForm(imagePath.c_str());
		myProcessor.alignForm();
		myProcessor.processForm(outputPath.c_str());
	}
}
