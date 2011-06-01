#include <string>
#include "./ImageProcessing.h"

using namespace std;

int main(int argc, char *argv[]) {
  train_PCA_classifier();
  string image("vr1.jpg");
  string json("bubble-locations");
  int tpos, tneg, fpos, fneg;

  vector<bubble_val> vr1_vals = ProcessImage(image, json);

  image = "vr2.jpg";
  vector<bubble_val> vr2_vals = ProcessImage(image, json);

  image = "vr3.jpg";
  vector<bubble_val> vr3_vals = ProcessImage(image, json);
}

void check_values(vector<bubble_val> &found, int *tpos, int *tneg, int *fpos, int *fneg) {
  vector<bubble_val> actual;
  string line;
  int bubble;

  ifstream bubble_value_file();
  if (bubble_value_file.is_open()) {
    while (getline(bubble_value_file, line)) {
      stringstream ss(line);

      while (ss >> bubble) {
        actual.push_back(bubble);
      }
    }
  }
}
