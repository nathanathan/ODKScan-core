#include <iostream>
#include <fstream>
#include <string>
#include "./ImageProcessing.h"

#define DEBUG 0

using namespace std;

void check_values(vector<bubble_val> &found, int *tpos, int *tneg, int *fpos, int *fneg);

int main(int argc, char *argv[]) {
  train_PCA_classifier();
  string image("vr.jpg");
  string bubbles("bubble-locations.full2");
  float i;

  for (i = 0.0; i <= 0.00; i += 0.01) {
    vector<bubble_val> bubble_vals = ProcessImage(image, bubbles, i);

    int tpos = 0, tneg = 0, fpos = 0, fneg = 0;
    check_values(bubble_vals, &tpos, &tneg, &fpos, &fneg);
    std::cout << "parameter_name, parameter_value, true_positives, ";
    std::cout << "false_positives, true_negatives, false_negatives" << std::endl;
    std::cout << "\"weight parameter\", " << i << ", ";
    std::cout << tpos << ", " << fpos << ", ";
    std::cout << tneg << ", " << fneg << std::endl;
  }
}

void check_values(vector<bubble_val> &found, int *tpos, int *tneg, int *fpos, int *fneg) {
  vector<int> actual;
  string line;
  int bubble, i;

  string valfile("bubble-vals");

  ifstream bubble_value_file(valfile.c_str());
  if (bubble_value_file.is_open()) {
    while (getline(bubble_value_file, line)) {
      stringstream ss(line);

      while (ss.good()) {
        ss >> bubble;
        actual.push_back(bubble);
      }
    }
  }

  for (i = 0; i < actual.size(); i++) {
    if (found[i] == 1 && actual[i] == 1) {
      (*tpos)++;
      //std::cout << "true positive" << std::endl;
    } else if (found[i] == 0 && actual[i] == 0) {
      (*tneg)++;
      //std::cout << "true negative" << std::endl;
    } else if (found[i] == 1 && actual[i] == 0) {
      (*fpos)++;
      //std::cout << "false positive" << std::endl;
    } else if (found[i] == 0 && actual[i] == 1) {
      (*fneg)++;
      //std::cout << "false negative" << std::endl;
    }
  }
}
