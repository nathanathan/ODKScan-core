#include <string>
#include "./ImageProcessing.h"

using namespace std;

int main(int argc, char *argv[]) {
  train_PCA_classifier();
  string image("vr3.jpg");
  string json("bubble-locations");
  vector<bubble_val> vr3 = ProcessImage(image, json);
}
