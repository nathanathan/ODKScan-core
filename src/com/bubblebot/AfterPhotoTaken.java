package com.bubblebot;


import com.bubblebot.jni.Processor;

import android.app.ProgressDialog;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.webkit.WebView;
import android.widget.Button;
import android.widget.RelativeLayout;

public class AfterPhotoTaken extends MScanExtendedActivity implements Runnable {
	
	private ProgressDialog pd;
    private Button retake;
    private Button process;
    
	String photoName;
	
	private boolean detectResult;

    @Override
	protected void onCreate(Bundle savedInstanceState) {
    	super.onCreate(savedInstanceState);
    	
    	Log.i("mScan", "After Photo taken");
    	
		setContentView(R.layout.after_photo_taken);
		
		pd = new ProgressDialog(this);
		pd.setCancelable(false);
		pd.setTitle("Processing...");
		
		Bundle extras = getIntent().getExtras();
		if (extras != null) {
			photoName = extras.getString("photoName");
		}
 
		// If user chooses to retake the photo, start the BubbleCollect activity
		retake = (Button) findViewById(R.id.retake_button);
		retake.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				Intent intent = new Intent(getApplication(), BubbleCollect2.class);
				startActivity(intent);
			}
		});
		// If user chooses to process the form, start the BubbleProcess activity
		process = (Button) findViewById(R.id.process_button);
		process.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				// Extract the image filename from the activity parameters
				Log.i("mScan","PICTURE NAME: " + photoName);
				pd.setMessage("Did you know... \n " +
						"The MMR measles (sarampo) vaccination is generally not given to children younger " +
        				"than 18 months because they usually retain antimeasles immunoglobulins (antibodies) " +
        				"transmitted from the mother during pregnancy.");
				pd.show();

				//It is possible to run this backgrounded with low priority to make things seems faster...
				Thread thread = new Thread(new RunProcessor(handler,
															getAlignedPhotoPath(photoName),
															getResources().getString(com.bubblebot.R.string.templatePath),
															getAppFolder() + "training_examples",
															getJsonPath(photoName),
															getMarkedupPhotoPath(photoName)));
				thread.setPriority(Thread.MAX_PRIORITY);
				thread.start();
			}
		});
		
		pd.setMessage("Aligning Image");
		pd.show();
		Thread thread = new Thread(this);
		thread.setPriority(Thread.MAX_PRIORITY);
		thread.start();
	}
	@Override
	public void onBackPressed() {
		Intent intent = new Intent(getApplication(), BubbleBot.class);
		startActivity(intent);
	}
    public void run() {
		Processor mProcessor = new Processor();
		
		Log.i("mScan", "mProcessor successfully constructed");
		
		String templatePath = getResources().getString(com.bubblebot.R.string.templatePath);
		
		if(mProcessor.loadForm(getPhotoPath(photoName))) {
			Log.i("mScan","Loading: " + photoName);
			if(mProcessor.loadTemplate(templatePath)) {
				Log.i("mScan","template loaded");
				detectResult = mProcessor.alignForm(getAlignedPhotoPath(photoName));
				Log.i("mScan","aligned");
			}
			else {
				Log.i("mScan","FAILED TO LOAD TEMPLATE: " + templatePath);
			}
		}
		else {
			Log.i("mScan","FAILED TO LOAD IMAGE: " + photoName);
		}
    	handler.sendEmptyMessage(0);
    }
    private Handler handler = new Handler() {
            @Override
            
            public void handleMessage(Message msg) {
            	
            	pd.dismiss();
            	
            	if(msg.what == RunProcessor.whatId){
            		Intent intent = new Intent(getApplication(), DisplayProcessedForm.class);
                    intent.putExtra("photoName", photoName);
                    startActivity(intent);
            	}
            	else{
	        		if ( detectResult ) {
	        			MScanUtils.displayImageInWebView((WebView)findViewById(R.id.webview), getAlignedPhotoPath(photoName));
	        			process.setEnabled(true);
	        		}
	        		else {
	        			RelativeLayout failureMessage = (RelativeLayout) findViewById(R.id.failureMessage);
	        			failureMessage.setVisibility(View.VISIBLE);
	        			process.setEnabled(false);
	        		}
            	}
            }
    };
}