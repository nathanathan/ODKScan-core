package com.bubblebot;

import java.util.Date;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.widget.FrameLayout;

import com.bubblebot.jni.Processor;
import com.bubblebot.jni.MarkupForm;

/* BubbleProcess activity
 * 
 * This activity processes an image of a bubble form and digitizes its information
 */
public class BubbleProcess extends Activity  {
	
	/* ProcessFormTask is an async task that displays a progress
	 * dialog while processing the form in an image
	 */
    private class ProcessFormTask extends AsyncTask<Void, Void, Void> {
        private Activity parent;
        private ProgressDialog dialog;
        private String filename;
        private long startTime;

        // Constructor
        public ProcessFormTask(Activity parent) {
            this.parent = parent;
        }

        // Before the task executes, display a progress dialog to indicate that
        // the program is processing the form.
        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            startTime = new Date().getTime();
            dialog = new ProgressDialog(parent);
            dialog.setMessage(getResources().getText(com.bubblebot.R.string.ProcessForm));
            dialog.setIndeterminate(true);
            dialog.setCancelable(false);
            dialog.show();
        }/*
        @Override
        protected void onProgressUpdate(Integer... progress){
        	Log.i("Nathan", progress.toString());
        }*/
        
        // When the task completes, remove the progress dialog
        @Override
        protected void onPostExecute(Void result) {
            super.onPostExecute(result);
            
            double timeTaken = (double)(new Date().getTime() - startTime) / 1000;
    		Log.i("BubbleProcess", "Process form elapsed time:" + String.format("%.2f", timeTaken));
            
            dialog.dismiss();
            parent.finish();
            
            // Lanuch the DisplayProcessedForm activity to display the processed form
            Intent intent = new Intent(getApplication(), DisplayProcessedForm.class);
            intent.putExtra("file", filename);
            startActivity(intent);
        }

        // Run the C++ code that process the form in the photo
		@Override
		protected Void doInBackground(Void... arg) {
			Log.d("Nathan","PICTURE NAME: " + pictureName);
			Processor mProcessor = new Processor(getResources().getString(com.bubblebot.R.string.templatePath));
			mProcessor.trainClassifier("/sdcard/mScan/training_examples");
			mProcessor.loadForm(pictureName);
			mProcessor.processForm("sdcard/BubbleBot/output.json");
			//This is a big hacky and should be fixed eventually...
			//Do I have to instantiate something to use it's methods?
			(new MarkupForm()).markupForm("sdcard/BubbleBot/output.json", pictureName, "sdcard/BubbleBot/markedup.jpg");
			
			return null;
		}
    }
 
    // The filename of the image
    private String pictureName;
	
	// An instance of the process form task
	private ProcessFormTask mTask = null;
	
	// Initialize the application
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
		
		// Create an async task for processing the form
		mTask = new ProcessFormTask(this);
		
		FrameLayout frame = new FrameLayout(this);
		setContentView(frame);
	}

	@Override
	protected void onPause() {
		super.onPause();
		mTask.cancel(true);
	}

	@Override
	protected void onResume() {
		super.onResume();
		
		// Use null for now. Eventually we will pass in a real image to the processor.
		Void[] arg = null;
		mTask.execute(arg);
	}
}
	