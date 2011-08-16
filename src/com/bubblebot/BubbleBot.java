package com.bubblebot;

import java.io.File;
import java.io.IOException;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.webkit.WebView;
import android.widget.Button;
import android.widget.RelativeLayout;

/* Bubblebot activity
 * 
 * This is the main activity. It displays the main menu
 * and launch the corresponding activity based on the user's selection
 */
public class BubbleBot extends MScanExtendedActivity implements Runnable{

	private static final int version = 25;
	private static final int APROX_IMAGE_SIZE = 1000000;
	ProgressDialog pd;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.bubble_bot); // Setup the UI
		
		SharedPreferences settings = getSharedPreferences(getResources().getString(R.string.prefs_name), 0);
		int currentVersion = settings.getInt("version", 0);
		if(currentVersion < version) {
			//TODO: It might be simpler to have all the runnables in separate files.
			pd = ProgressDialog.show(this, "Please wait...", "Extracting assets", true);
			Thread thread = new Thread(this);
			thread.start();
		}

		// Hook up handler for scan form button
		Button scanForm = (Button) findViewById(R.id.ScanButton);
		scanForm.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				//Intent intent = new Intent(getApplication(), BubbleCollect.class);
				Intent intent = new Intent(getApplication(), BubbleCollect2.class);
				//intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
				startActivity(intent);
			}
		});

		// Hook up handler for view scanned forms button
		Button viewForms = (Button) findViewById(R.id.ViewFormsButton);
		viewForms.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				Intent intent = new Intent(getApplication(), ViewBubbleForms.class);
				startActivity(intent); 
			}
		});

		// Hook up handler for bubblebot instructions button
		Button instructions = (Button) findViewById(R.id.InstructionsButton);
		instructions.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				Intent intent = new Intent(getApplication(), BubbleInstructions.class);
				startActivity(intent); 
			}
		});
	}
	@Override
	protected void onResume() {
		super.onResume();
		if(MScanUtils.getUsableSpace(this.getAppFolder()) < 4 * APROX_IMAGE_SIZE) {
			AlertDialog alert = new AlertDialog.Builder(this).create();
			alert.setMessage("It looks like there isn't enough space to store more images.");
			alert.show();
		}
	}
	@Override
	public void onBackPressed() {
		//This override is needed in order to avoid going back to the AfterPhotoTaken activity
		Intent setIntent = new Intent(Intent.ACTION_MAIN);
        setIntent.addCategory(Intent.CATEGORY_HOME);
        setIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(setIntent);
	}
	public void run() {
		
		boolean clearOldData = true;
		
		String mScanFolder = getAppFolder();
		Log.i("mScan", "A");
		
		SharedPreferences settings = getSharedPreferences(getResources().getString(R.string.prefs_name), 0);
		
		SharedPreferences.Editor editor = settings.edit();
		
		if(clearOldData){
			MScanUtils.clearDir(mScanFolder + photoDir);
			MScanUtils.clearDir(mScanFolder + alignedPhotoDir);
			MScanUtils.clearDir(mScanFolder + jsonDir);
			MScanUtils.clearDir(mScanFolder + markupDir);
			editor.clear();
		}
		
		// Set up directories if they do not exist
		(new File(mScanFolder, photoDir)).mkdirs();
		(new File(mScanFolder, alignedPhotoDir)).mkdirs();
		(new File(mScanFolder, jsonDir)).mkdirs();
		(new File(mScanFolder, markupDir)).mkdirs();
		
		Log.i("mScan", (new File(mScanFolder, markupDir)).toString());
		
		try {
			String traininExamplesDir =  mScanFolder + "training_examples";
			String formTemplatesDir = mScanFolder + "form_templates";
			MScanUtils.clearDir(traininExamplesDir);
			MScanUtils.clearDir(formTemplatesDir);
			
			extractAssets("training_examples", traininExamplesDir);
			extractAssets("form_templates", formTemplatesDir);
			
			editor.putInt("version", version);
		} catch (IOException e) {
			// TODO: Terminate the app if this fails.
			e.printStackTrace();
			Log.i("mScan", "Extration Error");
		}
		editor.commit();
		handler.sendEmptyMessage(0);
	}
	private Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
        	pd.dismiss();
        }
	};
}