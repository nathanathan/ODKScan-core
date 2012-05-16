#include "configuration.h"
#include "Processor.h"
#include "FileUtils.h"
#include "PCA_classifier.h"
#include "Aligner.h"
#include "SegmentAligner.h"
#include "AlignmentUtils.h"
#include "Addons.h"
#include "TemplateProcessor.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <json/json.h>

#include <iostream>
#include <fstream>
#include <map>
// This sets the resolution of the form at which to perform segment alignment and classification.
// It is a percentage of the size specified in the template.
#define SCALEPARAM 1.0

// Creates a buffer around segments porpotional to their size.
// I think .5 is the largest value that won't cause ambiguous cases.
#define SEGMENT_BUFFER .25

#ifdef OUTPUT_BUBBLE_IMAGES
	#include "NameGenerator.h"
	NameGenerator namer;
#endif

//#define TIME_IT

#define REFINE_ALL_BUBBLE_LOCATIONS true

using namespace std;
using namespace cv;

//TODO: Make this function better
Json::Value getFieldValue(const Json::Value& field){
	Json::Value output;
	const Json::Value segments = field["segments"];
	for ( size_t i = 0; i < segments.size(); i++ ) {
		const Json::Value segment = segments[i];
		const Json::Value items = segment["items"];
		for ( size_t j = 0; j < items.size(); j++ ) {
			const Json::Value classification = items[j].get("classification", false);
			const Json::Value itemValue = items[j]["value"];
			switch ( classification.type() )
			{
			case Json::stringValue:
				output = Json::Value(output.asString() +
				                     classification.asString());
			break;
			case Json::booleanValue:
				if(!itemValue.isNull()){
					if(classification.asBool()){
						output = Json::Value(output.asString() +
							             itemValue.asString());
					}
					else{
						output = Json::Value(output.asString());
					}
					break;
				}
				//Fall through and be counted as a 1 or 0
			case Json::intValue:
			case Json::uintValue:
				output = Json::Value(output.asInt() + classification.asInt());
			break;
			case Json::realValue:
				output = Json::Value(output.asDouble() + classification.asDouble());
			break;
			default:
			break;
			}
		}
	}
	return output;
}

class Processor::ProcessorImpl : public TemplateProcessor
{
private:
	typedef TemplateProcessor super;

	Mat formImage;

	Aligner aligner;

	Json::Value root;
	//Json::Value JsonOutput;

	typedef std::map<const std::string, cv::Ptr< PCA_classifier> > ClassiferMap;
	ClassiferMap classifiers;

	string templPath;
	string appRootDir;
	//string imageDir;

