run: suite
	rm -rf debug_segment_images
	mkdir debug_segment_images
	./suite

suite: test_ImageProcessing.cc ImageProcessing.cc
	g++ -g -o suite test_ImageProcessing.cc ImageProcessing.cc `pkg-config opencv --cflags --libs`
	
clean:
	rm -f marked_*
	rm suite
