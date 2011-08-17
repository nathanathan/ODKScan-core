package com.bubblebot;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.bubblebot.jni.MarkupForm;
import com.bubblebot.jni.Processor;

public class RunProcessor implements Runnable{
	
	public static final int ALIGNMENT_MODE = 1;
	public static final int PROCESS_MODE = 2;
	
	private int mode;
	Processor mProcessor;
	private Handler handler;
	private String photoName;
	
	public RunProcessor(Handler handler, String photoName){
		this.handler = handler;
		this.photoName = photoName;
		mProcessor = new Processor();
	}
	public void setMode(int m){
		mode = m;
	}
	@Override
	public void run() {
		Message msg = new Message();
		msg.arg1 = 0;
		msg.what = mode;
		if(mode == PROCESS_MODE){
			//I hope these args get evaluated in order... otherwise I'm in for some trouble.
			if( mProcessor.trainClassifier( MScanUtils.appFolder + MScanUtils.trainingExampleDir ) &&
				mProcessor.processForm( MScanUtils.getJsonPath(photoName)) ) {
				
				(new MarkupForm()).markupForm(  MScanUtils.getJsonPath(photoName),
												MScanUtils.getAlignedPhotoPath(photoName),
												MScanUtils.getMarkedupPhotoPath(photoName));
				msg.arg1 = 1;//indicates success
				
			}
		}
		else if(mode == ALIGNMENT_MODE){
			Log.i("mScan", "mProcessor successfully constructed");
			
			if(mProcessor.loadForm(MScanUtils.getPhotoPath(photoName))) {
				Log.i("mScan","Loading: " + photoName);
				//TODO: having to specify the template path will be phased out so this call won't be needed.
				if(mProcessor.loadTemplate( MScanUtils.appFolder + "form_templates/SIS-A01.json" )) {
					Log.i("mScan","template loaded");
					if( mProcessor.alignForm(MScanUtils.getAlignedPhotoPath(photoName)) ){
						msg.arg1 = 1;//indicates success
						Log.i("mScan","aligned");
					}
				}
				else {
					Log.i("mScan","FAILED TO LOAD TEMPLATE: " + MScanUtils.appFolder + "form_templates/SIS-A01.json");
				}
			}
			else {
				Log.i("mScan","FAILED TO LOAD IMAGE: " + photoName);
			}
		}
		
		handler.sendMessage(msg);
	
	}
}
