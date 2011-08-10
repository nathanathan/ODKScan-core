package com.bubblebot;

import java.io.File;
import java.util.Date;

import com.bubblebot.jni.Processor;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.widget.Button;
import android.widget.RelativeLayout;

public class AfterPhotoTaken extends Activity implements Runnable {
	
    private ProgressDialog pd;
    private Button retake;
    private Button process;
    private String capturedDir;
	private String alignmentOutputImage;
	private String photoPath;
	private String processedDir = "/sdcard/mScan/processedImages/";
	
	private boolean detectResult;
	
	private boolean debugMode;
	
    @Override
	protected void onCreate(Bundle savedInstanceState) {
    	super.onCreate(savedInstanceState);
		setContentView(R.layout.after_photo_taken);
		
		Bundle extras = getIntent().getExtras();
		if (extras != null) {
			photoPath = extras.getString("file");
			debugMode = extras.getBoolean("debugMode");
		}

		//Watch out, the string resource does not update unless the project is cleaned.
		//Also note that the app_folder is hard coded in the Native Previewer so if the
		//resource changes that needs to change too.
		capturedDir = getResources().getString(R.string.app_folder) + "capturedImages/";
		alignmentOutputImage = getResources().getString(R.string.app_folder) + "preview.jpg";

		// If user chose to retake the photo, delete the last photo taken
		// and start the BubbleCollect activity
		retake = (Button) findViewById(R.id.retake_button);
		retake.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				deletePhotoAndDataFile();
				Intent intent = new Intent(getApplication(), BubbleCollect.class);
				startActivity(intent);
				finish();
			}
		});

		// If user chose to process the form, start the
		// BubbleProcess activity
		process = (Button) findViewById(R.id.process_button);
		process.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				// Start process form algorithm
				Intent intent = new Intent(getApplication(),
						BubbleProcess.class);
				intent.putExtra("file", "/sdcard/BubbleBot/preview.jpg");
				//intent.putExtra("processor", mProcessor);
				startActivity(intent);
				finish();
			}
		});
		process.setEnabled(false);
		
        pd = ProgressDialog.show(this, "Working...", "Aligning Image", true, false);
        
		Thread thread = new Thread(this);
		
		thread.setPriority(Thread.MAX_PRIORITY);
		
		thread.start();
	}
	@Override
	public void onBackPressed() {
		super.onBackPressed();
		//I'm not so sure I should do this
		if(!debugMode){
			deletePhotoAndDataFile();
		}
		Intent intent = new Intent(getApplication(), BubbleBot.class);
		startActivity(intent); 
	}
	
	// Delete the last photo taken and its data file
	private void deletePhotoAndDataFile()
	{
		File photoFile = new File(photoPath);
		photoFile.delete();
		/*
		String dataFilename = processedDir + photoFilename.substring(0, photoFilename.length() - 4) + ".txt";
		File dataFile = new File(capturedDir + dataFilename);
		dataFile.delete();
		*/
	}
    public void run() {
		Processor mProcessor = new Processor();
		
		Log.i("Nathan", "point F");
		
		String templatePath = getResources().getString(com.bubblebot.R.string.templatePath);
		if(mProcessor.loadForm(photoPath)){
			Log.i("Nathan","Loading: " + photoPath);
			if(mProcessor.loadTemplate(templatePath)){
				detectResult = mProcessor.alignForm(alignmentOutputImage);
				Log.i("Nathan","aligned");
			}
			else{
				Log.i("Nathan","FAILED TO LOAD TEMPLATE: " + templatePath);
			}
		}
		else{
			Log.i("Nathan","FAILED TO LOAD IMAGE: " + photoPath);
		}
    	handler.sendEmptyMessage(0);
    }

    private Handler handler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                    pd.dismiss();
        			
        			CharSequence toastMessage;
        			if ( detectResult ) {
        				// Using WebView to display the straightened form image.
        				WebView myWebView = (WebView)findViewById(R.id.webview);
        				
        				myWebView.getSettings().setCacheMode(WebSettings.LOAD_NO_CACHE);
        				myWebView.getSettings().setBuiltInZoomControls(true);
        				myWebView.getSettings().setDefaultZoom(WebSettings.ZoomDensity.FAR);
        				myWebView.setVisibility(View.VISIBLE);
        				
        				// HTML is used to display the image.
        				// Appending the time stamp to the filename is a hack
        				// to prevent caching.
        				String html = new String();
        				html = ( "<body bgcolor=\"Black\"><center>" +
        							"<img src=\"file:///" + alignmentOutputImage + "?" + new Date().getTime() + "\" width=\"500\" >" +
        						"</center></body>");
        				       
        				myWebView.loadDataWithBaseURL("file:////sdcard/BubbleBot/",
        										 html, "text/html", "utf-8", "");
        				
        				process.setEnabled(true);
        				/*
        				toastMessage = getResources().getString(R.string.VerifyForm, capturedDir + photoFilename);
        				Toast.makeText(getApplicationContext(), toastMessage,
            					Toast.LENGTH_LONG).show();*/
        			}
        			else
        			{
        				//Untested:
        				//toastMessage = getResources().getString(R.string.DetectFormFailed);
        				RelativeLayout failureMessage = (RelativeLayout)findViewById(R.id.failureMessage);
        				failureMessage.setVisibility(View.VISIBLE);
        			}
        			
            }
    };
}