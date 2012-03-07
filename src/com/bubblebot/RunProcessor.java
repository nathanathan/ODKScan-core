package com.bubblebot;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.bubblebot.jni.MarkupForm;
import com.bubblebot.jni.Processor;
/*
 * RunProcessor is used to run the image processing algorithms on a separate thread.
 */
public class RunProcessor implements Runnable {
	
	public enum Mode {
	    LOAD, LOAD_ALIGN, PROCESS
	}
	
	private String[] templatePaths;
	private boolean undistort;
	
	private Mode mode;
	private Processor mProcessor;
	private Handler handler;
	private String photoName;

	public RunProcessor(Handler handler, String photoName, String[] templatePaths, boolean undistort) {
		this.handler = handler;
		this.photoName = photoName;
		this.templatePaths = templatePaths;
		this.undistort = undistort;
		mProcessor = new Processor(MScanUtils.appFolder);
	}
	/**
	 * This method sets the mode the processor is to run in.
	 * @param mode
	 */
	public void setMode(Mode mode) {
		this.mode = mode;
	}
	//@Override
	public void run() {
		
		Message msg = new Message();
		msg.arg1 = 0;//I'm using arg1 as a success indicator. A value of 1 means success.
		msg.what = mode.ordinal();

		if(mode == Mode.PROCESS) {
			if( mProcessor.processForm( MScanUtils.getJsonPath(photoName)) ) {
				
				MarkupForm.markupForm(  MScanUtils.getJsonPath(photoName),
										MScanUtils.getAlignedPhotoPath(photoName),
										MScanUtils.getMarkedupPhotoPath(photoName));
				msg.arg1 = 1;
			}
		}
		else if(mode == Mode.LOAD) {
			//TODO: Need to figure out a way to keep track of the template that was used to align them image...
			//		Could redetect.
			if( mProcessor.loadFormImage(MScanUtils.getAlignedPhotoPath(photoName), false) ) {
				if(mProcessor.setTemplate( MScanUtils.appFolder + "form_templates/SIS-A01.json" )) {
					msg.arg1 = 1;
				}
			}
		}
		else if(mode == Mode.LOAD_ALIGN) {
			
			if(mProcessor.loadFormImage(MScanUtils.getPhotoPath(photoName), undistort)) {
				Log.i("mScan","Loading: " + photoName);
				
				int formIdx = 0;
				
				for(int i = 0; i < templatePaths.length; i++){ 
					//Log.i("mScan", "loadingFD: " + MScanUtils.appFolder + templatePaths[i]);
					mProcessor.loadFeatureData(MScanUtils.appFolder + templatePaths[i]);
				}
				
				if(templatePaths.length > 1) {
					formIdx = mProcessor.detectForm();
				}
				
				if(formIdx >= 0) {
					if(mProcessor.setTemplate(MScanUtils.appFolder + templatePaths[formIdx])) {
						Log.i("mScan","template loaded");
						if( mProcessor.alignForm(MScanUtils.getAlignedPhotoPath(photoName), formIdx) ) {
							msg.arg1 = 1;//indicates success
							Log.i("mScan","aligned");
						}
					}
					else {
						Log.i("mScan","Faled to set template.");
					}
				}
				else {
					Log.i("mScan","Faled to detect form.");
				}
				//Indicate which template was used.
				msg.arg2 = formIdx;
			}
			else {
				Log.i("mScan","Faled to load image: " + photoName);
			}
		}
		handler.sendMessage(msg);
	
	}
}
