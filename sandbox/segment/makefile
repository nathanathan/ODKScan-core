run: segment_form
	./segment_form cropped_form.jpg full_form.segments
segment_form: segment_form.cpp
	g++ -o segment_form segment_form.cpp `pkg-config opencv --cflags --libs`
