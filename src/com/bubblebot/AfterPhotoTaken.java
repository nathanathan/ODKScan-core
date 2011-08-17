package com.bubblebot;


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
import android.widget.RelativeLayout;

public class AfterPhotoTaken extends Activity {
	
	private ProgressDialog pd;
    private Button retake;
    private Button process;
    
	String photoName;
	RunProcessor runProcessor;

    @Override
	protected void onCreate(Bundle savedInstanceState) {
    	super.onCreate(savedInstanceState);
    	
    	Log.i("mScan", "After Photo taken");
    	
		setContentView(R.layout.after_photo_taken);
		
		pd = new ProgressDialog(this);
		pd.setCancelable(false);
		pd.setTitle("Processing...");
		
		Bundle extras = getIntent().getExtras();
		if (extras == null) return;
		
		photoName = extras.getString("photoName");
		runProcessor = new RunProcessor(handler, photoName);
 
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
				runProcessor.setMode(RunProcessor.PROCESS_MODE);
				Thread thread = new Thread(runProcessor);
				thread.setPriority(Thread.MAX_PRIORITY);
				thread.start();
			}
		});
		
		pd.setMessage("Aligning Image");
		pd.show();
		runProcessor.setMode(RunProcessor.ALIGNMENT_MODE);
		Thread thread = new Thread( runProcessor );
		thread.setPriority(Thread.MAX_PRIORITY);
		thread.start();
	}
	@Override
	public void onBackPressed() {
		Intent intent = new Intent(getApplication(), BubbleBot.class);
		startActivity(intent);
	}
    private Handler handler = new Handler() {
            @Override
            
            public void handleMessage(Message msg) {
            	
            	pd.dismiss();
            	
            	if(msg.what == RunProcessor.PROCESS_MODE){
            		if ( msg.arg1 == 1 ) {
	            		Intent intent = new Intent(getApplication(), DisplayProcessedForm.class);
	                    intent.putExtra("photoName", photoName);
	                    startActivity(intent);
            		}
            	}
            	else if(msg.what == RunProcessor.ALIGNMENT_MODE){
	        		if ( msg.arg1 == 1 ) {
	        			MScanUtils.displayImageInWebView((WebView)findViewById(R.id.webview),
	        														MScanUtils.getAlignedPhotoPath(photoName));
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