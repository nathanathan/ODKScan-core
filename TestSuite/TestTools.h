#ifndef TESTTOOLS_H
#define TESTTOOLS_H
#include <string>
void compareFiles(const std::string& foundPath, const std::string& actualPath, int& tp, int& fp, int& tn, int& fn);
bool isImage(const std::string& filename);
void printData(int tp, int fp, int tn, int fn, int errors = 0, int numImages = 0);
#endif
