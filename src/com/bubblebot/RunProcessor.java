package com.bubblebot;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.bubblebot.jni.MarkupForm;
import com.bubblebot.jni.Processor;

public class RunProcessor implements Runnable{
	
	public enum Mode {
	    LOAD, LOAD_ALIGN, PROCESS
	}
	
	private Mode mode;
	private Processor mProcessor;
	private Handler handler;
	private String photoName;
	
	public RunProcessor(Handler handler, String photoName){
		this.handler = handler;
		this.photoName = photoName;
		mProcessor = new Processor();
	}
	public void setMode(Mode m){
		mode = m;
	}
	@Override
	public void run() {
		Message msg = new Message();
		msg.arg1 = 0;
		msg.what = mode.ordinal();

		if(mode == Mode.PROCESS){
			if( mProcessor.processForm( MScanUtils.getJsonPath(photoName)) ) {
				
				MarkupForm.markupForm(  MScanUtils.getJsonPath(photoName),
										MScanUtils.getAlignedPhotoPath(photoName),
										MScanUtils.getMarkedupPhotoPath(photoName));
				msg.arg1 = 1;//indicates success
				
			}
		}
		else if(mode == Mode.LOAD){
			if( mProcessor.setForm(MScanUtils.getAlignedPhotoPath(photoName)) ){
				if(mProcessor.setTemplate( MScanUtils.appFolder + "form_templates/SIS-A01.json" )) {
					msg.arg1 = 1;//indicates success
				}
			}
		}
		else if(mode == Mode.LOAD_ALIGN){
			Log.i("mScan", "mProcessor successfully constructed");
			
			if(mProcessor.setForm(MScanUtils.getPhotoPath(photoName))) {
				Log.i("mScan","Loading: " + photoName);
				//TODO: consider making the mScan root part of the processor state.
				String[] templatePaths = { "form_templates/SIS-A01", "form_templates/UW_course_eval_A_front" };
				for(int i = 0; i < templatePaths.length; i++){
					mProcessor.loadFeatureData(MScanUtils.appFolder + templatePaths[i]);
				}
				
				int formIdx = mProcessor.detectForm();
				
				if(formIdx >= 0 && mProcessor.setTemplate(MScanUtils.appFolder + templatePaths[formIdx])) {
					Log.i("mScan","template loaded");
					if( mProcessor.alignForm(MScanUtils.getAlignedPhotoPath(photoName), formIdx) ){
						msg.arg1 = 1;//indicates success
						Log.i("mScan","aligned");
					}
				}
			}
			else {
				Log.i("mScan","FAILED TO LOAD IMAGE: " + photoName);
			}
		}
		
		handler.sendMessage(msg);
	
	}
}
