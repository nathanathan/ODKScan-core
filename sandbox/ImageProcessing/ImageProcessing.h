/*
 * Header file for image processing functions.
 */
#ifndef IMAGEPROCESSING_H
#define IMAGEPROCESSING_H

#include <string>
#include "PCA_classifier.h"

// takes a filename and processes the entire image for bubbles
vector< vector<bubble_val> > ProcessImage(std::string &imagefilename, std::string &bubblefilename);

// takes a filename and JSON spec and looks for bubbles according
// to locations coded in the JSON
// int ProcessImage(string image, JSON_OBJ json);

#endif
