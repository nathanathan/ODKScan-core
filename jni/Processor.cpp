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

#define OUTPUT_SEGMENT_IMAGES

#ifdef OUTPUT_SEGMENT_IMAGES
#include "NameGenerator.h"
NameGenerator namer("debug_segment_images/");
#endif

using namespace std;
using namespace cv;

class Processor::ProcessorImpl {
Json::Value root;
Mat formImage;
PCA_classifier classifier;
public:
Json::Value classifySegment(const Json::Value &bubbleLocations, Mat &segment, Mat &transformation, Point offset){
	Json::Value bubbles;
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
		//Maybe make a define for bubble alignment. It can be controlled by shrinking the search window as well though...
		Point refined_location = classifier.bubble_align(segment, bubbleLocation);
		bubble_val current_bubble = classifier.classifyBubble(segment, refined_location);
		
		Json::Value bubble;
		Json::Value location;
		
		Mat actualLocation = transformation * Mat(Point3d(refined_location.x, refined_location.y, 0.));
		//TODO: add a Json::Value constructor for points.
		location.append((int)actualLocation.at<double>(0,0) + offset.x);
		location.append((int)actualLocation.at<double>(1,0) + offset.y);
		
		bubble["value"] = Json::Value(current_bubble > NUM_BUBBLE_VALS/2);
		bubble["location"] = Json::Value(location);
		bubbles.append(bubble);
		
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
	return bubbles;
}
Json::Value quadToJsonArray(vector<Point>& quad, Point& offset){
	Json::Value out;
	for(size_t i = 0; i < quad.size(); i++){
		Json::Value JsonPoint;
		Point myPoint = offset + quad[i];
		JsonPoint.append(myPoint.x);
		JsonPoint.append(myPoint.y);
		out.append(JsonPoint);
	}
	return out;
}
vector<Point> jsonArrayToQuad(const Json::Value& quadJson){
	vector<Point> out;
	for(size_t i = 0; i < quadJson.size(); i++){
		out.push_back(Point(quadJson[i][0u].asInt(), quadJson[i][1u].asInt()));
	}
	return out;
}
//TODO: this really needs to be cleaned up
Json::Value getAlignedSegment(const Json::Value &segmentTemplate, Mat &alignedSegment, Mat& transformation, Point& actualLoc) {

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
			
	actualLoc = SCALEPARAM * (seg_loc - SEGMENT_BUFFER * Point(seg_width, seg_height));
	//TODO: Add a conditional that will reduce the segment buffer if it goes over the edge.
	//		And use a Size type for seg width/height.
	Mat segment_img = formImage(Rect(actualLoc,
										Size(SCALEPARAM * seg_width * (SEGMENT_BUFFER * 2 + 1),
											 SCALEPARAM * seg_height * (SEGMENT_BUFFER * 2 + 1))));
	
	vector<Point> quad = findBoundedRegionQuad(segment_img);
	alignedSegment = Mat(0, 0, CV_8U);
	
	Size outImageSize(SCALEPARAM * seg_width, SCALEPARAM *  seg_height);
	alignImage(segment_img, alignedSegment, quad, outImageSize);
	
	transformation = getMyTransform(quad, segment_img.size(), outImageSize, true);
	return quadToJsonArray(quad, actualLoc);
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
~ProcessorImpl() {
}
bool trainClassifier(){
	Json::Value defaultSize;
	defaultSize.append(5);
	defaultSize.append(8);
	Json::Value bubbleSize = root.get("bubble_size", defaultSize);
	return classifier.train_PCA_classifier(Size(bubbleSize[0u].asInt(),
												bubbleSize[1u].asInt()));
}
void setClassifierWeight(float weight){
	classifier.set_weight(FILLED_BUBBLE, weight);
	classifier.set_weight(PARTIAL_BUBBLE, 1-weight);
	classifier.set_weight(EMPTY_BUBBLE, 1-weight);
}
bool loadForm(const char* imagePath){
	// Read the input image
	formImage = imread(imagePath, 0);
	return formImage.data != NULL;
}
bool alignForm(const char* alignedImageOutputPath){
	int form_width = root.get("width", 0).asInt() * SCALE_TEMPLATE;
	int form_height = root.get("height", 0).asInt() * SCALE_TEMPLATE;
	if( form_width <= 0 || form_height <= 0){
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
	vector<Point> quad = findFormQuad(formImage);
	Mat straightenedImage(0,0, CV_8U);
	alignImage(formImage, straightenedImage, quad, Size(form_width * SCALEPARAM, form_height * SCALEPARAM));
	
	if(straightenedImage.data == NULL){
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
	
	if( root == NULL ){ // || !formImage || !classifier){
		cout << "Unable to process form" << endl;
		return false;
	}
	
	Json::Value JsonOutput;
	
	const Json::Value fields = root["fields"];
	
	for ( int i = 0; i < fields.size(); i++ ) {
		const Json::Value field = fields[i];
		const Json::Value segments = field["segments"];
		
		Json::Value fieldJsonOut;
		fieldJsonOut["label"] = field.get("label", "unlabeled");
		
		for ( int index = 0; index < segments.size(); index++ ) {
			const Json::Value segmentTemplate = segments[index];
			
			Json::Value bubbleLocations = segmentTemplate["bubble_locations"];

			Mat alignedSegment, transformation;
			//Maybe the following should be grouped into a function?
			Json::Value segmentJsonOut;
			segmentJsonOut["key"] = segmentTemplate.get("key", -1);
			Point offset;
			segmentJsonOut["quad"] = getAlignedSegment(segmentTemplate, alignedSegment, transformation, offset);
			segmentJsonOut["bubbles"] = classifySegment(bubbleLocations, alignedSegment, transformation, offset);
														//Point(segmentTemplate["x"].asInt(), segmentTemplate["y"].asInt()));
			fieldJsonOut["segments"].append(segmentJsonOut);
		}
		//TODO: Modify output to include form image filename (which should be kept as a record),
		//the template name and a "fields" key that everything in the current arry will fall under.
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
//Marks up formImage based on the specifications of a bubble-vals JSON file at the given path.
//Then output the form to the given locaiton
bool markupForm(const char* bvPath, const char* outputPath) {
	Json::Value bvRoot;
	Mat markupImage;
	cvtColor(formImage, markupImage, CV_GRAY2RGB);
	if( parseJsonFromFile(bvPath, bvRoot) ){
		for ( size_t i = 0; i < bvRoot.size(); i++ ) {
			const Json::Value field = bvRoot[i];
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
					Point bubbleLocation(bubble["location"][0u].asInt(), bubble["location"][1u].asInt());
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

//Hook the processor class up to the implementation:
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
