SRCS := test_ImageProcessing.cpp ImageProcessing.cpp PCA_classifier.cpp FormAlignment.cpp FileUtils.cpp

run: suite $(SRCS) NameGenerator.h configuration.h
	@rm -rf debug_segment_images
	@mkdir debug_segment_images
	@rm -rf bubble_images
	@mkdir bubble_images
	./suite

suite: $(SRCS)
	g++ -g -o suite $(SRCS) `pkg-config opencv --cflags --libs`
	
clean:
	rm -rf debug_segment_images
	rm suite
