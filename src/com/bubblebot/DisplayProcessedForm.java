package com.bubblebot;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

/* DisplayProcessedForm activity
 * 
 * This activity displays the image of a processed form
 */
public class DisplayProcessedForm extends Activity {
	
	ImageView image;
	TextView text;
	Button data;
	Bitmap bm;
	String dir = "/sdcard/mScan/";
	String filename = "";
	
	// Set up the UI and load the processed image
	@Override
	protected void onCreate(Bundle savedInstanceState) {
       super.onCreate(savedInstanceState);
       setContentView(R.layout.processed_form);
       
       Bundle extras = getIntent().getExtras(); 
       if(extras !=null)
       {
    	   filename = extras.getString("file");
       }
       
       text = (TextView) findViewById(R.id.text);
       text.setText("Displaying " + dir + filename);
       
       image = (ImageView) findViewById(R.id.image);
       
       //TODO: Show something impressive here.
       //bm = BitmapFactory.decodeFile(dir + filename);
       //image.setImageBitmap(bm);
       
       data = (Button) findViewById(R.id.button);
       data.setOnClickListener(new View.OnClickListener() {
	       public void onClick(View v) {
	    	   //Find a way to pass in the relevant filename - displaying a set image for now
	    	   Intent intent = new Intent(getApplication(), DisplayProcessedData.class);
	    	   //String fname = (filename.substring(0,filename.length()-4));
	    	   //intent.putExtra("file", fname);
   			   startActivity(intent); 
	       }
	    });
	}
}