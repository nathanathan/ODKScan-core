/*
Header file for image processing functions.
*/

#ifndef IMAGEPROCESSING_H
#define IMAGEPROCESSING_H

#include <string>

class Processor{
	public:
		Processor(std::string &templatePath);
		virtual ~Processor();
		
		//These two probably don't need to be exposed except for in the testing suite
		bool trainClassifier();
		void setClassifierWeight(float weight);
		
		bool loadForm(std::string &imagePath);
		//Maybe instead of exposing alignForm I should just expose a method
		//for finding and displaying the contour.
		bool alignForm();
		bool processForm(std::string &outPath);
	private:
};


#endif
