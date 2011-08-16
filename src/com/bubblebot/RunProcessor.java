package com.bubblebot;

import android.os.Handler;
import android.os.Message;

import com.bubblebot.jni.MarkupForm;
import com.bubblebot.jni.Processor;

public class RunProcessor implements Runnable{
	public static final int whatId = 1;
	private Handler handler;
	private String alignedPhotoPath;
	private String templatePath;
	private String trainingExamples;
	private String jsonPath;
	private String markedupPhotoPath;
	
	public RunProcessor(Handler handler, String alignedPhotoPath,
						String templatePath, String trainingExamples,
						String jsonPath, String markedupPhotoPath){
		this.handler = handler;
		this.alignedPhotoPath = alignedPhotoPath;
		this.templatePath = templatePath;
		this.trainingExamples = trainingExamples;
		this.jsonPath = jsonPath;
		this.markedupPhotoPath = markedupPhotoPath;
	}
	@Override
	public void run() {
		Message msg = new Message();
		msg.what = whatId;
		
		Processor mProcessor = new Processor();
		//I hope these args get evaluated in order... otherwise I'm in for some trouble.
		if( mProcessor.loadForm( alignedPhotoPath ) &&
			mProcessor.loadTemplate( templatePath ) &&
			mProcessor.trainClassifier(trainingExamples) &&
			mProcessor.processForm(jsonPath) ) {
			
			(new MarkupForm()).markupForm(jsonPath, alignedPhotoPath, markedupPhotoPath);
			msg.arg1 = 1;
			
		}
		handler.sendMessage(msg);
	}
}
