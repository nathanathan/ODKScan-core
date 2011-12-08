#include <json/value.h>
#include <iostream>
#include <fstream>


//TODO: return refrence instead?
//TODO: Make it so empty json values are not appended, if they are appended.

class TemplateProcessor
{
	public:

	//inheritMembers makes the child value inherit the members that it does not override from the specified parent json value.
	//The parent is copied so it can be written over, while the child is passed in and returned with added members by refrence.
	Json::Value& inheritMembers(Json::Value& child, Json::Value parent) const {
		Json::Value::Members members = child.getMemberNames();
		for( Json::Value::Members::iterator itr = members.begin() ; itr != members.end() ; itr++ ) {
			//cout << *itr << flush;
			//TODO: If inheriting a Json Object, perhaps I could recusively inherit:
			//parent[*itr] = inheritMembers(child[*itr], parent[*itr]);
			parent[*itr] = child[*itr];
		}
		//I'm worried that we end up with refrences to elements of the stack allocated parent copy.
		//However, I think operator= is overloaded so that this doesn't happen.
		return child = parent;
	}
	//Include the inheritJson function

	//XXX: If you override these, you should call the base class functions after your code to keep descending.
	virtual Json::Value segmentFunction(const Json::Value& segment){
		//This will usually be the function to override
		std::cout << "AAA" << std::endl;
		return Json::Value();
	}
	virtual Json::Value fieldFunction(const Json::Value& field){
		const Json::Value segments = field["segments"];
		Json::Value outfield;
		Json::Value outSegments;
		
		for ( size_t j = 0; j < segments.size(); j++ ) {
			Json::Value segment = segments[j];
			inheritMembers(segment, field);
			outSegments.append(segmentFunction(segment));
		}

		outfield["segments"] = outSegments;
		return outfield;
	}
	virtual Json::Value formFunction(const Json::Value& templateRoot){
		const Json::Value fields = templateRoot["fields"];
		Json::Value outform;
		Json::Value outfields;
	
		for ( size_t i = 0; i < fields.size(); i++ ) {
			Json::Value field = fields[i];
			inheritMembers(field, templateRoot);
			outfields.append(fieldFunction(field));
		}

		outform["fields"] = outfields;
		return outform;
	}
	virtual bool start(const char* templatePath){
		Json::Value templateRoot;
		if( parseJsonFromFile(templatePath, templateRoot) ){
			formFunction(templateRoot);
			return true;		
		}
		return false;
	}


	virtual ~TemplateProcessor(){}
};
