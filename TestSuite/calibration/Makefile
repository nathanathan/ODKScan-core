INCLUDES := `pkg-config opencv --cflags --libs`

calibrate-droid: calibration
	./$< -w 6 -h 9 -s 0.025 -o droid_camera.yml -op -oe -su droid_image_list.xml
	cp droid_camera.yml ../../assets/camera.yml

calibrate-nexus: calibration
	./$< -w 6 -h 9 -s 0.025 -o nexus_camera.yml -op -oe -su nexus_image_list.xml
	cp nexus_camera.yml ../../assets/camera.yml

calibration: calibration.cpp
	g++ -Wall -g $< -o $@ $(INCLUDES)
