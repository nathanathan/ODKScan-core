ODKScan core
============

Code previously available at https://github.com/villagereach/mScan/tree/master/TestSuite

Setup
=====

1. Install OpenCV

Building
========

	make Experiment TEMPLATE=template_path INPUT_FOLDER=input_folder OUTPUT_FOLDER=output_folder

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

