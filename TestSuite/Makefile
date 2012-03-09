CORE_SRCS := $(wildcard core/*.cpp)
CORE_HEADERS := $(wildcard core/*.h)
CORE_OBJS := ${CORE_SRCS:.cpp=.o}

JSON_PARSER_SRCS := $(wildcard ../jsoncpp-src-0.5.0/src/lib_json/*.cpp)
JSON_PARSER_OBJS := ${JSON_PARSER_SRCS:.cpp=.o}

ALL_SRCS := $(CORE_SRCS) StatCollector.cpp
ALL_HEADERS := $(CORE_HEADERS) StatCollector.h configuration.h
ALL_OBJS := $(CORE_OBJS) StatCollector.o $(JSON_PARSER_OBJS)

#The OPENCV_INCLUDES will probably need to be adjusted if you aren't running this on a linux system.
#Even then, pkg-config might not be set up.
#Also, you might need to add this to your .bashrc
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
OPENCV_INCLUDES := `pkg-config opencv --cflags --libs`

INCLUDES := $(OPENCV_INCLUDES) -I../jsoncpp-src-0.5.0/include -I./core -I./

CourseEvalTest: tests/CourseEvalTest.test
	@rm -rf debug_segment_images
	@mkdir debug_segment_images
	@rm -rf bubble_images
	@mkdir bubble_images
	./$<

#The experiment target runs Experiment.cpp on the specified folder.
#Use the following syntax to specify the folder:
#make Experiment INPUT_FOLDER=experiment_1
#If no folder is specified it will default to experiment_1
ifndef $(INPUT_FOLDER)
	INPUT_FOLDER := experiment_1
endif
Experiment: tests/Experiment.test
	@rm -rf aligned_forms/$(INPUT_FOLDER)
	@mkdir aligned_forms/$(INPUT_FOLDER)
	@rm -rf debug_segment_images
	@mkdir debug_segment_images
	@rm -rf bubble_images
	@mkdir bubble_images
	./$< $(INPUT_FOLDER)
#Remove the features file so I dont accidentally put in in the apk (it's too big)
	@rm form_templates/SIS-A01.yml

ifndef $(TEMPLATE)
	TEMPLATE := checkbox_form
endif
Checkbox: tests/Checkbox.test
	@rm -rf aligned_forms/$(INPUT_FOLDER)
	@mkdir aligned_forms/$(INPUT_FOLDER)
	@rm -rf debug_segment_images
	@mkdir debug_segment_images
	@rm -rf bubble_images
	@mkdir bubble_images
	./$< $(INPUT_FOLDER) $(TEMPLATE)

TemplateGenerator: TemplateGenerator.test
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
#http://www.gnu.org/s/hello/manual/make/Conditional-Syntax.html#Conditional-Syntax
#http://en.wikipedia.org/wiki/Makefile
#http://stackoverflow.com/questions/2214575/passing-arguments-to-make-run
