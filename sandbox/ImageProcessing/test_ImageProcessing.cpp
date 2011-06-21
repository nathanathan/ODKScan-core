#include "testSuite.h"
#include <iostream>
#include <fstream>
#include <string>
#include "ImageProcessing.h"

#include <iterator>

#define DEBUG 0

using namespace std;

void check_values(vector<vector<bubble_val> > &found, int *tpos, int *tneg, int *fpos, int *fneg);

//Maybe this should go in image processing?
//Although it is likey to change when we switch to JSON templates.
void writeBubbleVals(string filename, vector< vector<bubble_val> > bubble_vals) {
	ofstream outfile(filename.c_str(), ios::out | ios::binary);
	vector< vector<bubble_val> >::iterator it;
	ostream_iterator<int> oi(outfile, " ");
	for( it=bubble_vals.begin(); it < bubble_vals.end(); it++) {
		copy((*it).begin(), (*it).end(), oi);
		outfile << endl;
	}
	outfile.close();
}

int main(int argc, char *argv[]) {
	vector<string> include;
	vector<string> exclude;
	exclude.push_back("filled");
	train_PCA_classifier(include, exclude);

	// image to be processed
	string image("vr_simulated.jpg");
	//string image("booklet_form.jpg");

	// bubble location file
	string bubbles("bubble-locations.full2");
	float i;

	std::cout << "parameter_name, parameter_value, true_positives, ";
	std::cout << "false_positives, true_negatives, false_negatives" << std::endl;

	// testing loop, currently iterates over weight_param
	#if 1
	for (i = 0.5; i <= .5; i += 0.05) {
	#else
	for (i = 0.0; i <= 1.0; i += 0.05) {
	#endif
		// process the image with the given parameter value
		vector<vector<bubble_val> > bubble_vals = ProcessImage(image, bubbles, i);
		
		writeBubbleVals("output-vals-test", bubble_vals);
		
		// get true positive, false positive, true negative, and false negative data
		int tpos = 0, tneg = 0, fpos = 0, fneg = 0;
		check_values(bubble_vals, &tpos, &tneg, &fpos, &fneg);

		// print out results to console, >> filename to save to file
		std::cout << "\"weight parameter\", " << i << ", ";
		std::cout << tpos << ", " << fpos << ", ";
		std::cout << tneg << ", " << fneg << std::endl;
	}
}

void check_values(vector<vector<bubble_val> > &found, int *tpos, int *tneg, int *fpos, int *fneg) {
	vector<vector<int> > actual;
	string line;
	int bubble, i, j;

	// file with actual bubble values, format:
	//   val val val val val val\n
	//   val val val val val val\n
	// etc, example:
	//   0 0 0 0 0 1 0 0 0 0 0 0 0
	// reads from left to right within row, top to bottom within segment
	// one segment per line
	string valfile("bubble-vals");

	// read in bubble values
	ifstream bubble_value_file(valfile.c_str());
	if (bubble_value_file.is_open()) {
		while (getline(bubble_value_file, line)) {
			stringstream ss(line);

			vector<int> bubble_line;
			while (ss.good()) {
				ss >> bubble;
				bubble_line.push_back(bubble);
			}

			actual.push_back(bubble_line);
		}
	}

	for (i = 0; i < actual.size(); i++) {
		for (j = 0; j < actual[i].size(); j++) {
			// true positive
			if (found[i][j] == 1 && actual[i][j] == 1) {
				(*tpos)++;
			}
			// true negative
			else if (found[i][j] == 0 && actual[i][j] == 0) {
				(*tneg)++;
			}
			// false positive
			else if (found[i][j] == 1 && actual[i][j] == 0) {
				(*fpos)++;
			}
			// false negative
			else if (found[i][j] == 0 && actual[i][j] == 1) {
				(*fneg)++;
			}
		}
	}
}
