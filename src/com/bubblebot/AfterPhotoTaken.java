package com.bubblebot;


import java.util.Date;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.webkit.WebView;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;

/*
 * This activity runs the form processor and provides user feedback
 * displaying progress dialogs and the alignment results.
 */
public class AfterPhotoTaken extends Activity {
	
    private boolean aligned = false;
    private String photoName;
    private RunProcessor runProcessor;
    private ProgressDialog pd;
    private Button processButton;
    private LinearLayout content;

    private long startTime;

    @Override
	protected void onCreate(Bundle savedInstanceState) {
    	super.onCreate(savedInstanceState);
    	
    	Log.i("mScan", "After Photo taken");
    	
    	setContentView(R.layout.after_photo_taken);
    	
    	content = (LinearLayout) findViewById(R.id.myLinearLayout);
    	
		Bundle extras = getIntent().getExtras();
		if (extras == null) {
			Log.i("mScan","extras == null");
			//This might happen if we use back to reach this activity from the camera activity.
			content.setVisibility(View.VISIBLE);
			return;
		}
		
		photoName = extras.getString("photoName");
		
		runProcessor = new RunProcessor(handler, photoName);
		
		pd = new ProgressDialog(this);
		pd.setCancelable(false);
		//pd.setTitle("Processing...");
		pd.setMessage("Aligning Image");
		pd.show();
		
		Button retake = (Button) findViewById(R.id.retake_button);
		retake.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				Intent intent = new Intent(getApplication(), BubbleCollect2.class);
				startActivity(intent);
			}
		});
		// If user chooses to process the form, start the BubbleProcess activity
		processButton = (Button) findViewById(R.id.process_button);
		processButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				Log.i("mScan","PICTURE NAME: " + photoName);
				pd.setMessage("Did you know... \n " +
						"The MMR measles (sarampo) vaccination is generally not given to children younger " +
	    				"than 18 months because they usually retain antimeasles immunoglobulins (antibodies) " +
	    				"transmitted from the mother during pregnancy.");
				pd.show();
				//It is possible to run this backgrounded with low priority to make things seems faster...
				runProcessor.setMode(RunProcessor.Mode.PROCESS);
				startTime = new Date().getTime();
				Thread thread = new Thread(runProcessor);
				thread.setPriority(Thread.MAX_PRIORITY);
				thread.start();
			}
		});
		if( extras.getBoolean("preAligned") ){
			runProcessor.setMode(RunProcessor.Mode.LOAD);
		}
		else{
			runProcessor.setMode(RunProcessor.Mode.LOAD_ALIGN);
		}
		startTime = new Date().getTime();
		Thread thread = new Thread( runProcessor );
		thread.setPriority(Thread.MAX_PRIORITY);
		thread.start();
	}
    private Handler handler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
            	
            	pd.dismiss();
            	
            	RunProcessor.Mode mode = RunProcessor.Mode.values()[msg.what];
            	
            	double timeTaken = (double) (new Date().getTime() - startTime) / 1000;
    			Log.i("mScan", "Mode: " + mode + "/n" + "Time taken:"
    							+ String.format("%.2f", timeTaken));
    			
            	if(mode == RunProcessor.Mode.PROCESS){
            		if ( msg.arg1 == 1 ) {
	            		Intent intent = new Intent(getApplication(), DisplayProcessedForm.class);
	                    intent.putExtra("photoName", photoName);
	                    startActivity(intent);
            		}
            	}
            	else if(mode == RunProcessor.Mode.LOAD_ALIGN ||
            			mode == RunProcessor.Mode.LOAD){
            		aligned = (msg.arg1 == 1);
	        		if ( aligned ) {
	        			MScanUtils.displayImageInWebView((WebView)findViewById(R.id.webview),
	        														MScanUtils.getAlignedPhotoPath(photoName));
	        		}
	        		else {
	        			RelativeLayout failureMessage = (RelativeLayout) findViewById(R.id.failureMessage);
	        			failureMessage.setVisibility(View.VISIBLE);
	        		}
	        		content.setVisibility(View.VISIBLE);
	        		processButton.setEnabled(aligned);
            	}
            }
    };
}