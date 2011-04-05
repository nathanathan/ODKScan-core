#pragma once

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/legacy/compat.hpp>
#include <list>

class Processor
{
public:
  Processor();
  virtual ~Processor();

  char* ProcessForm(char* filename);
  bool DetectOutline(char* filename, bool fIgnoreDatFile = false);
  bool DetectOutline(char* filename, bool fIgnoreDatFile, cv::Rect &r);
  void warpImage(IplImage* img, IplImage* warpImg, CvPoint * cornerPoints);
  std::vector<cv::Point> findBubbles(IplImage* pImage);
  CvPoint * findLineValues(IplImage* img);

private:
};

