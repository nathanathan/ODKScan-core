run: checkBubbles-c
	./checkBubbles
checkBubbles-c: checkBubbles.cpp
	g++ -o checkBubbles checkBubbles.cpp `pkg-config opencv --cflags --libs`
