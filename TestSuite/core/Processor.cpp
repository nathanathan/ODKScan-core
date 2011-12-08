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


#ifdef OUTPUT_SEGMENT_IMAGES
	#include "NameGenerator.h"
	NameGenerator namer;
#endif

//#define TIME_IT
#define CLASSIFY_BUBBLES true
#define DO_BUBBLE_ALIGNMENT true

#define REFINE_ALL_BUBBLE_LOCATIONS true

//#define DO_BUBBLE_INFERENCE

using namespace std;
using namespace cv;

Json::Value genBubblesJson( const vector<int>& bubbleVals, const vector<Point>& bubbleLocations, 
                            const Point& offset, const Mat& transformation =
                                                 (Mat_<double>(3,3) << 1, 0, 0, 0, 1, 0, 0, 0, 1)) {
					
	assert(bubbleLocations.size() == bubbleVals.size());
	Json::Value out;
	for(size_t i = 0; i < bubbleVals.size(); i++){
		Json::Value bubble;
		bubble["value"] = Json::Value( bubbleVals[i] );
		Mat actualLocation = transformation * Mat(Point3d( bubbleLocations[i].x,
                                                                   bubbleLocations[i].y, 1.f));
		bubble["location"] = pointToJson(Point( actualLocation.at<double>(0,0) / actualLocation.at<double>(2, 0),
		                                        actualLocation.at<double>(1,0) / actualLocation.at<double>(2, 0)) + offset);
		out.append(bubble);
	}
	return out;
}

class Processor::ProcessorImpl : public TemplateProcessor
{
private:
	typedef TemplateProcessor super;
	Mat formImage;
	Aligner aligner;
	Json::Value root;
	Json::Value JsonOutput;
	typedef std::map<const std::string, cv::Ptr< PCA_classifier> > ClassiferMap;
	ClassiferMap classifiers;
	string templPath;
	string appRootDir;

