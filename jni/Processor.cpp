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

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include "Processor.h"
#include "PCA_classifier.h"
#include "FormAlignment.h"

#if 0
#define SCALEPARAM 1.1
#else
#define SCALEPARAM 0.55
#endif

//I might not need this...
#define SCALE_TEMPLATE 2.0

// Creates a buffer around segments porpotional to their size.
// I think .5 is the largest value that won't cause ambiguous cases.
#define SEGMENT_BUFFER .5

#define DEBUG 0

//#define OUTPUT_SEGMENT_IMAGES

#ifdef OUTPUT_SEGMENT_IMAGES
#include "NameGenerator.h"
NameGenerator namer("debug_segment_images/");
#endif

using namespace std;
using namespace cv;

Json::Value Processor::classifySegment(const Json::Value &bubbleLocations, Mat &segment){
	Json::Value bubble_vals;
	#if DEBUG > 0
	cout << "finding bubbles" << endl;
	#endif
	
	#ifdef OUTPUT_SEGMENT_IMAGES
	//TODO: This should become perminant functionality.
	//		My idea is to have Java code that displays segment images as the form is being processed.
	Mat dbg_out;
	cvtColor(segment, dbg_out, CV_GRAY2RGB);
	dbg_out.copyTo(dbg_out);
	#endif

	for (size_t i = 0; i < bubbleLocations.size(); i++) {
		Point bubbleLocation(bubbleLocations[i][0u].asInt() * SCALE_TEMPLATE, bubbleLocations[i][1u].asInt() * SCALE_TEMPLATE);
		Point refined_location = classifier.bubble_align(segment, bubbleLocation);
		//cout << refined_location.x << ", " << refined_location.y << endl;
		bubble_val current_bubble = classifier.classifyBubble(segment, refined_location);
		bubble_vals.append(Json::Value(current_bubble > NUM_BUBBLE_VALS/2));
		
		#ifdef OUTPUT_SEGMENT_IMAGES
		Scalar color(0, 0, 0);
		switch(current_bubble) {
			case EMPTY_BUBBLE:
				color = Scalar(255, 0, 0);
				break;
			case PARTIAL_BUBBLE:
				color = Scalar(255, 0, 255);
				break;
			case FILLED_BUBBLE:
				color = Scalar(0, 0, 255);
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
	return bubble_vals;
}
void Processor::getAlignedSegment(const Json::Value &segmentTemplate, Mat &alignedSegment) {

	#if DEBUG > 0
	cout << "aligning segment" << endl;
	#endif
	Point seg_loc(segmentTemplate.get("x", -1).asInt() * SCALE_TEMPLATE,
				  segmentTemplate.get("y", -1).asInt() * SCALE_TEMPLATE);
	int seg_width = segmentTemplate.get("width", -1).asInt() * SCALE_TEMPLATE;
	int seg_height = segmentTemplate.get("height", -1).asInt() * SCALE_TEMPLATE;

	//Ensure that the segment with the buffer added on doesn't go off the form
	//and that everything is positive.
	assert(seg_width > 0 && seg_height > 0);
	assert(seg_loc.x - SEGMENT_BUFFER * seg_width >= 0 && seg_loc.y - SEGMENT_BUFFER * seg_height >= 0);
	assert(seg_loc.x + seg_width * (1 + SEGMENT_BUFFER) <= formImage.cols / SCALEPARAM &&
			seg_loc.y + seg_height *  (1 + SEGMENT_BUFFER) <= formImage.rows / SCALEPARAM);
			
	//TODO: Add a conditional that will reduce the segment buffer if it goes over the edge.
	//		And use a Size type for seg width/height.
	Mat segment_img = formImage(Rect(SCALEPARAM * (seg_loc - SEGMENT_BUFFER * Point(seg_width, seg_height)),
										Size(SCALEPARAM * seg_width * (SEGMENT_BUFFER * 2 + 1),
											 SCALEPARAM * seg_height * (SEGMENT_BUFFER * 2 + 1))));
	alignedSegment = Mat(0, 0, CV_8U);
	align_image(segment_img, alignedSegment, Size(SCALEPARAM * seg_width, SCALEPARAM *  seg_height));
}

Processor::Processor(const char* templatePath){
	#if DEBUG > 0
	cout << "Parsing JSON" << endl;
	#endif
	ifstream JSONin;
	Json::Reader reader;
	
	JSONin.open(templatePath, ifstream::in);
	bool parse_successful = reader.parse( JSONin, root );
	
	JSONin.close();
	
	#if DEBUG > 0
	string form_name = root.get("name", "no_name").asString();
	cout << form_name << endl;
	#endif
}
Processor::~Processor() {
}
//TODO: Make it so this and load form and align form return meaningful bools for success
bool Processor::trainClassifier(){
	Json::Value defaultSize;
	defaultSize.append(5);
	defaultSize.append(8);
	Json::Value bubbleSize = root.get("bubble_size", defaultSize);
	classifier.train_PCA_classifier(Size(bubbleSize[0u].asInt(),
										 bubbleSize[1u].asInt()));
	return true;
}
void Processor::setClassifierWeight(float weight){
	classifier.set_weight(FILLED_BUBBLE, weight);
	classifier.set_weight(PARTIAL_BUBBLE, 1-weight);
	classifier.set_weight(EMPTY_BUBBLE, 1-weight);
}
bool Processor::loadForm(const char* imagePath){
	// Read the input image
	formImage = imread(imagePath, 0);
	return formImage.data != NULL;
}
bool Processor::alignForm(const char* alignedImageOutputPath){
	int form_width = root.get("width", 0).asInt() * SCALE_TEMPLATE;
	int form_height = root.get("height", 0).asInt() * SCALE_TEMPLATE;
	if( form_width <= 0 || form_height <= 0){
		//LOGI("Couldn't read template");
		return false;
	}
	
	#if DEBUG > 0
	cout << "straightening image" << endl;
	#endif
	Mat straightenedImage(0,0, CV_8U);
	
	//TODO: figure out the best place to do this...
	Mat temp;
    transpose(formImage, temp);
    flip(temp,formImage, 1); //This might vary by phone... Current setting is for Nexus.
    if(form_width == 0 || form_height == 0){
    	return false;
    }
    
	align_image(formImage, straightenedImage, Size(form_width * SCALEPARAM, form_height * SCALEPARAM), 12);
	formImage = straightenedImage;
	
	if(straightenedImage.data == NULL){
		return false;
	}
	
	return imwrite(alignedImageOutputPath, formImage );
}

bool Processor::processForm(const char* outputPath) {
	#if DEBUG > 0
	cout << "debug level is: " << DEBUG << endl;
	#endif
	
	Json::Value JsonOutput;
	
	const Json::Value fields = root["fields"];
	
	for ( int i = 0; i < fields.size(); i++ ) {
		const Json::Value field = fields[i];
		Json::Value fieldJsonOut;
		
		fieldJsonOut["label"] = field.get("label", "unlabeled");

		const Json::Value segments = field["segments"];
		
		for ( int index = 0; index < segments.size(); index++ ) {
			const Json::Value segmentTemplate = segments[index];
			Json::Value segmentJsonOut;
			segmentJsonOut["key"] = segmentTemplate.get("key", -1);
		
			Json::Value bubbleLocations = segmentTemplate["bubble_locations"];

			Mat alignedSegment;
			getAlignedSegment(segmentTemplate, alignedSegment);
			segmentJsonOut["bubble_vals"] = classifySegment(bubbleLocations, alignedSegment);
			fieldJsonOut["segments"].append(segmentJsonOut);
		}
		JsonOutput.append(fieldJsonOut);
	}
	
	#if DEBUG > 0
	cout << "outputting bubble vals... " << endl;
	#endif
	ofstream outfile(outputPath, ios::out | ios::binary);
	outfile << JsonOutput;
	outfile.close();
	return true;
}
