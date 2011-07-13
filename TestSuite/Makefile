#A very helpful source: http://stackoverflow.com/questions/5799820/makefile-and-c-project
#TEST_SRCS := $(addsuffix .cpp, $(addprefix tests/, $(TESTS)))
MY_SRCS := Processor.cpp PCA_classifier.cpp FormAlignment.cpp FileUtils.cpp Addons.cpp
MY_OBJS := ${MY_SRCS:.cpp=.o}
MY_HEADERS := ${MY_SRCS:.cpp=.h} NameGenerator.h

TEST_SRCS := TestTools.cpp

JSON_PARSER_SRCS := $(wildcard ../jsoncpp-src-0.5.0/src/lib_json/*.cpp)
JSON_PARSER_OBJS := ${JSON_PARSER_SRCS:.cpp=.o}
INCLUDES := `pkg-config opencv --cflags --libs` -I../jsoncpp-src-0.5.0/include -I./

#TODO: it's better to link every time than to recompile every time something changes.
ImageProcessTest: tests/ImageProcessTest.test $(MY_SRCS) $(TEST_SRCS) $(MY_HEADERS) configuration.h
	@rm -rf debug_segment_images
	@mkdir debug_segment_images
	@rm -rf bubble_images
	@mkdir bubble_images
	./tests/$@.test
	
FormAlignmentTest: tests/FormAlignmentTest.test $(MY_SRCS) $(TEST_SRCS) $(MY_HEADERS) configuration.h
	@rm -rf aligned_forms
	@cp -rL form_images aligned_forms
	./tests/$@.test

#does linking
%.test: %.cpp $(MY_SRCS) $(MY_OBJS) $(JSON_PARSER_OBJS)
	g++ -g -o $@ $< $(MY_OBJS) $(JSON_PARSER_OBJS) $(INCLUDES)

#does compiling
.PRECIOUS: %.o
%.o: %.cpp
	g++ -c $< -o $@ $(INCLUDES)
	
#This will move all the relevant files into the JNI folder.
jni-transfer: $(filter-out test_%, $(MY_SRCS) $(MY_HEADERS))
	cp -i $? ../jni
	@echo "You can delete this file. It just tells make the jni-transfer target is up-to-date." > jni-transfer

clean:
	rm -rf debug_segment_images
	rm $(MY_OBJS)
	rm suite
	rm $(JSON_PARSER_OBJS)
