CORE_SRCS := $(wildcard core/*.cpp)
CORE_HEADERS := $(wildcard core/*.h)
CORE_OBJS := ${CORE_SRCS:.cpp=.o}

JSON_PARSER_SRCS := $(wildcard ../jsoncpp-src-0.5.0/src/lib_json/*.cpp)
JSON_PARSER_OBJS := ${JSON_PARSER_SRCS:.cpp=.o}

ALL_SRCS := $(CORE_SRCS) TestTools.cpp
ALL_HEADERS := $(CORE_HEADERS) TestTools.h configuration.h
ALL_OBJS := $(CORE_OBJS) TestTools.o $(JSON_PARSER_OBJS)

INCLUDES := `pkg-config opencv --cflags --libs` -I../jsoncpp-src-0.5.0/include -I./core -I./

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
%.test: %.cpp $(ALL_SRCS) $(ALL_OBJS) $(ALL_HEADERS)
	g++ -g -o $@ $< $(ALL_OBJS) $(INCLUDES)

#does compiling
.PRECIOUS: %.o
%.o: %.cpp $(ALL_HEADERS)
	g++ -Wall -c $< -o $@ $(INCLUDES)
	
#This will move all the relevant files into the JNI folder.
jni-transfer: $(CORE_SRCS) $(CORE_HEADERS)
	cp -i $? ../jni/core
	@echo "You can delete this file. It just tells make the jni-transfer target is up-to-date." > jni-transfer
.IGNORE: clean
clean:
	rm -rf debug_segment_images
	rm $(ALL_OBJS)
	rm tests/*.test
	rm TemplateEditor.test
	rm jni-transfer

#Some helpful sources I used to make this makefile:
#http://stackoverflow.com/questions/5799820/makefile-and-c-project
#http://www.gnu.org/s/hello/manual/make/Special-Targets.html
#http://en.wikipedia.org/wiki/Makefile
