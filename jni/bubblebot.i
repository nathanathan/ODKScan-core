%module bubblebot

/*
 * the java import code muse be included for the opencv jni wrappers
 * this means that the android project must reference opencv/android as a project
 * see the default.properties for how this is done
 */

%pragma(java) jniclasscode=%{
	static {
		try {
			//load the cvcamera library, make sure that libcvcamera.so is in your <project>/libs/armeabi directory
			//so that android sdk automatically installs it along with the app.
			
			//the android-opencv lib must be loaded first inorder for the cvcamera
			//lib to be found
			//check the apk generated, by opening it in an archive manager, to verify that
			//both these libraries are present

			System.loadLibrary("bubblebot");
		} catch (UnsatisfiedLinkError e) {
			//badness
			throw e;
		}
	}

%}

%include "Processor.i"
%include "MarkupForm.i"
