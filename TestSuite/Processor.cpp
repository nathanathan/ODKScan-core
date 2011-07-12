#include "configuration.h"
#ifdef USE_ANDROID_HEADERS_AND_IO
// Some of the headers probably aren't needed:
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/legacy/compat.hpp>
#include <sys/stat.h>
#include "log.h"
#define LOG_COMPONENT "Nathan"
//Note: LOGI doesn't yet transfer to the test suite
//		Can I make it work like cout and use defines to switch between them?
#else
#include "cv.h"
#include "highgui.h"
#endif

#include <json/json.h>

#include <iostream>
#include <fstream>

#include "Processor.h"
#include "PCA_classifier.h"
#include "FormAlignment.h"
#include "Addons.h"

#if 1
#define SCALEPARAM 1.0
#else
#define SCALEPARAM 0.5
#endif

// Creates a buffer around segments porpotional to their size.
// I think .5 is the largest value that won't cause ambiguous cases.
#define SEGMENT_BUFFER .5

#define DEBUG 1

#define OUTPUT_SEGMENT_IMAGES

#ifdef OUTPUT_SEGMENT_IMAGES
#include "NameGenerator.h"
NameGenerator namer("debug_segment_images/");
#endif

using namespace std;
using namespace cv;

class Processor::ProcessorImpl {

private:

Json::Value root;
Mat formImage;
PCA_classifier classifier;

public:

Json::Value classifyBubbles(Mat& segment, const Json::Value& bubbleLocations, Mat& transformation, const Point& offset){
	#ifdef OUTPUT_SEGMENT_IMAGES
	//TODO: This should become perminant functionality.
	//		My idea is to have Java code that displays segment images as the form is being processed.
	Mat dbg_out;
	cvtColor(segment, dbg_out, CV_GRAY2RGB);
	dbg_out.copyTo(dbg_out);
	#endif
	Json::Value bubblesJsonOut;
	for (size_t i = 0; i < bubbleLocations.size(); i++) {
		Point bubbleLocation = (2.0 / 1.1) * SCALEPARAM * jsonToPoint(bubbleLocations[i]);
		Point refined_location = classifier.bubble_align(segment, bubbleLocation);
		bubble_val current_bubble = classifier.classifyBubble(segment, refined_location);
		
		Mat actualLocation = transformation * Mat(Point3d(refined_location.x, refined_location.y, 1.));

		Json::Value bubble;
		bubble["value"] = Json::Value(current_bubble > NUM_BUBBLE_VALS/2);
		bubble["location"] = pointToJson(Point(actualLocation.at<double>(0,0),
											   actualLocation.at<double>(1,0)) + offset);
		bubblesJsonOut.append(bubble);
		
		#ifdef OUTPUT_SEGMENT_IMAGES
		Scalar color(0, 0, 0);
		switch(current_bubble) {
			case EMPTY_BUBBLE:
				color = Scalar(255, 255, 0);
				break;
			case PARTIAL_BUBBLE:
				color = Scalar(255, 0, 255);
				break;
			case FILLED_BUBBLE:
				color = Scalar(0, 255, 255);
				break;
		}
		rectangle(dbg_out, bubbleLocation-Point(classifier.exampleSize.width/2,classifier.exampleSize.height/2),
						   bubbleLocation+Point(classifier.exampleSize.width/2,classifier.exampleSize.height/2),
						   color);
		
		circle(dbg_out, refined_location, 1, Scalar(255, 2555, 255), -1);
		#endif
	}

	#ifdef OUTPUT_SEGMENT_IMAGES
	string segfilename = namer.get_unique_name("marked_");
	segfilename.append(".jpg");
	imwrite(segfilename, dbg_out);
	#endif
	return bubblesJsonOut;
}
Json::Value processSegment(const Json::Value &segmentTemplate){
	#if DEBUG > 0
	cout << "aligning segment" << endl;
	#endif
	Rect imageRect( SCALEPARAM * Point(segmentTemplate.get("x", INT_MIN ).asInt(),
									   segmentTemplate.get("y", INT_MIN).asInt()),
					SCALEPARAM * Size(segmentTemplate.get("width", INT_MIN).asInt(),
									  segmentTemplate.get("height", INT_MIN).asInt()));	
	
	Rect expandedRect = expandRect(imageRect, SEGMENT_BUFFER);
	
	//Ensure that the segment with the buffer added on doesn't go off the form
	//and that everything is positive.
	//Maybe add some code that will reduce the segment buffer if it goes over the edge.
	assert(expandedRect.tl().x >= 0 && expandedRect.tl().y >= 0);
	assert(expandedRect.br().x < formImage.cols && expandedRect.br().y < formImage.rows);

	Mat segment = formImage(expandedRect);
	
	vector<Point> quad = findBoundedRegionQuad(segment);
	
	Mat alignedSegment(0, 0, CV_8U);
	alignImage(segment, alignedSegment, quad, imageRect.size());
	
	Mat transformation = getMyTransform(quad, expandedRect.size(), imageRect.size(), true);
	
	Json::Value segmentJsonOut;
	segmentJsonOut["key"] = segmentTemplate.get("key", -1);
	segmentJsonOut["quad"] = quadToJsonArray(quad, expandedRect.tl());
	segmentJsonOut["bubbles"] = classifyBubbles(alignedSegment, segmentTemplate["bubble_locations"], transformation, expandedRect.tl());
	
	return segmentJsonOut;
}
bool parseJsonFromFile(const char* filePath, Json::Value& myRoot){
	#if DEBUG > 0
	cout << "Parsing JSON" << endl;
	#endif
	ifstream JSONin;
	Json::Reader reader;
	
	JSONin.open(filePath, ifstream::in);
	bool parse_successful = reader.parse( JSONin, myRoot );
	
	JSONin.close();
}
ProcessorImpl(const char* templatePath){

	parseJsonFromFile(templatePath, root);
	
	#if DEBUG > 0
	string form_name = root.get("name", "no_name").asString();
	cout << form_name << endl;
	#endif
}
bool trainClassifier(){
	#if DEBUG > 0
	cout << "training classifier...";
	#endif
	Json::Value defaultSize;
	defaultSize.append(5);
	defaultSize.append(8);
	Json::Value bubbleSize = root.get("bubble_size", defaultSize);
	if( !classifier.train_PCA_classifier(Size(SCALEPARAM * bubbleSize[0u].asInt(),
											  SCALEPARAM * bubbleSize[1u].asInt())) ){
		return false;
	}
	classifier.set_search_window(Size(0,0));
	#if DEBUG > 0
	cout << "trained" << endl;
	#endif
	return true;
}
void setClassifierWeight(float weight){
	classifier.set_weight(FILLED_BUBBLE, weight);
	classifier.set_weight(PARTIAL_BUBBLE, 1-weight);
	classifier.set_weight(EMPTY_BUBBLE, 1-weight);
}
bool loadForm(const char* imagePath){
	#if DEBUG > 0
	cout << "loading form image..." << endl;
	#endif
	formImage = imread(imagePath, 0);
	return formImage.data != NULL;
}
bool alignForm(const char* alignedImageOutputPath){
	Size form_sz(root.get("width", 0).asInt(), root.get("height", 0).asInt());
	if( form_sz.width <= 0 || form_sz.height <= 0){
		return false;
	}
	
	//TODO: figure out the best place to do this...
	#ifdef USE_ANDROID_HEADERS_AND_IO
	Mat temp;
    transpose(formImage, temp);
    flip(temp,formImage, 1); //This might vary by phone... Current setting is for Nexus.
    #endif
    
	#if DEBUG > 0
	cout << "straightening image" << endl;
	#endif
	
	//templDir will contain JSON templates, form images, and yml? feature data for each form.
	String templDir("");
	Mat straightenedImage(0,0, CV_8U);
	alignFormImage(formImage, straightenedImage, templDir+root.get("form_name", "default").asString(),
					SCALEPARAM * form_sz, .25);
					
	/*
	vector<Point> quad = findFormQuad(formImage);
	Mat straightenedImage(0,0, CV_8U);
	alignImage(formImage, straightenedImage, quad,  SCALEPARAM * form_sz);
	*/
	
	if(straightenedImage.data == NULL) {
		return false;
	}
	else{
		formImage = straightenedImage;
		return writeFormImage(alignedImageOutputPath);
	}
}

bool processForm(const char* outputPath) {
	#if DEBUG > 0
	cout << "debug level is: " << DEBUG << endl;
	#endif
	
	if( root == NULL || formImage.data == NULL || !classifier.trained()){
		cout << "Unable to process form" << endl;
		return false;
	}
	
	const Json::Value fields = root["fields"];
	Json::Value JsonOutputFields;
	
	for ( int i = 0; i < fields.size(); i++ ) {
		const Json::Value field = fields[i];
		const Json::Value segments = field["segments"];
		
		Json::Value fieldJsonOut;
		fieldJsonOut["label"] = field.get("label", "unlabeled");
		
		for ( int index = 0; index < segments.size(); index++ ) {
			const Json::Value segmentTemplate = segments[index];
			Json::Value segmentJsonOut = processSegment(segmentTemplate);
			fieldJsonOut["segments"].append(segmentJsonOut);
		}
		//TODO: Modify output to include form image filename (which should be kept as a record),
		//the template name and a "fields" key that everything in the current arry will fall under.
		JsonOutputFields.append(fieldJsonOut);
	}
	
	Json::Value JsonOutput;
	JsonOutput["form_type"] = root.get("form_type", "NA");
	JsonOutput["form_image_path"] = Json::Value("NA");
	JsonOutput["fields"] = JsonOutputFields;
	
	#if DEBUG > 0
	cout << "outputting bubble vals... " << endl;
	#endif
	ofstream outfile(outputPath, ios::out | ios::binary);
	outfile << JsonOutput;
	outfile.close();
	return true;
}
//Marks up formImage based on the specifications of a bubble-vals JSON file at the given path.
//Then output the form to the given locaiton
bool markupForm(const char* bvPath, const char* outputPath) {
	Json::Value bvRoot;
	Mat markupImage;
	cvtColor(formImage, markupImage, CV_GRAY2RGB);
	if( parseJsonFromFile(bvPath, bvRoot) ){
		const Json::Value fields = bvRoot["fields"];
		for ( size_t i = 0; i < fields.size(); i++ ) {
			const Json::Value field = fields[i];
			const Json::Value segments = field["segments"];
			for ( size_t j = 0; j < segments.size(); j++ ) {
				const Json::Value segment = segments[j];
				vector<Point> quad = jsonArrayToQuad(segment["quad"]);
				const Point* p = &quad[0];
				int n = (int) quad.size();
				polylines(markupImage, &p, &n, 1, true, 200, 2, CV_AA);
				const Json::Value bubbles = segment["bubbles"];
				for ( size_t k = 0; k < bubbles.size(); k++ ) {
					const Json::Value bubble = bubbles[k];
					Point bubbleLocation(jsonToPoint(bubble["location"]));
					//cout << bubble["location"][0u].asInt() << "," << bubble["location"][1u].asInt() << endl;
					Scalar color(0, 0, 0);
					if(bubble["value"].asBool()){
						color = Scalar(255, 0, 0);
					}
					else{
						color = Scalar(0, 0, 255);
					}
									   
					circle(markupImage, bubbleLocation, 1, color, -1);
				}
			}
			
		}
		return imwrite(outputPath, markupImage);
	}
	else{
		return false;
	}
}
//Writes the form image to a file.
bool writeFormImage(const char* outputPath){
	return imwrite(outputPath, formImage);
}

};


/* This stuff hooks the Processor class up to the implementation class: */
Processor::Processor(const char* templatePath) : processorImpl(new ProcessorImpl(templatePath)){}
bool Processor::trainClassifier(){
	return processorImpl->trainClassifier();
}
void Processor::setClassifierWeight(float weight){
	processorImpl->setClassifierWeight(weight);
}
bool Processor::loadForm(const char* imagePath){
	return processorImpl->loadForm(imagePath);
}
bool Processor::alignForm(const char* alignedImageOutputPath){
	return processorImpl->alignForm(alignedImageOutputPath);
}
bool Processor::processForm(const char* outPath){
	return processorImpl->processForm(outPath);
}
bool Processor::markupForm(const char* bvPath, const char* outputPath){
	return processorImpl->markupForm(bvPath, outputPath);
}
bool Processor::writeFormImage(const char* outputPath){
	return processorImpl->writeFormImage(outputPath);
}
