#ifndef NAME_GENERATOR
#define NAME_GENERATOR
#include <iostream>
#include <string>

class NameGenerator {
		int unique_name_counter;
		std::string initial_prefix;
	public:
		NameGenerator(std::string prefix){
			unique_name_counter = 0;
			initial_prefix = prefix;
		}
		//Returns prefix with a number concatinated to the end.
		//Each call increments the number so that the resulting string will always be unique.
		std::string get_unique_name(std::string prefix) {
			std::stringstream ss;
			int gc_temp = unique_name_counter;
			while( true ) {
				ss << (char) ((gc_temp % 10) + '0');
				gc_temp = gc_temp / 10;
				if(gc_temp == 0)
					break;
			}
			std::string temp = ss.str();
			reverse(temp.begin(), temp.end());
			unique_name_counter+=1;
			return initial_prefix + prefix + temp;
		}
};
#endif
