SRCS := formAlignment.cpp alignmentTest.cpp

run: test $(SRCS)
	./test

test: $(SRCS)
	g++ -g -o test $(SRCS) `pkg-config opencv --cflags --libs`
