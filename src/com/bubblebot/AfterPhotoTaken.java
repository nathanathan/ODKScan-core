package com.bubblebot;

import java.io.File;
import java.util.Date;

import com.bubblebot.jni.Processor;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;


/* AfterPhotoTaken activity
 * 
 * This activity detects the form in the photo taken from the camera and
 * let the user decides whether to retake the photo or process the form.
 */
public class AfterPhotoTaken extends Activity {
	TextView text;
	Button retake;
	Button process;
	String capturedDir = "/sdcard/BubbleBot/capturedImages/";
	String processedDir = "/sdcard/BubbleBot/processedImages/";
	String tempjpg = "/sdcard/BubbleBot/preview.jpg";
	String photoFilename = "";

	private final Processor mProcessor = new Processor();
	private DetectOutlineTask task;

	/* DetectOutlineTask is an async task that displays a progress
	 * dialog while detecting the form in a photo
	 */
	private class DetectOutlineTask extends AsyncTask<Void, Void, Void> {
		private Activity parent;
		private ProgressDialog dialog;
		private String imageFilename;
		private long startTime;
		private boolean detectResult;

		// Constructor
		public DetectOutlineTask(Activity parent, String filename) {
			this.parent = parent;
			this.imageFilename = filename;
			if (imageFilename.endsWith(".jpg"))
			{
				imageFilename = imageFilename.substring(0, imageFilename.length() - 4);
			}
		}

		// Before the task executes, display a progress dialog to indicate that
		// the program is analyzing the image.
		@Override
		protected void onPreExecute() {
			super.onPreExecute();
			startTime = new Date().getTime();
			dialog = new ProgressDialog(parent);
			dialog.setMessage(getResources().getText(
					com.bubblebot.R.string.DetectForm));
			dialog.setIndeterminate(true);
			dialog.setCancelable(false);
			dialog.show();
		}

		// When the task completes, remove the progress dialog
		@Override
		protected void onPostExecute(Void result) {
			super.onPostExecute(result);

			double timeTaken = (double) (new Date().getTime() - startTime) / 1000;
			Log.i("AfterPhotoTaken",
					"Detect outline elapsed time:"
							+ String.format("%.2f", timeTaken));
			dialog.dismiss();
			if (detectResult) {
				ImageView image = (ImageView)parent.findViewById(R.id.image);
				Bitmap bm = BitmapFactory.decodeFile(tempjpg);
				image.setImageBitmap(bm);
				Button btProcess = (Button)parent.findViewById(R.id.process_button);
				btProcess.setEnabled(true);
			}
			else
			{
				text.setText(R.string.DetectFormFailed);
			}
			text.setVisibility(TextView.VISIBLE);
		}

		// Run the C++ code that detects the form in the photo
		@Override
		protected Void doInBackground(Void... arg) {
			detectResult = mProcessor.DetectOutline(imageFilename);
			return null;
		}
	}

	// Set up the UI when the activity is created
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.after_photo_taken);

		Bundle extras = getIntent().getExtras();
		if (extras != null) {
			photoFilename = extras.getString("file");
		}

		text = (TextView) findViewById(R.id.text);
		text.setText(getResources().getString(R.string.VerifyForm, capturedDir + photoFilename));
		text.setVisibility(TextView.INVISIBLE);
		
		// Create an async task for detecting the form in the photo
		task = new DetectOutlineTask(this, photoFilename);

		// If user chose to retake the photo, delete the last photo taken
		// and start the BubbleCollect activity
		retake = (Button) findViewById(R.id.retake_button);
		retake.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				deletePhotoAndDataFile();
				Intent intent = new Intent(getApplication(),
						BubbleCollect.class);
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
				intent.putExtra("file", photoFilename);
				startActivity(intent);
				finish();
			}
		});
		process.setEnabled(false);
	}

	// If the user presses the back button, delete the last
	// photo taken and exit.
	@Override
	public void onBackPressed() {
		super.onBackPressed();
		deletePhotoAndDataFile();
	}

	@Override
	protected void onPause() {
		super.onPause();
	}

	// Execute the task when the activity resumes
	@Override
	protected void onResume() {
		super.onResume();
		Void[] arg = null;
		task.execute(arg);
	}
	
	// Delete the last photo taken and its data file
	private void deletePhotoAndDataFile()
	{
		File photoFile = new File(capturedDir + photoFilename);
		photoFile.delete();
		String dataFilename = processedDir + photoFilename.substring(0, photoFilename.length() - 4) + ".txt";
		File dataFile = new File(capturedDir + dataFilename);
		dataFile.delete();
	}
}