	#ifdef TIME_IT
		clock_t init;
	#endif


//NOTE: training_data_uri must be a directory with no leading or trailing slashes.
Ptr<PCA_classifier>& getClassifier(const Json::Value& classifier){

	const string& training_data_uri = classifier["training_data_uri"].asString();

	Size acutal_classifier_size = SCALEPARAM * Size(classifier["classifier_width"].asInt(),
	                                                classifier["classifier_height"].asInt());
	ostringstream ss;
	ss << acutal_classifier_size.height << 'x' << acutal_classifier_size.width;
	string key(training_data_uri + ss.str());

	#ifdef DEBUG_PROCESSOR
		cout << "classifierKey: " << key << endl;
	#endif

	ClassiferMap::iterator it = classifiers.find(key);
/*
	#ifdef DEBUG_PROCESSOR
		cout << "erasing old classifier (if it exisits)" << endl;
		if( it != classifiers.end() ) cout << "erased: " << classifiers.erase(key) << endl;
		cout << "erased" << endl;
	#endif
*/
	if( it == classifiers.end() ) {
		//PCA_classifier classifier = classifiers[key];
		classifiers[key] = Ptr<PCA_classifier>(new PCA_classifier);
		
		string dataPath = appRootDir + "training_examples/" + training_data_uri;
		string cachedDataPath = dataPath + "/cached_classifier_data_" + ss.str() + ".yml";

		classifiers[key]->set_classifier_params(classifier);

		try{
			classifiers[key]->load(cachedDataPath);
			#ifdef DEBUG_PROCESSOR
				cout << "found cached classifier..." << endl;
			#endif
		}
		catch(...){
			#ifdef DEBUG_PROCESSOR
				cout << "training new classifier..." << endl;
			#endif
			vector<string> filepaths;
			CrawlFileTree(dataPath, filepaths);//TODO: Check if this dir exists and show a log message if not.
			
			/*
			#ifdef DEBUG_PROCESSOR
				cout << dataPath << endl;
				cout << acutal_classifier_size.width << endl;
				cout << filepaths.size() << endl;
				cout << classifiers.size() << endl;
			#endif
			*/
			const Json::Value advanced = classifier.get("advanced", Json::Value());
			bool success = classifiers[key]->train_PCA_classifier( filepaths,
				                                               acutal_classifier_size,
				                                               advanced.get("eigenvalues", 9).asInt(),
			                                                       advanced.get("flip_training_data", true).asBool());
			if( !success ) {
				//TODO: A better error message here when the training data isn't found would be a big help.
				LOGI("\n\nCould not train classifier.\n\n");
				return classifiers[key];
			}

			classifiers[key]->save(cachedDataPath);

			#ifdef DEBUG_PROCESSOR
				cout << "trained" << endl;
			#endif
		}
	}
	return classifiers[key];
}
Json::Value segmentFunction(const Json::Value& segmentTemplate) {

	Json::Value segmentJsonOut;
	Mat segmentImg;
	vector <Point> segBubbleLocs;
	vector <int> bubbleVals;
	Rect segmentRect( SCALEPARAM * Point(segmentTemplate.get( "segment_x", INT_MIN ).asInt(),
	                                     segmentTemplate.get( "segment_y", INT_MIN ).asInt()),
	                                     SCALEPARAM * Size(segmentTemplate.get("segment_width", INT_MIN).asInt(),
	                                                       segmentTemplate.get("segment_height", INT_MIN).asInt()));
	//Transfromation and offest are used to get absolute bubble locations.
	Mat transformation = (Mat_<double>(3,3) << 1, 0, 0, 0, 1, 0, 0, 0, 1);
	Point offset = segmentRect.tl();

	//Get the cropped segment image:
	if(segmentTemplate.get("align_segment", true).asBool()) {
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

		vector<Point2f> quad;
		findSegment(segmentImg, segmentRect - expandedRect.tl(), quad);

		if(testQuad(quad, segmentRect, .15)){
			#ifdef DEBUG_PROCESSOR
				//This makes a stream of dots so we can see how fast things are going.
				//we get a ! when things go wrong.
				cout << '.' << flush;
			#endif
			transformation = quadToTransformation(quad, segmentRect.size());
			offset = expandedRect.tl();
			Mat alignedSegment(0, 0, CV_8U);
			warpPerspective(segmentImg, alignedSegment, transformation, segmentRect.size());
			segmentImg = alignedSegment;
			
			segmentJsonOut["quad"] = quadToJsonArray( quad, offset );
			/*
			if(inputType == "classifier"){
				segBubbleLocs = getBubbleLocations(*classifier, segmentImg, segmentTemplate["items"], DO_BUBBLE_ALIGNMENT);
				bubbleVals = classifyBubbles( segmentImg, segBubbleLocs, *classifier );
				segmentJsonOut["bubbles"] = genBubblesJson( bubbleVals, segBubbleLocs, 
					                                    expandedRect.tl(), 
				                                            segmentTemplate["bubble_labels"],
				                                            transformation.inv() );
			}*/
		}
		else{
			#ifdef DEBUG_PROCESSOR
				//Bad quad alignment detected
				cout << "!" << flush;
			#endif
			segmentJsonOut["quad"] = quadToJsonArray(quad, expandedRect.tl());
			segmentJsonOut["notFound"] = true;
			return segmentJsonOut;
		}
	}
	else {
		segmentImg = formImage(segmentRect);
		segmentJsonOut["quad"] = quadToJsonArray(rectToQuad( segmentRect ));
		/*
		else if(inputType == "classifier"){
			segBubbleLocs = getBubbleLocations(*classifier, segmentImg,
			                                   segmentTemplate["items"], false);
			bubbleVals = classifyBubbles( segmentImg, segBubbleLocs, *classifier );
			segmentJsonOut["bubbles"] = genBubblesJson( bubbleVals, segBubbleLocs, segmentRect.tl(), segmentTemplate["bubble_labels"]);
		}*/
	}

	//Do classification stuff:
	Json::Value items = segmentTemplate["items"];
	if(!items.isNull()){
		Json::Value itemsJsonOut;
		Json::Value classifierJson = segmentTemplate["classifier"];
		bool alignItems = classifierJson.isMember("alignment_radius");
		Ptr<PCA_classifier> classifier = getClassifier(classifierJson);
		
		for (size_t i = 0; i < items.size(); i++) {
			Json::Value itemJsonOut = items[i];

			//Classify the item
			Point itemLocation = SCALEPARAM * Point(items[i]["item_x"].asDouble(), items[i]["item_y"].asDouble());
			if(alignItems){
				itemLocation = classifier->align_item(segmentImg, itemLocation);
			}
			itemJsonOut["classification"] = classifier->classify_item(segmentImg, itemLocation);
			
			//Create JSON output
			Mat absoluteLocation = transformation.inv() * Mat(Point3d( itemLocation.x,
		                                                                   itemLocation.y, 1.f));
			itemJsonOut["absolute_location"] = pointToJson(
				Point( absoluteLocation.at<double>(0,0) / absoluteLocation.at<double>(2, 0),
				       absoluteLocation.at<double>(1,0) / absoluteLocation.at<double>(2, 0)) +
				offset);

			itemsJsonOut.append(itemJsonOut);
		}
		segmentJsonOut["items"] = itemsJsonOut;
	}
	
	//Output the segment image:
	Mat segment_out;
	cvtColor(segmentImg, segment_out, CV_GRAY2RGB);
	/*
	vector <Point> expectedBubbleLocs = getBubbleLocations(*classifier, segmentImg, segmentTemplate["items"], false);

	Point classifier_size = jsonToPoint( segmentTemplate["classifier_size"] );

	for(size_t i = 0; i < expectedBubbleLocs.size(); i++){
		rectangle(segment_out, expectedBubbleLocs[i] - .5 * classifier_size,
		          expectedBubbleLocs[i] + .5 * classifier_size,
		          colors[bubbleVals[i]]);
						   
		circle(segment_out, segBubbleLocs[i], 1, Scalar(255, 2555, 255), -1);
	}
	*/
	string segmentOutPath;
	string segmentName;
	try{
		segmentOutPath = segmentTemplate.get("outputPath", 0).asString() + "segments/";
		segmentName = segmentTemplate["name"].asString() + "_image_" +
		              intToStr(segmentTemplate.get("index", 0).asInt()) + ".jpg";
		imwrite(segmentOutPath + segmentName, segment_out);
		segmentJsonOut["imagePath"] = segmentOutPath + segmentName;
	}
	catch(...){
		LOGI(("Could not output segment to: " + segmentOutPath+segmentName).c_str());
	}
	return segmentJsonOut;
}
Json::Value fieldFunction(const Json::Value& field){
	Json::Value fieldJsonOut;

	if(field.get("mask", false).asBool()) {
		cout << "mask" << endl;
		return Json::Value();
	}

	#ifdef OUTPUT_BUBBLE_IMAGES
		namer.setPrefix(field.get("label", "unlabeled").asString());
	#endif

	//Add segment index numbers:
	Json::Value mutableField;
	const Json::Value segments = field["segments"];
	Json::Value outSegments;
	for ( size_t j = 0; j < segments.size(); j++ ) {
		Json::Value segment = segments[j];
		segment["index"] = (int)j;
		outSegments.append(segment);
	}
	mutableField = field;
	mutableField["segments"] = outSegments;

	fieldJsonOut = super::fieldFunction(mutableField);
	inheritMembers(fieldJsonOut, mutableField);
	fieldJsonOut["value"] = getFieldValue(fieldJsonOut);
	return fieldJsonOut;
}
Json::Value formFunction(const Json::Value& templateRoot){
	Json::Value outForm = super::formFunction(templateRoot);
	outForm["form_scale"] = Json::Value(SCALEPARAM);
	outForm["templatePath"] = Json::Value(templPath);
	//TODO: replace templatePath instance with templateDir
	outForm["template_dir"] = Json::Value(templPath);
	return outForm;
}

public:

ProcessorImpl(const string& appRootDir) : appRootDir(string(appRootDir)) {}

//The template is persistent because it is used for both alignment and processing.
bool setTemplate(const char* templatePath) {
	#ifdef DEBUG_PROCESSOR
		cout << "setting template..." << endl;
	#endif
	templPath = string(templatePath);
	//We don't need a file extension so this removes it if one is provided.
	if(templPath.find(".") != string::npos){
		templPath = templPath.substr(0, templPath.find_last_of("."));
	}
	
	if(!parseJsonFromFile(templPath + ".json", root)) return false;
	return true;
}
bool loadFormImage(const char* imagePath, bool undistort) {
	#ifdef DEBUG_PROCESSOR
		cout << "loading form image..." << flush;
	#endif
	#ifdef TIME_IT	
		init = clock();
	#endif
	Mat temp;
	
	formImage = imread(imagePath, 0);
	if(formImage.empty()) return false;
	
	//Automatically rotate90 if the image is wider than it is tall
	//We want to keep the orientation consistent because:
	//1. It seems to make a slight difference in alignment. (SURF is not *completely* rotation invariant)
	//2. Undistortion
	//Maybe need to watch out incase a clock-wise rotation results in upsidown photos on some phones.
	if(formImage.cols > formImage.rows){
		transpose(formImage, temp);
		flip(temp,formImage, 1);
	}

	if(undistort){
		Mat cameraMatrix, distCoeffs;
		Mat map1, map2;
		Size imageSize = formImage.size();
		
		String cameraCalibFile(appRootDir + "camera.yml");
		
		if( !fileExists(cameraCalibFile) ) return false;
		
		FileStorage fs(cameraCalibFile, FileStorage::READ);
		fs["camera_matrix"] >> cameraMatrix;
		fs["distortion_coefficients"] >> distCoeffs;
		
		initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(),
		                        getOptimalNewCameraMatrix(cameraMatrix,
		                                                  distCoeffs, imageSize, 0, imageSize, 0),
		                        imageSize, CV_16SC2, map1, map2);

		remap(formImage, temp, map1, map2, INTER_LINEAR);
		formImage = temp;
	}
/*
	string path = string(imagePath);
	imageDir = path.substr(0, path.find_last_of("/") + 1);
*/	
	#ifdef TIME_IT
		LOGI("LoadFormImage time: ");
		ostringstream ss;
		ss << (double)(clock()-init) / ((double)CLOCKS_PER_SEC);
		LOGI( ss.str().c_str() );
	#endif
	#ifdef DEBUG_PROCESSOR
		cout << "loaded" << endl;
	#endif
	return true;
}
bool alignForm(const char* alignedImageOutputPath, size_t formIdx) {
	#ifdef DEBUG_PROCESSOR
		cout << "aligning form..." << endl;
	#endif
	#ifdef TIME_IT	
		init = clock();
	#endif
	Mat straightenedImage;
	try{
		Size form_sz(root.get("width", 0).asInt(), root.get("height", 0).asInt());
		
		if( form_sz.width <= 0 || form_sz.height <= 0)
			CV_Error(CV_StsError, "Invalid form dimension in template.");
		
		//If the image was not set (because form detection didn't happen) set it.
		if( aligner.currentImg.empty() ) aligner.setImage(formImage);

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
	//JsonOutput["aligned_image_path"] = alignedImageOutputPath;

	#ifdef TIME_IT
		LOGI("alignForm time: ");
		ostringstream ss;
		ss << (double)(clock()-init) / ((double)CLOCKS_PER_SEC);
		LOGI( ss.str().c_str() );
	#endif
	#ifdef DEBUG_PROCESSOR
		cout << "aligned" << endl;
	#endif
	return writeFormImage(alignedImageOutputPath);
}
bool processForm(const char* outputPath) {
	#ifdef  DEBUG_PROCESSOR
		cout << "Processing form" << endl;
	#endif
	#ifdef TIME_IT	
		init = clock();
	#endif
	
	if( !root || formImage.empty() ){
		cout << "Unable to process form. Error code: " <<
		        (int)!root << (int)formImage.empty() << endl;
		return false;
	}
	root["outputPath"] = outputPath;
	const Json::Value JsonOutput = formFunction(root);

	#ifdef  DEBUG_PROCESSOR
		cout << "done" << endl;
	#endif
	
	#ifdef  DEBUG_PROCESSOR
		cout << "outputting bubble vals... " << endl;
	#endif

	ofstream outfile((string(outputPath) + "output.json").c_str(), ios::out | ios::binary);
	outfile << JsonOutput;
	outfile.close();

	#ifdef TIME_IT
		LOGI("Process time: ");
		ostringstream ss;
		ss << (double)(clock()-init) / ((double)CLOCKS_PER_SEC);
		LOGI( ss.str().c_str() );
	#endif
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
//TODO: If the wrong path is specified this causes a freeze which I should fix.
bool loadFeatureData(const char* templatePathArg) {
	try{
		string templatePath(templatePathArg);
		if(*templatePath.rbegin() != '/'){
			//Eventually this will be deprecated
			aligner.loadFeatureData(templatePath);
		}
		else{
			aligner.loadFeatureData(templatePath + "form.jpg",
			                        templatePath + "template.json",
			                        templatePath + "cached_features.yml");
		}
	}
	catch(cv::Exception& e){
		LOGI(e.what());
		return false;
	}
	return true;
}
int detectForm(){
	int formIdx;
	try{
		LOGI("Detecting form...");
		aligner.setImage(formImage);
		formIdx = (int)aligner.detectForm();
	}
	catch(cv::Exception& e){
		LOGI(e.what());
		return -1;
	}
	return formIdx;
}
};

/* This stuff hooks the Processor class up to the implementation class: */
Processor::Processor() : processorImpl(new ProcessorImpl("")){
	LOGI("Processor successfully constructed.");
}
Processor::Processor(const char* appRootDir) : processorImpl(new ProcessorImpl(addSlashIfNeeded(string(appRootDir)))){
	LOGI("Processor successfully constructed.");
}
bool Processor::loadFormImage(const char* imagePath, bool undistort){
	return processorImpl->loadFormImage(imagePath, undistort);
}
bool Processor::loadFeatureData(const char* templatePath){
	return processorImpl->loadFeatureData(templatePath);
}
int Processor::detectForm(){
	return processorImpl->detectForm();
}
bool Processor::setTemplate(const char* templatePath){
	return processorImpl->setTemplate(templatePath);
}
bool Processor::alignForm(const char* alignedImageOutputPath, int formIdx){
	if(formIdx < 0) return false;
	return processorImpl->alignForm(alignedImageOutputPath, (size_t)formIdx);
}
bool Processor::processForm(const char* outPath) {
	return processorImpl->processForm(outPath);
}
bool Processor::writeFormImage(const char* outputPath) const{
	return processorImpl->writeFormImage(outputPath);
}