	#ifdef TIME_IT
		clock_t init;
	#endif


//NOTE: training_data_uri must be a directory with no leading or trailing slashes.
Ptr<PCA_classifier>& getClassifier(const string& training_data_uri, const Point& classifier_size){

/*
	#ifdef DEBUG_PROCESSOR
		cout << "getClassifier: " << training_data_uri << ", " << classifier_size << endl;
	#endif
*/
	ostringstream ss;

	Size acutal_classifier_size = SCALEPARAM * Size(classifier_size.x, classifier_size.y);
	ss << acutal_classifier_size.height << acutal_classifier_size.width;
	
	string key(training_data_uri + ss.str());
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
		string cachedDataPath = dataPath + "/cached_classifier_data" + ss.str() + ".yml";

		bool createClassifier = true;
		//TODO: Reenable cacheing once things work
		if( fileExists(cachedDataPath) ) {
			#ifdef DEBUG_PROCESSOR
				cout << "found cached classifier..." << endl;
			#endif
			//TODO: Why do I need the size here?
			createClassifier = !classifiers[key]->load(cachedDataPath, acutal_classifier_size);
		}
		if( createClassifier ) {
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
			*/
			bool success = classifiers[key]->train_PCA_classifier( filepaths,
				                                              acutal_classifier_size,
				                                              EIGENBUBBLES, true);//flip training examples. TODO: put this stuff in template
			if( !success ) {
				LOGI("\n\nThings are going to break.\n\n");
				return classifiers[key];
			}

			classifiers[key]->save(cachedDataPath);

			#ifdef DEBUG_PROCESSOR
				cout << "trained" << endl;
			#endif
		}
		classifiers[key]->set_search_window(Size(5,5)); //TODO: put in template
		return classifiers[key];
	}
	return classifiers[key];
}
//TODO: Could probably do with some refactoring here.
//TODO: Make some debug functions (for example one that shows an image)
vector<int> classifyBubbles(const Mat& segment, const vector<Point> bubbleLocations, const PCA_classifier& classifier) {
	vector<int> out;
	for (size_t i = 0; i < bubbleLocations.size(); i++) {
		out.push_back( classifier.classifyBubble(segment, bubbleLocations[i]) ); 
	}
	return out;
}
//TODO: Here's a problem, I have to pass in the classifer, which doesn't seem nice.
//	Maybe this should be part of the classifier class?
vector<Point> getBubbleLocations(const PCA_classifier& classifier, const Mat& segment, const Json::Value& bubbleLocationsJSON, bool refine) {

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
			bubbleLocations.push_back(
				classifier.bubble_align(segment,
				                        SCALEPARAM * jsonToPoint(bubbleLocationsJSON[i]) ) );
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
Json::Value segmentFunction(const Json::Value& segmentTemplate) {

	Json::Value segmentJsonOut;
	Mat segmentImg;
	vector <Point> segBubbleLocs;
	vector <int> bubbleVals;

	Ptr<PCA_classifier> classifier = getClassifier(segmentTemplate["training_data_uri"].asString(),
	                                               jsonToPoint(segmentTemplate["classifier_size"]));

	Rect segmentRect( SCALEPARAM * Point(segmentTemplate.get( "x", INT_MIN ).asInt(),
	                                     segmentTemplate.get( "y", INT_MIN ).asInt()),
	                                     SCALEPARAM * Size(segmentTemplate.get("segment_width", INT_MIN).asInt(),
	                                                       segmentTemplate.get("segment_height", INT_MIN).asInt()));

	string inputType = segmentTemplate.get("in_type", "classifier").asString();
	
	if(!segmentTemplate.get("bounded", true).asBool()) { //If segments don't have a bounding box
		segmentImg = formImage(segmentRect);
		segmentJsonOut["quad"] = quadToJsonArray(rectToQuad( segmentRect ));
		if(inputType == "classifier"){
			segBubbleLocs = getBubbleLocations(*classifier, segmentImg, segmentTemplate["bubble_locations"], false);
			bubbleVals = classifyBubbles( segmentImg, segBubbleLocs, *classifier );
			segmentJsonOut["bubbles"] = genBubblesJson( bubbleVals, segBubbleLocs, segmentRect.tl() );
		}
	}
	else {
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

		if(testQuad(quad, segmentRect, .15) && CLASSIFY_BUBBLES){
			#ifdef DEBUG_PROCESSOR
				//This makes a stream of dots so we can see how fast things are going.
				//we get a ! when things go wrong.
				cout << '.' << flush;
			#endif
			Mat transformation = quadToTransformation(quad, segmentRect.size());
			Mat alignedSegment(0, 0, CV_8U);
			warpPerspective(segmentImg, alignedSegment, transformation, segmentRect.size());
			segmentImg = alignedSegment;
			
			segmentJsonOut["quad"] = quadToJsonArray( quad, expandedRect.tl() );

			if(inputType == "classifier"){
				segBubbleLocs = getBubbleLocations(*classifier, segmentImg, segmentTemplate["bubble_locations"], DO_BUBBLE_ALIGNMENT);
				bubbleVals = classifyBubbles( segmentImg, segBubbleLocs, *classifier );
				segmentJsonOut["bubbles"] = genBubblesJson( bubbleVals, segBubbleLocs, 
					                                    expandedRect.tl(), transformation.inv() );
			}
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

	#ifdef OUTPUT_SEGMENT_IMAGES
		//TODO: Maybe add Java code that displays segment images as the form is being processed.
		Mat dbg_out;
		cvtColor(segmentImg, dbg_out, CV_GRAY2RGB);
		
		vector <Point> expectedBubbleLocs = getBubbleLocations(*classifier, segmentImg, segmentTemplate["bubble_locations"], false);
		
		Point classifer_size = jsonToPoint( segmentTemplate["classifer_size"] );

		for(size_t i = 0; i < expectedBubbleLocs.size(); i++){
			rectangle(dbg_out, expectedBubbleLocs[i] - .5 * classifer_size,
			          expectedBubbleLocs[i] + .5 * classifer_size,
			          getColor(bubbleVals[i]));
							   
			circle(dbg_out, segBubbleLocs[i], 1, Scalar(255, 2555, 255), -1);
		}
		//TODO: name by field and segment# instead.
		string segfilename = namer.get_unique_name("marked_");
		imwrite("debug_segment_images/" + segfilename + ".jpg", dbg_out);
	#endif

	return segmentJsonOut;
}
Json::Value fieldFunction(const Json::Value& field){
	Json::Value fieldJsonOut;

	if(field.get("mask", false).asBool()) Json::Value();
	//cout << field.get("label", "unlabeled") << endl;
	#ifdef OUTPUT_SEGMENT_IMAGES
		namer.setPrefix(field.get("label", "unlabeled").asString());
	#endif

	fieldJsonOut = super::fieldFunction(field);
	fieldJsonOut["label"] = field.get("label", "unlabeled");
	fieldJsonOut["out_type"] = field.get("out_type", "number");
	fieldJsonOut["key"] = field.get("key", -1);//TODO: I want to get rid of this

	return fieldJsonOut;
}
Json::Value formFunction(const Json::Value& templateRoot){
	Json::Value outForm = super::formFunction(templateRoot);
	outForm["form_scale"] = Json::Value(SCALEPARAM);
	return outForm;
}

public:

ProcessorImpl(const string& appRootDir) : appRootDir(string(appRootDir)) {}

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
	
	JsonOutput["template_path"] = templPath;
	//trainClassifier(JsonOutput.get("training_image_directory", "").asString());
	
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
                                getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, imageSize, 0, imageSize, 0),
                                imageSize, CV_16SC2, map1, map2);
		remap(formImage, temp, map1, map2, INTER_LINEAR);
		formImage = temp;
	}

	JsonOutput["image_path"] = imagePath;
	
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
	JsonOutput["aligned_image_path"] = alignedImageOutputPath;

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
//Sorry for the confusing names
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
	
	JsonOutput = formFunction(root);

	#ifdef  DEBUG_PROCESSOR
		cout << "done" << endl;
	#endif
	
	#ifdef  DEBUG_PROCESSOR
		cout << "outputting bubble vals... " << endl;
	#endif

	ofstream outfile(outputPath, ios::out | ios::binary);
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
bool loadFeatureData(const char* templatePath){
	try{
		aligner.loadFeatureData(templatePath);
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
		cout << "Detecting form..." << endl;
		aligner.setImage(formImage);
		formIdx = aligner.detectForm();
	}
	catch(cv::Exception& e){
		LOGI(e.what());
		return -1;
	}
	return (int)formIdx;
}
};

string addSlashIfNeeded(const string& str){
	if(*str.rbegin() != '/'){
		return str + "/";
	}
	return str;
}

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
bool Processor::processForm(const char* outPath) const{
	return processorImpl->processForm(outPath);
}
bool Processor::writeFormImage(const char* outputPath) const{
	return processorImpl->writeFormImage(outputPath);
}
