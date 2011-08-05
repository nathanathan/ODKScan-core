package com.bubblebot;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.FrameLayout;

import com.bubblebot.jni.Processor;
import com.bubblebot.jni.MarkupForm;

public class BubbleProcess extends Activity implements Runnable {
	 
    private ProgressDialog pd;
    private String pictureName;
    
    @Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		// Extract the image filename from the activity parameters
		Bundle extras = getIntent().getExtras(); 
		if(extras !=null)
	    {
	   	   pictureName = extras.getString("file");
	   	   //pictureName = (pictureName.substring(0,pictureName.length()-4));
	    }
		
        pd = ProgressDialog.show(this, "Working..", "Processing Form", true,
                false);

		Thread thread = new Thread(this);
		thread.setPriority(Thread.MAX_PRIORITY);
		thread.start();
		
		//Not sure what this does exactly
		FrameLayout frame = new FrameLayout(this);
		setContentView(frame);
	}

    public void run() {
    	Log.d("Nathan","PICTURE NAME: " + pictureName);
		Processor mProcessor = new Processor();
		//I hope these args get evaluated in order... otherwise I'm in for some trouble.
		if( mProcessor.loadForm(pictureName) &&
			mProcessor.loadTemplate(getResources().getString(com.bubblebot.R.string.templatePath)) &&
			mProcessor.trainClassifier("/sdcard/mScan/training_examples") &&
			mProcessor.processForm("sdcard/BubbleBot/output.json") ){
			(new MarkupForm()).markupForm("sdcard/BubbleBot/output.json", pictureName, "sdcard/BubbleBot/markedup.jpg");
		}
    	handler.sendEmptyMessage(0);
    }

    private Handler handler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                    pd.dismiss();
                    Intent intent = new Intent(getApplication(), DisplayProcessedForm.class);
                    startActivity(intent);
            }
    };
}