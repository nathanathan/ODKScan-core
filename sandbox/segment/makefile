run: segment_form
	./segment_form cropped_form.jpg
segment_form: segment_form.cpp
	g++ -o segment_form segment_form.cpp `pkg-config opencv --cflags --libs`
