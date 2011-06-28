#ifndef NAME_GENERATOR
#define NAME_GENERATOR
#include <iostream>
class NameGenerator {
		int unique_name_counter;
		string initial_prefix;
	public:
		NameGenerator (string prefix){
			unique_name_counter = 0;
			initial_prefix = prefix;
		}
		//Returns prefix with a number concatinated to the end.
		//Each call increments the number so that the resulting string will always be unique.
		string get_unique_name(string prefix) {
			stringstream ss;
			int gc_temp = unique_name_counter;
			while( true ) {
				ss << (char) ((gc_temp % 10) + '0');
				gc_temp = gc_temp / 10;
				if(gc_temp == 0)
					break;
			}
			string temp = ss.str();
			reverse(temp.begin(), temp.end());
			unique_name_counter+=1;
			return initial_prefix + prefix + temp;
		}
};
#endif
