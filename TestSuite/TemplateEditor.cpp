#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/legacy/compat.hpp>

#include <json/json.h>

#include <iostream>
#include <fstream>

#include "Processor.h"
#include "PCA_classifier.h"
#include "FormAlignment.h"
#include "Addons.h"
#include "MarkupForm.h"

#define OUTPUT_SEGMENT_IMAGES

//The scale param can be used to generate templates at higher res but the templates are not scaled by it.
#define SCALEPARAM 2.0

// Creates a buffer around segments porpotional to their size.
// I think .5 is the largest value that won't cause ambiguous cases.
#define SEGMENT_BUFFER .25

#ifdef OUTPUT_SEGMENT_IMAGES
#include "NameGenerator.h"
NameGenerator namer2("debug_segment_images/");
#endif

using namespace std;
using namespace cv;

Json::Value root;
Mat formImage;
PCA_classifier classifier;
string templDir;
MarkupForm marker;
/*
//This function was one time use for a specific template, but might be modable to do something useful in the future.
Json::Value groupSegmentsByY(const Json::Value& fields){
	vector <int> ys;
	vector <Json::Value> outFields0to11meses;
	vector <Json::Value> outFields12to23meses;
	vector <Json::Value> outFieldsFemininos;
	vector <Json::Value> outFieldsMasculinos;
	
	for ( size_t i = 0; i < fields.size(); i++ ) {
		Json::Value field = fields[i];
		int y = field["segments"][0u]["y"].asInt();
		if (y < 200) continue;
		ys.push_back(y);
		field["segments"].clear();
		
		if (y < 900) {
			Json::Value field1 = Json::Value(field);
			field1["label"] = field1["label"].asString() + " 0 to 11 meses";
			outFields0to11meses.push_back(field1);
		
			Json::Value field2 = Json::Value(field);
			field2["label"] = field2["label"].asString() + " 12 to 23 meses";
			outFields12to23meses.push_back(field2);
		}
		else{
			Json::Value field1 = Json::Value(field);
			field1["label"] = field1["label"].asString() + " femeninos";
			outFieldsFemininos.push_back(field1);
			
			Json::Value field2 = Json::Value(field);
			field2["label"] = field2["label"].asString() + " masculinos";
			outFieldsMasculinos.push_back(field2);
		}
	}
	for ( size_t i = 0; i < fields.size(); i++ ) {
		const Json::Value segments = fields[i]["segments"];
		for ( size_t j = 0; j < segments.size(); j++ ) {
			const Json::Value segment = segments[j];
			int y = segment["y"].asInt();
			int min_dist = INT_MAX;
			size_t min_idx;
			for(size_t k = 0; k < ys.size(); k++){
				if(abs(y - ys[k]) < min_dist){
					min_dist = abs(y - ys[k]);
					min_idx = k;
				}
			}
			if( min_idx < 9 ){
				if(outFields0to11meses[min_idx]["segments"].size() < 6){
					outFields0to11meses[min_idx]["segments"].append(segment);
				}
				else{
					outFields12to23meses[min_idx]["segments"].append(segment);
				}
			}
			else{
				if(outFieldsFemininos[0u]["segments"].size() < 5){
					outFieldsFemininos[0u]["segments"].append(segment);
				}
				else{
					outFieldsMasculinos[0u]["segments"].append(segment);
				}
			}
		}
	}
	Json::Value outFieldsJson;
	for(size_t i = 0; i < outFields0to11meses.size(); i++){
		outFieldsJson.append(outFields0to11meses[i]);
	}
	for(size_t i = 0; i < outFields12to23meses.size(); i++){
		outFieldsJson.append(outFields12to23meses[i]);
	}
	for(size_t i = 0; i < outFieldsFemininos.size(); i++){
		outFieldsJson.append(outFieldsFemininos[i]);
	}
	for(size_t i = 0; i < outFieldsMasculinos.size(); i++){
		outFieldsJson.append(outFieldsMasculinos[i]);
	}
	return outFieldsJson;
}
*/
Json::Value findBubbleLocations(Mat& segment, const Json::Value& bubbleLocations, bool doSomething){
	#ifdef OUTPUT_SEGMENT_IMAGES
	//TODO: This should become perminant functionality.
	//		My idea is to have Java code that displays segment images as the form is being processed.
	Mat dbg_out;
	cvtColor(segment, dbg_out, CV_GRAY2RGB);
	dbg_out.copyTo(dbg_out);
	#endif
	Json::Value bubblesLocationsOut;
	for (size_t i = 0; i < bubbleLocations.size(); i++) {
		Point bubbleLocation = SCALEPARAM * jsonToPoint(bubbleLocations[i]);
		/*
		if(bubbleLocation.x > segment.cols || bubbleLocation.y > segment.rows ||
									bubbleLocation.x < 0 || bubbleLocation.y < 0){
			continue; //Skip out of bounds bubbles
		}

		if( doSomething ){

			if(bubbleLocation.y == 31){
				bubbleLocation.y = 33;
			}
			if(bubbleLocation.y < 28 && bubbleLocation.y > 17){
				bubbleLocation.y +=2;
			}

			cout << "AAAAAAAA" << endl;
		}

		if( doSomething ){
			if(bubbleLocation.x == 8){
				bubbleLocation.x += 3;
			}
			if(bubbleLocation.x == 16){
				bubbleLocation.x += 2;
			}
			if(bubbleLocation.y > 45){
				bubbleLocation.y -= 1;
			}
			cout << "AAAAAAAA" << endl;
		}
		
		bubbleLocation.x *= .9;
		bubbleLocation.y += 2;
		bubbleLocation.y *= 1.01;
		if(bubbleLocation.y == 46){
			bubbleLocation.y = 45;
		}
		if(bubbleLocation.y == 20){
			bubbleLocation.y = 18;
		}
		if(bubbleLocation.y == 33){
			bubbleLocation.y = 31;
		}
		if(bubbleLocation.x == 8){
			bubbleLocation.x == 9;
		}*/
		
		Point refined_location = classifier.bubble_align(segment, bubbleLocation);
		bubblesLocationsOut.append( pointToJson(1.f/SCALEPARAM * refined_location));
		#ifdef OUTPUT_SEGMENT_IMAGES
		Scalar color(0, 255, 0);
		rectangle(dbg_out, bubbleLocation-Point(classifier.exampleSize.width/2,classifier.exampleSize.height/2),
						   bubbleLocation+Point(classifier.exampleSize.width/2,classifier.exampleSize.height/2),
						   color);
		
		circle(dbg_out, refined_location, 1, Scalar(255, 0, 255), -1);
		#endif
	}

	#ifdef OUTPUT_SEGMENT_IMAGES
	string segfilename = namer2.get_unique_name("marked_");
	segfilename.append(".jpg");
	imwrite(segfilename, dbg_out);
	#endif
	return bubblesLocationsOut;
}
bool about(int x, int y){
	return x < y + 6 && x > y - 6;
}
Json::Value processSegment(const Json::Value &segmentTemplate, bool doSomething){
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
	
	vector<Point> quad = findBoundedRegionQuad(segment, SEGMENT_BUFFER);
	
	Mat transformation = quadToTransformation(quad, imageRect.size());
	Mat alignedSegment;
	warpPerspective(segment, alignedSegment, transformation, imageRect.size());

	//This is a hacky of ensuring that the quad has the right ordering of points.
	quad = transformationToQuad(transformation, imageRect.size());
	
	Point h = .5*( quad[3]-quad[0] + quad[2]-quad[1] );
	Point w = .5*( quad[1]-quad[0] + quad[2]-quad[3] );
	Point topLeft = .25*(quad[0] + quad[1]-w + quad[2] - h - w + quad[3]-h) + expandedRect.tl();
	/*
	if(w.x > 10 && h.y > 10){
		for(size_t i = 0; i<4; i++){
			cout <<  quad[i].x << ", " << quad[i].y << endl;
		}
	}*/
	Json::Value segmentJsonOut;
	segmentJsonOut["x"] =  (int)(topLeft.x/SCALEPARAM);//imageRect.tl().x;
	segmentJsonOut["y"] =  (int)(topLeft.y/SCALEPARAM);//imageRect.tl().y;
	segmentJsonOut["width"] = (int)(w.x/SCALEPARAM);//imageRect.width;
	segmentJsonOut["height"] = (int)(h.y/SCALEPARAM);//imageRect.height;

	segmentJsonOut["bubble_locations"] = findBubbleLocations(alignedSegment, segmentTemplate["bubble_locations"], doSomething);
	
	/*, 
											about(topLeft.x, 185 ) || 
											about(topLeft.x, 283 ) ||
											about(topLeft.x, 380 ) ||
											about(topLeft.x, 431 ) ||
											about(topLeft.x, 579 ) && topLeft.y < 900);*/

	return segmentJsonOut;
}
bool trainClassifier(const char* trainingImageDir, bool flipExamples){
	#if DEBUG > 0
	cout << "training classifier...";
	#endif
	const Json::Value defaultSize = pointToJson(Point(5,8));
	Json::Value bubbleSize = root.get("bubble_size", defaultSize);
	bool success = classifier.train_PCA_classifier(string(trainingImageDir), 
													SCALEPARAM * Size(bubbleSize[0u].asInt(), bubbleSize[1u].asInt()), flipExamples);
	if (!success) return false;
	
	//classifier.set_search_window(Size(7,18));
	//classifier.set_search_window(Size(0,0));
	#if DEBUG > 0
	cout << "trained" << endl;
	#endif
}
bool loadForm(const char* imagePath){
	#if DEBUG > 0
	cout << "loading form image..." << endl;
	#endif
	Mat temp = imread(imagePath, 0);
	if(temp.empty()) return false;
	resize(temp, formImage, SCALEPARAM * temp.size(), 0, 0, INTER_CUBIC);
	return true;
}
//TODO: Needed methods:
//		Group segments by field.
//		Output Template but with altered coordinates.
bool processTemplate(const char* outputPath) {
	#if DEBUG > 0
	cout << "debug level is: " << DEBUG << endl;
	#endif
	
	if( !root || !formImage.data || !classifier.trained()){
		cout << "Unable to process form. Err code: " <<
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
			
			Json::Value segmentJsonOut = processSegment(segmentTemplate, 
					field.get("label", "unlabeled").asString().find("masculinos") != string::npos);
					
			segmentJsonOut["key"] = j;
			fieldJsonOut["segments"].append(segmentJsonOut);
		}
		fieldJsonOut["key"] = i;
		JsonOutputFields.append(fieldJsonOut);
	}
	
	Json::Value JsonOutput = root;
	JsonOutput["fields"] = JsonOutputFields;
	
	#if DEBUG > 0
	cout << "outputting... " << endl;
	#endif
	ofstream outfile(outputPath, ios::out | ios::binary);
	outfile << JsonOutput;
	outfile.close();
	return true;
}
bool markupForm(const char* formPath, const char* markupPath, Mat& markupImage) {
	
	if(!marker.markupForm(markupPath, formPath, formPath)) return false;
	markupImage = imread(formPath);
	return !markupImage.empty();
}
int main(int argc, char *argv[]) {

	classifier = PCA_classifier(9);

	string formPath("form_templates/A0_from_booklet.jpg");
	string templatePath("form_templates/unbounded_form_refined.json");
	string templateOutPath("form_templates/unbounded_form_refined2.json");
	
	templDir = string(templatePath);
	templDir = templDir.substr(0, templDir.find_last_of("/") + 1);
	string templateImgDir = templDir+root.get("feature_data_path", "default").asString() + ".jpg";

	if( !parseJsonFromFile(templatePath.c_str(), root) ) return false;
	loadForm(formPath.c_str());
	trainClassifier("training_examples/empty", true);
	processTemplate(templateOutPath.c_str());

	Mat markupImage;
	if( !markupForm(formPath.c_str(), templateOutPath.c_str(), markupImage) ) return false;

	const string winName = "marked-up image";
	namedWindow(winName, CV_WINDOW_NORMAL);
	imshow( winName, markupImage );

	for(;;)
    {
        char c = (char)waitKey(0);
        if( c == '\x1b' ) // esc
        {
            cout << "Exiting ..." << endl;
            return 0;
        }
        if( c == 'l' )
        {
        	//First fix offset scale then do this...
    		if( !parseJsonFromFile(templateOutPath.c_str(), root) ) return false;
			processTemplate(templateOutPath.c_str());
			
        	if( !markupForm(formPath.c_str(), templateOutPath.c_str(), markupImage) ) return false;
			imshow( winName, markupImage );
			
			//if( !markupForm(templatePath.c_str(), markupImage) ) return false;
			//imshow( winName, markupImage );
        }
    }
}

