package com.bubblebot;

import android.app.ProgressDialog;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.FrameLayout;

import com.bubblebot.jni.Processor;
import com.bubblebot.jni.MarkupForm;

public class BubbleProcess extends MScanExtendedActivity implements Runnable {
	 
    private ProgressDialog pd;
    String photoName;
    
    @Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
        pd = ProgressDialog.show(this, "Processing Form", "Did you know... \n " +
        		"The MMR measles (sarampo) vaccination is generally not given to children younger " +
        		"than 18 months because they usually retain antimeasles immunoglobulins (antibodies) " +
        		"transmitted from the mother during pregnancy.", true,
                false);

		Thread thread = new Thread(this);
		thread.setPriority(Thread.MAX_PRIORITY);
		thread.start();
		
		//Not sure I need this
		FrameLayout frame = new FrameLayout(this);
		setContentView(frame);
	}

    public void run() {
		// Extract the image filename from the activity parameters
		Bundle extras = getIntent().getExtras(); 
		if(extras == null) return;
   	    photoName = extras.getString("photoName");
    	Log.d("mScan","PICTURE NAME: " + photoName);
    	
		Processor mProcessor = new Processor();
		//I hope these args get evaluated in order... otherwise I'm in for some trouble.
		if( mProcessor.loadForm(getAlignedPhotoPath(photoName)) &&
			mProcessor.loadTemplate(getResources().getString(com.bubblebot.R.string.templatePath)) &&
			mProcessor.trainClassifier(getAppFolder() + "training_examples") &&
			mProcessor.processForm(getJsonPath(photoName)) ) {
			
			(new MarkupForm()).markupForm(  getJsonPath(photoName), getAlignedPhotoPath(photoName),
					 						getMarkedupPhotoPath(photoName));
			
		}
    	handler.sendEmptyMessage(0);
    }

    private Handler handler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                    pd.dismiss();
                    Intent intent = new Intent(getApplication(), DisplayProcessedForm.class);
                    intent.putExtra("photoName", photoName);
                    startActivity(intent);
            }
    };
}