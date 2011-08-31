#include "configuration.h"
#include "Processor.h"
#include "FileUtils.h"
#include "PCA_classifier.h"
#include "Aligner.h"
#include "SegmentAligner.h"
#include "AlignmentUtils.h"
#include "Addons.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <json/json.h>

#include <iostream>
#include <fstream>

// This sets the resolution of the form at which to perform segment alignment and classification.
// It is a percentage of the size specified in the template.
#define SCALEPARAM 1.0

// Creates a buffer around segments porpotional to their size.
// I think .5 is the largest value that won't cause ambiguous cases.
#define SEGMENT_BUFFER .25

#define REFINE_ALL_BUBBLE_LOCATIONS true

//#define DO_BUBBLE_INFERENCE

#ifdef OUTPUT_SEGMENT_IMAGES
#include "NameGenerator.h"
NameGenerator namer;
#endif

using namespace std;
using namespace cv;

Json::Value genBubblesJson( const vector<bool>& bubbleVals, const vector<Point>& bubbleLocations, 
							const Point& offset,
							const Mat& transformation = (Mat_<double>(3,3) << 1, 0, 0, 0, 1, 0, 0, 0, 1)){
							
	assert(bubbleLocations.size() == bubbleVals.size());
	Json::Value out;
	for(size_t i = 0; i < bubbleVals.size(); i++){
		Json::Value bubble;
		bubble["value"] = Json::Value( bubbleVals[i] );
		Mat actualLocation = transformation * Mat(Point3d(bubbleLocations[i].x, bubbleLocations[i].y, 1.f));
		bubble["location"] = pointToJson(Point(actualLocation.at<double>(0,0) / actualLocation.at<double>(2, 0),
											   actualLocation.at<double>(1,0) / actualLocation.at<double>(2, 0)) + offset);
		out.append(bubble);
	}
	return out;
}

