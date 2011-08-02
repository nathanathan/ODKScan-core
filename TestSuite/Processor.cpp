#include "configuration.h"
#include "Processor.h"
#include "FileUtils.h"
#include "PCA_classifier.h"
#include "FormAlignment.h"
#include "Addons.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/legacy/compat.hpp>

#include <json/json.h>

#include <iostream>
#include <fstream>

// This sets the resolution of the form at which to perform segment alignment and classification.
// It is a percentage of the size specified in the template.
#define SCALEPARAM 1.0

// Creates a buffer around segments porpotional to their size.
// I think .5 is the largest value that won't cause ambiguous cases.
#define SEGMENT_BUFFER .25

//#define DO_BUBBLE_INFERENCE

#ifdef OUTPUT_SEGMENT_IMAGES
#include "NameGenerator.h"
NameGenerator namer("debug_segment_images/");
#endif

using namespace std;
using namespace cv;

class Processor::ProcessorImpl {

private:

Json::Value root;
Json::Value JsonOutput;
PCA_classifier classifier;
string templPath;
Mat formImage;

Json::Value classifyBubbles(const Mat& segment, const Json::Value& bubbleLocations, const Mat& transformation, const Point& offset){
	#ifdef OUTPUT_SEGMENT_IMAGES
	//TODO: This should become perminant functionality.
	//		My idea is to have Java code that displays segment images as the form is being processed.
	Mat dbg_out;
	cvtColor(segment, dbg_out, CV_GRAY2RGB);
	dbg_out.copyTo(dbg_out);
	#endif
	
	Json::Value bubblesJsonOut;
	
	for (size_t i = 0; i < bubbleLocations.size(); i++) {
		Point bubbleLocation = SCALEPARAM * jsonToPoint(bubbleLocations[i]);
		Point refined_location = classifier.bubble_align(segment, bubbleLocation);
		
		bool bubbleVal = classifier.classifyBubble(segment, refined_location);
		
		Mat actualLocation = transformation * Mat(Point3d(refined_location.x, refined_location.y, 1.f));
		
		Json::Value bubble;
		bubble["value"] = Json::Value( bubbleVal );
		//I'm thinking I should compute the transformations in the markup function and
		//just record relative point locations here.
		//On the other hand that makes the output data less portable.
		//(1.f/SCALEPARAM)* this is probably a bad idea (unless I shrink the output aligned image...)
		bubble["location"] = pointToJson((Point(actualLocation.at<double>(0,0) / actualLocation.at<double>(2, 0),
											   actualLocation.at<double>(1,0) / actualLocation.at<double>(2, 0)) + offset));
		bubblesJsonOut.append(bubble);
		
		#ifdef OUTPUT_SEGMENT_IMAGES
		rectangle(dbg_out, bubbleLocation-Point(classifier.exampleSize.width / 2, classifier.exampleSize.height/2),
						   bubbleLocation+Point(classifier.exampleSize.width / 2, classifier.exampleSize.height/2),
						   getColor(bubbleVal));
		
		circle(dbg_out, refined_location, 1, Scalar(255, 2555, 255), -1);
		#endif
	}
	#ifdef OUTPUT_SEGMENT_IMAGES
	//TODO: name by field and segment# instead.
	string segfilename = namer.get_unique_name("marked_");
	segfilename.append(".jpg");
	imwrite(segfilename, dbg_out);
	#endif
	return bubblesJsonOut;
}
Json::Value processSegment(const Json::Value &segmentTemplate){

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

	vector<Point> quad = findBoundedRegionQuad(segment, SEGMENT_BUFFER);
	//TODO: come up with some way for segment alignment to fail.
				
	if(quad.size() == 4){
		#ifdef DEBUG_PROCESSOR
		//This makes a stream of dots so we can see how fast things are going.
		//we get a ! when things go wrong.
		cout << '.' << flush;
		#endif
		Mat transformation = quadToTransformation(quad, imageRect.size());
		Mat alignedSegment(0, 0, CV_8U);
		warpPerspective(segment, alignedSegment, transformation, imageRect.size());

		Json::Value segmentJsonOut;
		segmentJsonOut["quad"] = quadToJsonArray(quad, expandedRect.tl());
		segmentJsonOut["bubbles"] = classifyBubbles(alignedSegment, segmentTemplate["bubble_locations"],
													transformation.inv(), expandedRect.tl());
													
		return segmentJsonOut;
	}
	else{
		#ifdef DEBUG_PROCESSOR
		cout << "!" << flush;
		#endif
		Json::Value segmentJsonOut;
		segmentJsonOut["quad"] = quadToJsonArray(quad, expandedRect.tl());
		return segmentJsonOut;
	}
}

public:
//Constructor:
//TODO: rewrite this so it is a separate function that can return an error if it fails to find a template.
ProcessorImpl(const char* templatePath){
	templPath = string(templatePath);
	if(templPath.find(".") != string::npos){
		templPath = templPath.substr(0, templPath.find_last_of("."));
	}
	cout << templPath << endl;
	parseJsonFromFile(templPath + ".json", root);
	JsonOutput["template_path"] = templPath;
}
bool trainClassifier(const char* trainingImageDir){
	#ifdef DEBUG_PROCESSOR
	cout << "training classifier..." << endl;
	#endif
	const Json::Value defaultSize = pointToJson(Point(8,12));
	Json::Value bubbleSize = root.get("bubble_size", defaultSize);
	
	vector<string> filepaths;
	CrawlFileTree(string(trainingImageDir), filepaths);
	bool success = classifier.train_PCA_classifier( filepaths, SCALEPARAM * Size(bubbleSize[0u].asInt(),
																				 bubbleSize[1u].asInt()),
													7, true);//flip training examples.
	if (!success) return false;
	
	classifier.set_search_window(Size(5,5));
	JsonOutput["training_image_directory"] = Json::Value( trainingImageDir );
	#ifdef DEBUG_PROCESSOR
	cout << "trained" << endl;
	#endif
	return true;
}
void setClassifierWeight(float weight){
	/*
	classifier.set_weight(FILLED_BUBBLE, weight);
	classifier.set_weight(PARTIAL_BUBBLE, 1-weight);
	classifier.set_weight(EMPTY_BUBBLE, 1-weight);
	*/
	return;
}

bool loadForm(const char* imagePath, int rotate90){
	#ifdef DEBUG_PROCESSOR
	cout << "loading form image..." << flush;
	#endif
	formImage = imread(imagePath, 0);
	if(formImage.empty()) return false;
	
	//When using feature based alignment the rotate 90 thing might be unnecessairy.
	if(rotate90 != 0){
		Mat temp;
		transpose(formImage, temp);
		flip(temp,formImage, rotate90); //This might vary by phone... 1 is for Nexus.
	}
	JsonOutput["image_path"] = imagePath;
	#ifdef DEBUG_PROCESSOR
	cout << "loaded" << endl;
	#endif
	return true;
}
bool alignForm(const char* alignedImageOutputPath){

	#ifdef DEBUG_PROCESSOR
	cout << "aligning form..." << endl;
	#endif
	
	Size form_sz(root.get("width", 0).asInt(), root.get("height", 0).asInt());

	if( form_sz.width <= 0 || form_sz.height <= 0) return false;
	
	//TODO: For scaling just resize the picture so that it is aprox 20% more area than the template image.
	Mat straightenedImage;
	if( !alignFormImage( formImage, straightenedImage,
									templPath,
									SCALEPARAM * form_sz, .5) ) return false;
	
	if(straightenedImage.empty()) return false;

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
		
		for ( size_t j = 0; j < segments.size(); j++ ) {
			const Json::Value segmentTemplate = segments[j];
			Json::Value segmentJsonOut = processSegment(segmentTemplate);
			segmentJsonOut["key"] = segmentTemplate.get("key", -1);
			fieldJsonOut["segments"].append(segmentJsonOut);
		}
		fieldJsonOut["key"] = field.get("key", -1);
		
		#ifdef DO_BUBBLE_INFERENCE
		inferBubbles(fieldJsonOut);
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
bool writeFormImage(const char* outputPath){
	//string path(outputPath);
	//TODO: Made this create directory paths if one does not already exist
	//see http://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux
	return imwrite(outputPath, formImage);
}

};


/* This stuff hooks the Processor class up to the implementation class: */
Processor::Processor(const char* templatePath) : processorImpl(new ProcessorImpl(templatePath)){}
bool Processor::trainClassifier(const char* trainingImageDir){
	return processorImpl->trainClassifier(trainingImageDir);
}
void Processor::setClassifierWeight(float weight){
	processorImpl->setClassifierWeight(weight);
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
