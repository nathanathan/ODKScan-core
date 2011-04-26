run: json_form
	./json_form test1.jpg test1.json
json_form: json_form.cpp
	g++ -o json_form json_form.cpp `pkg-config opencv --cflags --libs`
