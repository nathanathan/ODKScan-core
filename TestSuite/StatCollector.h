#ifndef STATCOLLECTOR_H
#define STATCOLLECTOR_H
#include <string>
#include <json/json.h>

bool isImage(const std::string& filename);

class StatCollector
{
int tp, fp, tn, fn;
int errors, numImages;
int missedSegments, numSegments;

private:
	void compareSegments(const Json::Value& foundSeg, const Json::Value& actualSeg);
	void compareFields(const Json::Value& foundField, const Json::Value& actualField);
public:
	StatCollector():
	tp(0), fp(0), tn(0), fn(0),
	errors(0), numImages(0),
	missedSegments(0), numSegments(0)
	{}
	void compareFiles(const std::string& foundPath, const std::string& actualPath);
	void printData();
	void incrErrors(){ errors++; }
	void incrImages(){ numImages++; }
};
#endif
