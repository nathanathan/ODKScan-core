SRCS := test_ImageProcessing.cc ImageProcessing.cc PCA_classifier.cpp formAlignment.cpp

run: suite $(SRCS)
	@rm -rf debug_segment_images
	@mkdir debug_segment_images
	./suite

suite: $(SRCS)
	g++ -g -o suite $(SRCS) `pkg-config opencv --cflags --libs`
	
clean:
	rm -rf debug_segment_images
	rm suite
