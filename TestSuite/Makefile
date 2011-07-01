#A very helpful source: http://stackoverflow.com/questions/5799820/makefile-and-c-project
MY_SRCS := test_ImageProcessing.cpp Processor.cpp PCA_classifier.cpp FormAlignment.cpp FileUtils.cpp
MY_OBJS := ${MY_SRCS:.cpp=.o}
JSON_PARSER_SRCS := $(wildcard ../jsoncpp-src-0.5.0/src/lib_json/*.cpp)
JSON_PARSER_OBJS := ${JSON_PARSER_SRCS:.cpp=.o}

INCLUDES := `pkg-config opencv --cflags --libs` -I../jsoncpp-src-0.5.0/include

run: suite $(SRCS) NameGenerator.h configuration.h
#In the future this directory stuff should be done in code.
	@rm -rf debug_segment_images
	@mkdir debug_segment_images
	@rm -rf bubble_images
	@mkdir bubble_images
	./suite

suite: $(SRCS) $(MY_OBJS) $(JSON_PARSER_OBJS)
	g++ -g -o suite $(MY_OBJS) $(JSON_PARSER_OBJS) $(INCLUDES)

%.o: %.cpp
	g++ -c $< -o $@ $(INCLUDES)
	
clean:
	rm -rf debug_segment_images
	rm $(MY_OBJS)
	rm suite
	rm $(JSON_PARSER_OBJS)
