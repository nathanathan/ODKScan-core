#A very helpful source: http://stackoverflow.com/questions/5799820/makefile-and-c-project
#TEST_SRCS := $(addsuffix .cpp, $(addprefix tests/, $(TESTS)))
MY_SRCS := TestTools.cpp Processor.cpp PCA_classifier.cpp FormAlignment.cpp FileUtils.cpp Addons.cpp MarkupForm.cpp
MY_OBJS := ${MY_SRCS:.cpp=.o}
MY_HEADERS := ${MY_SRCS:.cpp=.h} NameGenerator.h

JSON_PARSER_SRCS := $(wildcard ../jsoncpp-src-0.5.0/src/lib_json/*.cpp)
JSON_PARSER_OBJS := ${JSON_PARSER_SRCS:.cpp=.o}

ALL_OBJS := $(MY_OBJS) $(JSON_PARSER_OBJS)

INCLUDES := `pkg-config opencv --cflags --libs` -I../jsoncpp-src-0.5.0/include -I./

ImageProcessTest: tests/ImageProcessTest.test 
	@rm -rf debug_segment_images
	@mkdir debug_segment_images
	@rm -rf bubble_images
	@mkdir bubble_images
	./$<
	
FormAlignmentTest: tests/FormAlignmentTest.test
	@rm -rf aligned_forms
	@cp -rL form_images aligned_forms
	./$<

EverythingTest: tests/EverythingTest.test
	@rm -rf aligned_forms
	@cp -rL form_images aligned_forms
	@rm -rf debug_segment_images
	@mkdir debug_segment_images
	@rm -rf bubble_images
	@mkdir bubble_images
	./$<

TemplateEditor: TemplateEditor.test
	@rm -rf debug_segment_images
	@mkdir debug_segment_images
	@rm -rf bubble_images
	@mkdir bubble_images
	./$<

#does linking
%.test: %.cpp $(MY_SRCS) $(ALL_OBJS) $(MY_HEADERS) configuration.h
	g++ -g -o $@ $< $(ALL_OBJS) $(INCLUDES)

#does compiling
.PRECIOUS: %.o
%.o: %.cpp $(MY_HEADERS) configuration.h
	g++ -Wall -c $< -o $@ $(INCLUDES)
	
#This will move all the relevant files into the JNI folder.
jni-transfer: $(filter-out Test%, $(MY_SRCS) $(MY_HEADERS))
	cp -i $? ../jni
	@echo "You can delete this file. It just tells make the jni-transfer target is up-to-date." > jni-transfer
.IGNORE: clean
clean:
	rm -rf debug_segment_images
	rm $(MY_OBJS)
	rm tests/*.test
	rm TemplateEditor.test
	rm $(JSON_PARSER_OBJS)
	rm jni-transfer
