TestSuite
=========
This is a testing suite for the form scanning procedure.
None of the tests work out of the box.
You will need to have a form image and form template, and to specify their respective paths in the code for the test you are running.
In addition you will need to provide your own form images in a "form_images" folder.

Source file information
-----------------------

	Core Files:

* Processor (.h|.cpp) -- Handles JSON parsing and provides an interface.
* Aligner (.h|.cpp) -- Class for detecting and aligning forms.
* SegmentAligner (.h|.cpp) -- Contains code for aligning segments.
* AlignmentUtils (.h|.cpp) -- Functions useful for doing alignment related things like dealing with quads.
* PCA_classifier (.h|.cpp) -- Classifies bubbles using OpenCV's SVM with PCA. Contains code for reading training data from directories and refining bubble positions.
* Addons (.h|.cpp) -- Misc. utility functions that are useful in multiple places.
* NameGenerator (.h|.cpp) --A utility for generating unique names.
* FileUtils (.h|.cpp) --A utility for crawling file trees and returning all the file names.
* MarkupForm (.h|.cpp) --Contains a function for marking up a form using a JSON form template or bubble vals file.

	Special note:
Core Files can be swapped with those in the jni directory.
The command "make jni-transfer" will do this swapping for you.

	Test Suite Specific Files:

* TestTools.cpp --Code for use in test programs. Prints out stats.
* ROC_plot_script --Plots ROC curves using the resultant data from tests with varying parameters. In order to generate plots you will have to install [R] the statisical programming language.
* TemplateEditor.cpp --This is some code that might be useful for editing JSON templates. It's pretty messy so I don't know if it will be of much use to anyone but me.
* configuration.h --The jni folder has a configuration file as well, but they are *not* interchangable. This file is used to define macros that enable test suite specific funtionality.(For example outputting debug text/images).
	tests:
	
* Experiment.cpp --This is the most comprehensive test. You can run it on a collection of images in some_folder with the command:

make Experiment EXPERIMENT_FOLDER=some_folder

(It might have some parameters that are hard-coded for a specific form I've been using not publicly available)

* CourseEvalTest.cpp --This test uses the course eval form included in the repository so it should work out of the box. (However, at the moment I'm writing this, I'm pretty sure it doesn't)
							
	calibration:
	
* calibration.cpp --Calibration code from OpenCV samples. Calibration can provide substancial improvement to all steps in the pipline.

The tests folder contains the actual test code.
Each cpp file in there should have a make corresponding make target you can use.
If you should decide to modify any of that code with changes to I/O it's important to note that it all gets run from the TestSuite directory.
More to come as the suite develops...
