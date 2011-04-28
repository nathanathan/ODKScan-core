run: segment_form
	./segment_form full_form.jpg full_form.segments
segment_form: segment_form.cpp
	g++ -o segment_form segment_form.cpp `pkg-config opencv --cflags --libs`
