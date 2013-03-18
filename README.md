ODKScan core
============

ODKScan is a tool for extracting information from images of paper forms.
It does optical mark recognition (so it can handle bubble/checkbox forms).
This repo contains the backend code that is shared between
[the ODKScan web-app](https://github.com/nathanathan/ODKScan_webapp)
and [the ODKScan Android app](https://github.com/villagereach/mScan).

Setup
=====

1. Install OpenCV

	Install guide: http://opencv.willowgarage.com/wiki/InstallGuide
	
	(If you run into any compile errors, disabling some of the features with cmake flags has helped me a couple times.)

2. Set OPENCV_INCLUDES in the makefile. If you're using ubuntu you just need to do this:

```
#install pkg-config:
apt-get install pkg-config
#add this to your .bashrc:
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
```

Command Line Usage:
===================

```
make
./ODKScan.run assets/form_templates/example example_input/img0.jpg output/img0
```

How to modify the form alignment code:
======================================

Aligner.cpp does image alignment using the OpenCV 2D features framework.
This framework consistes of modular components for detecting, describing and matching features respectively.
(The OpenCV Docs have some tutorials on it available [here](http://docs.opencv.org/doc/tutorials/features2d/table_of_content_features2d/table_of_content_features2d.html#table-of-content-feature2d).)
These compontents are set and configured in the Aliger class's initialization function.

OpenCV provides a number of different classes to choose between for each feature module,
in addition to a number of different ways to parameterize them.
Figuring out which works the best has involved a lot trial and error for me.
Although, certain types of features are designed to make trade-offs (e.g. speed vs. scale/rotation invariance).
Defining the SHOW_MATCHES_WINDOW constant will display a window that makes it possible
to see how various detector/descriptor/matcher settings fare.

Finally, Aligner.cpp also globally transforms the image by computing a homography from the matched features.
Using region based alignment instead may be been better for dealing with deformations.

Source file information
=======================

* Processor (.h|.cpp) -- Handles JSON parsing and provides an interface.
* Aligner (.h|.cpp) -- Class for detecting and aligning forms.
* SegmentAligner (.h|.cpp) -- Contains code for aligning segments.
* AlignmentUtils (.h|.cpp) -- Functions useful for doing alignment related things like dealing with quads.
* PCA_classifier (.h|.cpp) -- Classifies bubbles using OpenCV's SVM with PCA. Contains code for reading training data from directories and refining bubble positions.
* Addons (.h|.cpp) -- Misc. utility functions that are useful in multiple places.
* FileUtils (.h|.cpp) -- A utility for crawling file trees and returning all the file names.
* MarkupForm (.h|.cpp) -- Contains a function for marking up a form using a JSON form template or bubble vals file.

Code previously available at https://github.com/villagereach/mScan/tree/master/TestSuite
