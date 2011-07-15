#ifndef TESTTOOLS_H
#define TESTTOOLS_H
#include <string>
void compareFiles(const std::string& foundPath, const std::string& actualPath, int& tp, int& fp, int& tn, int& fn);

#endif
