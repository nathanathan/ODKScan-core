package com.bubblebot;

import java.io.File;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
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
       File dir = new File("/sdcard/BubbleBot/capturedImages");
       dir.mkdirs();
       dir = new File("/sdcard/BubbleBot/processedImages");
       dir.mkdirs();
       dir = new File("/sdcard/BubbleBot/processedText");
       dir.mkdirs();

       
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

	@Override
	protected void onPause() {
		super.onPause();
	}

	@Override
	protected void onResume() {
		super.onResume();
	}
}