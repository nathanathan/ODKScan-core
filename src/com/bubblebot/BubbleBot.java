package com.bubblebot;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

/* Bubblebot activity
 * 
 * This is the main activity. It displays the main menu
 * and launch the corresponding activity based on the user's selection
 */
public class BubbleBot extends Activity {

	// Initialize the application
	@Override
	protected void onCreate(Bundle savedInstanceState) {
       super.onCreate(savedInstanceState);
       setContentView(R.layout.bubble_bot); // Setup the UI
       
       // Set up directories if they do not exist
       File dir = new File("/sdcard/mScan");
       dir.mkdirs();
	   try {
	    extractAssets("training_examples", "/sdcard/mScan/training_examples");
		extractAssets("form_templates", "/sdcard/mScan/form_templates");
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			Log.i("Nathan", "ERROR");
		}
       
       // Hook up handler for scan form button
       Button scanForm = (Button) findViewById(R.id.ScanButton);
       scanForm.setOnClickListener(new View.OnClickListener() {
           public void onClick(View v) {
        	Intent intent = new Intent(getApplication(), BubbleCollect.class);
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
	// This method copies the files within a given directory (with path relative to the assets folder).
	// This does not recursively descend into subdirectories, and it might try to copy empty subdirectories.
	protected void extractAssets(String assetsDir, String outputPath) throws IOException{
		String[] assets = getAssets().list(assetsDir);
		for(int i = 0; i < assets.length; i++){
			if(getAssets().list(assetsDir + "/" + assets[i]).length == 0){
				copyAsset(assetsDir + "/" + assets[i], outputPath + "/" + assets[i]);
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