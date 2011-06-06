run: makeExampleStrip-c
	./makeExampleStrip
 makeExampleStrip-c:  makeExampleStrip.cpp FileUtils.cc
	g++ -o  makeExampleStrip makeExampleStrip.cpp FileUtils.cc `pkg-config opencv --cflags --libs`
