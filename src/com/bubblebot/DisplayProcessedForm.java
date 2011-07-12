package com.bubblebot;

import java.util.Date;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

/* DisplayProcessedForm activity
 * 
 * This activity displays the image of a processed form
 */
public class DisplayProcessedForm extends Activity {
	
	TextView text;
	Button data;
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
       filename = "sdcard/BubbleBot/markedup.jpg";
       
       text = (TextView) findViewById(R.id.text);
       text.setText("Displaying " + filename);
       
       // TODO: Figure out how to share a function for doing this with AfterPhotoTaken
       // Using WebView to display the straightened form image.
		WebView myWebView = (WebView)findViewById(R.id.webview2);
		//myWebView.getSettings().setCacheMode(WebSettings.LOAD_NO_CACHE);
		myWebView.getSettings().setBuiltInZoomControls(true);
		myWebView.getSettings().setDefaultZoom(WebSettings.ZoomDensity.FAR);
		//myWebView.setVerticalScrollbarOverlay(false);
		
		// HTML is used to display the image.
		// Appending the time stamp to the filename is a hack
		// to prevent caching.
		String html = new String();
		html = ( "<body bgcolor=\"Black\"><center>" +
					"<img src=\"file:///" + filename + "?" + new Date().getTime() + "\" width=\"500\" >" +
				"</center></body>");
		       
		myWebView.loadDataWithBaseURL("file:////sdcard/BubbleBot/",
								 html, "text/html", "utf-8", "");
		
		
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