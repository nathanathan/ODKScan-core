#include <string>
#include "./ImageProcessing.h"

using namespace std;

int main(int argc, char *argv[]) {
  train_PCA_classifier();
  string image("test1.jpg");
  string json("bubble-locations");
  ProcessImage(image, json);
}
