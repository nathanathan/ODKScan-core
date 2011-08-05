package com.bubblebot;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.View;
import android.widget.Button;

/* Bubblebot activity
 * 
 * This is the main activity. It displays the main menu
 * and launch the corresponding activity based on the user's selection
 */

public class BubbleBot extends Activity {

	public static final String PREFS_NAME = "mScanPrefs";
	//The version variable is used to reextract assets after they are modified.
	public static final int version = 12;
	// Initialize the application
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.bubble_bot); // Setup the UI
		
		if( (new File("/sdcard")).getUsableSpace() < 1000){
			Log.i("Nathan", "Not enough space");
			//TODO: make a message to this effect that offers to remove saved images.
			return;
		}
		
		//Extract all the necessary assets.
		//TODO: this can take a few seconds so add a dialog window.
		SharedPreferences settings = getSharedPreferences(PREFS_NAME, 0);
		int currentVersion = settings.getInt("version", 0);
		if(currentVersion < version){
			// Set up directories if they do not exist
			File dir = new File("/sdcard/mScan");
			dir.mkdirs();
			try {
				clearDir("/sdcard/mScan/training_examples");
				clearDir("/sdcard/mScan/form_templates");
				clearDir("/sdcard/mScan/nothing");
				extractAssets("training_examples", "/sdcard/mScan/training_examples");
				extractAssets("form_templates", "/sdcard/mScan/form_templates");
				SharedPreferences sharedPrefs = getSharedPreferences(PREFS_NAME, 0);
				SharedPreferences.Editor editor = sharedPrefs.edit();
				editor.putInt("version", version);
				// Commit the edits!
				editor.commit();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				Log.i("Nathan", "ERROR");
			}
		}

		// Hook up handler for scan form button
		Button scanForm = (Button) findViewById(R.id.ScanButton);
		scanForm.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				//Intent intent = new Intent(getApplication(), BubbleCollect.class);
				Intent intent = new Intent(getApplication(), BubbleCollect2.class);
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
	
	protected void clearDir(String dirPath){
		File dir = new File(dirPath);
		if(dir.exists()){
			String[] files = dir.list();
			for(int i = 0; i < files.length; i++){
				File file = new File(dirPath + "/" + files[i]);
				Log.i("Nathan", dirPath + "/" + files[i]);
				if(file.isDirectory()){
					clearDir(dirPath + "/" + files[i]);
				}
				else{
					file.delete();
				}
			}
			dir.delete();
		}
		else{
			Log.i("Nathan", "clearDir error");
		}
	}
	// This method copies the files within a given directory (with path relative to the assets folder).
	protected void extractAssets(String assetsDir, String outputPath) throws IOException{
		File dir = new File(outputPath);
		dir.mkdirs();
		String[] assets = getAssets().list(assetsDir);
		for(int i = 0; i < assets.length; i++){
			if(getAssets().list(assetsDir + "/" + assets[i]).length == 0){
				copyAsset(assetsDir + "/" + assets[i], outputPath + "/" + assets[i]);
			}
			else{
				extractAssets(assetsDir + "/" + assets[i], outputPath + "/" + assets[i]);
			}
		}
	}
	protected void copyAsset(String assetLoc, String outputLoc) throws IOException{
		//Log.i("Nathan", "copy from " + assetLoc + " to " + outputLoc);
		InputStream fis = getAssets().open(assetLoc);
		File trainingExampleFile = new File(outputLoc);
		trainingExampleFile.createNewFile();
		FileOutputStream fos = new FileOutputStream(trainingExampleFile);

		// Transfer bytes from in to out
		byte[] buf = new byte[1024];
		int len;
		while ((len = fis.read(buf)) > 0) {
			fos.write(buf, 0, len);
		}

		fos.close();
		fis.close();
	}

	@Override
	protected void onPause() {
		super.onPause();
	}

	@Override
	protected void onResume() {
		super.onResume();
	}
}