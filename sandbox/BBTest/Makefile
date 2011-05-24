run: BBTest-c
	./BBTest
BBTest: BBTest-c
	./BBTest
TemplMatch: TemplMatch-c
	./TemplMatch
TemplMatch2: TemplMatch2-c
	./TemplMatch2
RGChrome-c: RGChrome.cpp
	g++ -o RGChrome RGChrome.cpp `pkg-config opencv --cflags --libs`
BBTest-c: BBTest.cpp
	g++ -o BBTest BBTest.cpp `pkg-config opencv --cflags --libs`
TemplMatch-c: TemplMatch.cpp
	g++ -o TemplMatch TemplMatch.cpp `pkg-config opencv --cflags --libs`
TemplMatch2-c: TemplMatch2.cpp
	g++ -o TemplMatch2 TemplMatch2.cpp `pkg-config opencv --cflags --libs`
test_process: test_ImageProcessing.cc ImageProcessing.cc
	g++ -g -o test_process test_ImageProcessing.cc ImageProcessing.cc `pkg-config opencv --cflags --libs`
