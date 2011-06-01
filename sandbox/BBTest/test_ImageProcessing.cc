#include <iostream>
#include <fstream>
#include <string>
#include "./ImageProcessing.h"

using namespace std;

void check_values(vector<bubble_val> &found, int *tpos, int *tneg, int *fpos, int *fneg);

int main(int argc, char *argv[]) {
  train_PCA_classifier();
  string image("vr1.jpg");
  string json("bubble-locations");
  float i;

  for (i = 0.0; i <= 1.0; i += 0.01) {
    vector<bubble_val> vr1_vals = ProcessImage(image, json, i);

    int tpos = 0, tneg = 0, fpos = 0, fneg = 0;
    check_values(vr1_vals, &tpos, &tneg, &fpos, &fneg);
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