class Processor::ProcessorImpl {

private:

Json::Value root;
Json::Value JsonOutput;
PCA_classifier classifier;
string templPath;
Mat formImage;

vector<bool> classifyBubbles(const Mat& segment, const vector<Point> bubbleLocations) {
	vector<bool> out;
	for (size_t i = 0; i < bubbleLocations.size(); i++) {
		out.push_back( classifier.classifyBubble(segment, bubbleLocations[i]) );
	}
	return out;
}

vector<Point> getBubbleLocations(const Mat& segment, const Json::Value& bubbleLocationsJSON, bool refine) {

	vector <Point> bubbleLocations;
	vector<Point2f> points1, points2;
	
	if(!refine){
		for (size_t i = 0; i < bubbleLocationsJSON.size(); i++) {
			bubbleLocations.push_back( SCALEPARAM * jsonToPoint(bubbleLocationsJSON[i]) );
		}
		return bubbleLocations;
	}
	if(REFINE_ALL_BUBBLE_LOCATIONS || bubbleLocationsJSON.size() < 25){
		for (size_t i = 0; i < bubbleLocationsJSON.size(); i++) {
			bubbleLocations.push_back( classifier.bubble_align(segment, SCALEPARAM * jsonToPoint(bubbleLocationsJSON[i]) ) );
		}
		return bubbleLocations;
	}
	
	for (size_t i = 0; i < bubbleLocationsJSON.size(); i++) {
		bubbleLocations.push_back( SCALEPARAM * jsonToPoint(bubbleLocationsJSON[i]) );
		if(i % 3 == 2) {
			Point refinedLocation = classifier.bubble_align(segment, bubbleLocations.back());
			points1.push_back(Point2f(bubbleLocations.back().x, bubbleLocations.back().y));
			points2.push_back(Point2f(refinedLocation.x, refinedLocation.y));
		}
	}
	
	Mat H = findHomography( Mat(points1), Mat(points2), CV_RANSAC); //CV_LMEDS is suprisingly bad
	
	for (size_t i = 0; i < bubbleLocations.size(); i++) {
		Mat actualLocation = H * Mat(Point3d(bubbleLocations[i].x, bubbleLocations[i].y, 1.f));
		bubbleLocations[i] = Point( actualLocation.at<double>(0,0) / actualLocation.at<double>(2, 0),
									actualLocation.at<double>(1,0) / actualLocation.at<double>(2, 0));
	}
	return bubbleLocations;
}
Json::Value processSegment(const Json::Value &segmentTemplate) {
	
	Json::Value segmentJsonOut;
	Mat segmentImg;
	vector <Point> segBubbleLocs;
	vector <bool> bubbleVals;

	Rect segmentRect( SCALEPARAM * Point(segmentTemplate.get("x", INT_MIN ).asInt(),
									   segmentTemplate.get("y", INT_MIN).asInt()),
						SCALEPARAM * Size(segmentTemplate.get("width", INT_MIN).asInt(),
									  segmentTemplate.get("height", INT_MIN).asInt()));

	if(!segmentTemplate.get("bounded", true).asBool()){ //If segments don't have a bounding box
		segmentImg = formImage(segmentRect);
		segBubbleLocs = getBubbleLocations(segmentImg, segmentTemplate["bubble_locations"], false);
		bubbleVals = classifyBubbles( segmentImg, segBubbleLocs );
		segmentJsonOut["quad"] = quadToJsonArray(rectToQuad( segmentRect ));
		segmentJsonOut["bubbles"] = genBubblesJson( bubbleVals, segBubbleLocs, segmentRect.tl() );
	}
	else{
		Rect expandedRect = resizeRect(segmentRect, 1 + SEGMENT_BUFFER);
	
		//Reduce the segment buffer if it goes over the image edge.
		//TODO: see if you can do this with the rectangle intersection method.
		if(expandedRect.x < 0){
			expandedRect.x = 0;
		}
		if(expandedRect.y < 0){
			expandedRect.y = 0;
		}
		if(expandedRect.br().x > formImage.cols){
			expandedRect.width = formImage.cols - expandedRect.x;
		}
		if(expandedRect.br().y > formImage.rows){
			expandedRect.height = formImage.rows - expandedRect.y;
		}

		segmentImg = formImage(expandedRect);

		vector<Point> quad = findSegment(segmentImg, segmentRect - expandedRect.tl());

		if(testQuad(quad, segmentRect, .15)){
			#ifdef DEBUG_PROCESSOR
				//This makes a stream of dots so we can see how fast things are going.
				//we get a ! when things go wrong.
				cout << '.' << flush;
			#endif
			Mat transformation = quadToTransformation(quad, segmentRect.size());
			Mat alignedSegment(0, 0, CV_8U);
			warpPerspective(segmentImg, alignedSegment, transformation, segmentRect.size());
			segmentImg = alignedSegment;
			
			segBubbleLocs = getBubbleLocations( segmentImg, segmentTemplate["bubble_locations"], true);
			bubbleVals = classifyBubbles( segmentImg, segBubbleLocs );
			
			segmentJsonOut["quad"] = quadToJsonArray(quad, expandedRect.tl());
			segmentJsonOut["bubbles"] = genBubblesJson( bubbleVals, segBubbleLocs, 
														expandedRect.tl(), transformation.inv());
		}
		else{
			#ifdef DEBUG_PROCESSOR
				cout << "!" << flush;
			#endif
			segmentJsonOut["quad"] = quadToJsonArray(quad, expandedRect.tl());
			segmentJsonOut["notFound"] = true;
			return segmentJsonOut;
		}
	}
	
	#ifdef OUTPUT_SEGMENT_IMAGES
		//TODO: Maybe add Java code that displays segment images as the form is being processed.
		Mat dbg_out;
		cvtColor(segmentImg, dbg_out, CV_GRAY2RGB);
		
		vector <Point> expectedBubbleLocs = getBubbleLocations( segmentImg, segmentTemplate["bubble_locations"], false);
		for(size_t i = 0; i < expectedBubbleLocs.size(); i++){
			rectangle(dbg_out, expectedBubbleLocs[i] - Point(classifier.exampleSize.width / 2, classifier.exampleSize.height/2),
							   expectedBubbleLocs[i] + Point(classifier.exampleSize.width / 2, classifier.exampleSize.height/2),
							   getColor(bubbleVals[i]));
							   
			circle(dbg_out, segBubbleLocs[i], 1, Scalar(255, 2555, 255), -1);
		}
		//TODO: name by field and segment# instead.
		string segfilename = namer.get_unique_name("marked_");
		imwrite("debug_segment_images/" + segfilename + ".jpg", dbg_out);		
	#endif
	
	return segmentJsonOut;
}
bool setTemplate(const char* templatePath) {
	
	templPath = string(templatePath);
	//We don't need a file extension so this removes it if one is provided.
	if(templPath.find(".") != string::npos){
		templPath = templPath.substr(0, templPath.find_last_of("."));
	}
	if(!parseJsonFromFile(templPath + ".json", root)) return false;
	JsonOutput["template_path"] = templPath;
	return true;
	
}

public:

bool trainClassifier(const char* trainingImageDir) {
	#ifdef DEBUG_PROCESSOR
	cout << "training classifier..." << endl;
	#endif
	
	if (!root.isMember("bubble_size")) return false;
	
	Json::Value bubbleSize = root["bubble_size"];
	Size requiredExampleSize = SCALEPARAM * Size(bubbleSize[0u].asInt(),
												 bubbleSize[1u].asInt());
	//TODO: trainingImageDir should be specified in the template.
	//		perhaps multiple dirs will be specified if the form needs multiple classifiers...
	JsonOutput["training_image_directory"] = Json::Value( trainingImageDir );
	
	string cachedDataPath = string(trainingImageDir) + "/cached_classifier_data.yml";
	
	bool createClassifier = true;
	if( fileExists(cachedDataPath) ) {
		createClassifier = !classifier.load(cachedDataPath, requiredExampleSize);
	}
	if ( createClassifier ) {
		vector<string> filepaths;
		CrawlFileTree(string(trainingImageDir), filepaths);
		bool success = classifier.train_PCA_classifier( filepaths, requiredExampleSize,
														EIGENBUBBLES, true);//flip training examples.
		if (!success) return false;
		classifier.save(cachedDataPath);
	}
	
	classifier.set_search_window(Size(5,5));
	
	#ifdef DEBUG_PROCESSOR
	cout << "trained" << endl;
	#endif
	return true;
}
bool loadForm(const char* imagePath, int rotate90) {
	#ifdef DEBUG_PROCESSOR
		cout << "loading form image..." << flush;
	#endif
	formImage = imread(imagePath, 0);
	if(formImage.empty()) return false;
	
	/*
	//When using feature based alignment the rotate 90 thing is unnecessairy.
	//Apparently I do get slightly different performance when the image is rotated90
	if(rotate90 != 0){
		Mat temp;
		transpose(formImage, temp);
		flip(temp,formImage, int(rotate90 > 0)); //This might vary by phone... 1 is for Nexus.
	}
	*/
	//Automatically rotate90 if the image is wider than it is tall
	if(formImage.cols > formImage.rows){
		Mat temp;
		transpose(formImage, temp);
		flip(temp,formImage, 1);
	}
	
	JsonOutput["image_path"] = imagePath;
	#ifdef DEBUG_PROCESSOR
		cout << "loaded" << endl;
	#endif
	return true;
}
bool alignForm(const char* alignedImageOutputPath) {//and detect template

	#ifdef DEBUG_PROCESSOR
	cout << "aligning form..." << endl;
	#endif
	
	vector <string> templates;
	templates.push_back("form_templates/SIS-A01");
	templates.push_back("form_templates/UW_course_eval_A_front");
	
	Mat straightenedImage;
	try{
		Aligner aligner;
		cout << "Detecting form..." << endl;
		for(size_t i = 0; i < templates.size(); i++){
			aligner.loadFeatureData(templates[i]);
		}
		
		aligner.setImage(formImage);
		size_t formIdx = aligner.detectForm();
		cout << "\tDetected: " << templates[formIdx] << "\t\tIndex: " << formIdx << endl;
		setTemplate(templates[formIdx].c_str());
		
		Size form_sz(root.get("width", 0).asInt(), root.get("height", 0).asInt());
		if( form_sz.width <= 0 || form_sz.height <= 0) return false;
		
		aligner.alignFormImage( straightenedImage, SCALEPARAM * form_sz, formIdx );
	}
	catch(cv::Exception& e){
		LOGI(e.what());
		return false;
	}
	
	if(straightenedImage.empty()) {
		cout << "does this ever happen?" << endl;
		return false;
	}

	formImage = straightenedImage;	
	JsonOutput["aligned_image_path"] = alignedImageOutputPath;
	
	#ifdef DEBUG_PROCESSOR
		cout << "aligned" << endl;
	#endif
	return writeFormImage(alignedImageOutputPath);
}
bool processForm(const char* outputPath) {
	#ifdef  DEBUG_PROCESSOR
		cout << "Processing form" << flush;
	#endif
	
	if( !root || formImage.empty() || !classifier.trained()){
		cout << "Unable to process form. Error code: " <<
				(int)!root << (int)formImage.empty() << (int)!classifier.trained() << endl;
		return false;
	}
	
	const Json::Value fields = root["fields"];
	Json::Value JsonOutputFields;
	
	for ( size_t i = 0; i < fields.size(); i++ ) {
		const Json::Value field = fields[i];
		const Json::Value segments = field["segments"];
		
		Json::Value fieldJsonOut;
		fieldJsonOut["label"] = field.get("label", "unlabeled");
		
		if(field.get("mask", false).asBool()) continue;
		
		for ( size_t j = 0; j < segments.size(); j++ ) {
			const Json::Value segmentTemplate = segments[j];
			#ifdef OUTPUT_SEGMENT_IMAGES
				namer.setPrefix(field.get("label", "unlabeled").asString() + "_" + namer.intToString(j) + "_");	
			#endif
			Json::Value segmentJsonOut = processSegment(segmentTemplate);
			segmentJsonOut["key"] = segmentTemplate.get("key", -1);
			fieldJsonOut["segments"].append(segmentJsonOut);
		}
		fieldJsonOut["key"] = field.get("key", -1);
		
		#ifdef DO_BUBBLE_INFERENCE
			inferBubbles(fieldJsonOut, INFER_LTR_TTB);
		#endif
		
		JsonOutputFields.append(fieldJsonOut);
	}
	JsonOutput["form_scale"] = Json::Value(SCALEPARAM);
	JsonOutput["fields"] = JsonOutputFields;
	#ifdef  DEBUG_PROCESSOR
		cout << "done" << endl;
	#endif
	
	#ifdef  DEBUG_PROCESSOR
		cout << "outputting bubble vals... " << endl;
	#endif
	ofstream outfile(outputPath, ios::out | ios::binary);
	outfile << JsonOutput;
	outfile.close();
	return true;
}
//Writes the form image to a file.
bool writeFormImage(const char* outputPath) {
	//string path(outputPath);
	//TODO: Make this create directory paths if one does not already exist
	//		maybe make a function in addons to do that so it can be used for debug images as well.
	//see http://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux
	return imwrite(outputPath, formImage);
}

};


/* This stuff hooks the Processor class up to the implementation class: */
Processor::Processor() : processorImpl(new ProcessorImpl){ LOGI("Testing to see if C++ code works..."); }
bool Processor::loadTemplate(const char* templatePath){
	return true;
}
bool Processor::trainClassifier(const char* trainingImageDir){
	return processorImpl->trainClassifier(trainingImageDir);
}
bool Processor::loadForm(const char* imagePath, int rotate90){
	return processorImpl->loadForm(imagePath, rotate90);
}
bool Processor::alignForm(const char* alignedImageOutputPath){
	return processorImpl->alignForm(alignedImageOutputPath);
}
bool Processor::processForm(const char* outPath){
	return processorImpl->processForm(outPath);
}
bool Processor::writeFormImage(const char* outputPath){
	return processorImpl->writeFormImage(outputPath);
}
