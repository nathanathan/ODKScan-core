package com.bubblebot;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

/* BubbleProcessData activity
 * 
 * This activity displays the digitized information of a processed form
 */
public class DisplayProcessedData extends Activity{
	
	TextView data;
	Button button;
	File dir = new File("/sdcard/BubbleBot/processedText/");

	String filename = "";
	
	// Set up the UI
	@Override
	protected void onCreate(Bundle savedInstanceState) {
       super.onCreate(savedInstanceState);
       setContentView(R.layout.processed_data);
       
       Bundle extras = getIntent().getExtras(); 
       if(extras !=null)
       {
    	   filename = extras.getString("file") + ".txt";
       }
       
       //read in captured data from textfile
	   File file = new File(dir,filename);
       
	   //Read text from file
	   StringBuilder text = new StringBuilder();
	   text.append(dir.toString() + "/" + filename + "\n\n");
	   try {
		   BufferedReader br = new BufferedReader(new FileReader(file));
		   String line;
	
		   while ((line = br.readLine()) != null) 
		   {
	         	text.append(line);
	         	text.append('\n');
		   }
	   }
	   catch (IOException e) {
		   //You'll need to add proper error handling here
	   }

       data = (TextView) findViewById(R.id.text);
       data.setText("Data from: " + text);
       
       button = (Button) findViewById(R.id.button);
       button.setOnClickListener(new View.OnClickListener() {  
	       public void onClick(View v) {
	    	   //Back to the menu
	    	   Intent intent = new Intent(getApplication(), BubbleBot.class);
	    	   intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
   			   startActivity(intent); 
	       }
	    });
	}

}
