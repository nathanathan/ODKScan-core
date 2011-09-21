INCLUDES := `pkg-config opencv --cflags --libs`

run: calibration
	./calibration -w 6 -h 9 -s 0.025 -o camera.yml -op -oe -su droid_image_list.xml

calibration: calibration.cpp
	g++ -Wall -g $< -o $@ $(INCLUDES)
