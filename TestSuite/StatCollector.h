#ifndef STATCOLLECTOR_H
#define STATCOLLECTOR_H
#include <string>
#include <json/json.h>
#include <iostream>

class StatCollector
{
int tp, fp, tn, fn;
int errors, numImages;
int missedSegments, numSegments;
std::vector<double> times;

private:
	void compareSegments(const Json::Value& foundSeg, const Json::Value& actualSeg);
	void compareFields(const Json::Value& foundField, const Json::Value& actualField);
public:
	StatCollector():
		tp(0), fp(0), tn(0), fn(0),
		errors(0), numImages(0),
		missedSegments(0), numSegments(0)
	{}
	
	StatCollector(  int tp, int fp, int tn, int fn,
					int errors, int numImages,
					int missedSegments, int numSegments, std::vector<double> times):
		tp(tp), fp(fp), tn(tn), fn(fn),
		errors(errors), numImages(numImages),
		missedSegments(missedSegments), numSegments(numSegments), times(times)
	{}
	
	void incrErrors(){ errors++; }
	void incrImages(){ numImages++; }
	void addTime(double t){ times.push_back(t); }
	void compareFiles(const std::string& foundPath, const std::string& actualPath);

	float formAlignmentRatio() const { return 1.f * (numImages - errors) / numImages; }
	float segmentAlignmentRatio() const { return 1.f * (numSegments - missedSegments) / numSegments; }
	float correctClassificationRatio() const { return 1.f * (tp + tn) / (tp+fp+tn+fn); }
	
	void print(std::ostream& myOut) const;
	
	StatCollector& operator+=(const StatCollector& sc){
		this->tp += sc.tp;
		this->fp += sc.fp;
		this->tn += sc.tn;
		this->fn += sc.fn;
		this->errors += sc.errors;
		this->numImages += sc.numImages;
		this->missedSegments += sc.missedSegments;
		this->numSegments += sc.numSegments;
		this->times.insert(this->times.end(), sc.times.begin(), sc.times.end());
		return *this;
	}

	StatCollector operator+(const StatCollector& sc) const {
		std::vector <double> newTimes;
		newTimes.insert(newTimes.end(), times.begin(), times.end());
		newTimes.insert(newTimes.end(), sc.times.begin(), sc.times.end());
		return StatCollector(
					tp + sc.tp,
					fp + sc.fp,
					tn + sc.tn,
					fn + sc.fn,
					errors + sc.errors,
					numImages + sc.numImages,
					missedSegments + sc.missedSegments,
					numSegments + sc.numSegments,
					newTimes );
	}
};

std::ostream& operator<<(std::ostream& os, const StatCollector& sc);

const std::string linebreak("________________________________________________________");

#endif
