#include "Processor.h"
#include "StatCollector.h"
#include "MarkupForm.h"

#include <iostream>
#include <string>
#include <time.h>

using namespace std;

int main(int argc, char *argv[]) {

	clock_t init, final;	
	
	string inputDir("form_images/course_eval");
	string outputDir("aligned_forms/course_eval");
	string imgName("A_1");
	string imgPath(inputDir + "/" + imgName + ".jpg");

	string templatePath("form_templates/UW_course_eval_A_front.json");

	string imgOutputPath(outputDir +  "/" + imgName + ".jpg");
	string markedupFormOutfile(outputDir +  "/" + imgName + "_marked.jpg");
	string jsonOutfile(outputDir +  "/" + imgName + ".json");
	
	cout << "Processing image: " << imgPath << endl;
	
	init = clock();
	
	Processor myProcessor;
	
	if( !myProcessor.loadForm(imgPath.c_str())) {
		cout << "\E[31m" <<  "Could not load. Arg: " << "\e[0m" << imgPath << endl;
		return 0;
	}
	if( !myProcessor.loadTemplate(templatePath.c_str()) ) {
		cout << "Could not load. Arg: " << templatePath << endl;
		return 0;
	}
	
	cout << "Outputting aligned image to: " << imgOutputPath << endl;
	
	if( !myProcessor.alignForm(imgOutputPath.c_str()) ) {
		cout << "\E[31m" <<  "Could not align. Arg: " << "\e[0m" << imgOutputPath  << endl;
		return 0;
	}
	myProcessor.trainClassifier("training_examples/android_training_examples");
	if( !myProcessor.processForm(jsonOutfile.c_str()) ) {
		cout << "\E[31m" << "Could not process. Arg: " << "\e[0m" << jsonOutfile << endl;
		return 0;
	}
	if( !MarkupForm::markupForm(jsonOutfile.c_str(), imgOutputPath.c_str(), markedupFormOutfile.c_str()) ) {
		cout << "\E[31m" <<  "Could not markup. Arg: " << "\e[0m" << markedupFormOutfile  << endl;
		return 0;
	}
	
	final=clock()-init;
	cout << (double)final / ((double)CLOCKS_PER_SEC) << endl;
	cout << "\E[32m" << "Apparent success!" << "\e[0m" << endl;

}
