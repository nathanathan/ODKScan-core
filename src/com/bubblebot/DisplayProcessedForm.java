package com.bubblebot;

import java.util.Date;

import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.widget.TextView;

/* DisplayProcessedForm activity
 * 
 * This activity displays the image of a processed form
 */
public class DisplayProcessedForm extends MScanExtendedActivity {
	
	TextView text;
	private String photoName;
	
	// Set up the UI and load the processed image
	@Override
	protected void onCreate(Bundle savedInstanceState) {
       super.onCreate(savedInstanceState);
       setContentView(R.layout.processed_form);
       
       Bundle extras = getIntent().getExtras(); 
		if (extras != null) {
			photoName = extras.getString("photoName");
		}
       
       text = (TextView) findViewById(R.id.text);
       text.setText("Displaying " + photoName);
       
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
					"<img src=\"file:///" + getMarkedupPhotoPath(photoName) + "?" + new Date().getTime() + "\" width=\"500\" >" +
				"</center></body>");
		       
		myWebView.loadDataWithBaseURL("file:////sdcard/BubbleBot/",
								 html, "text/html", "utf-8", "");
		
	}
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
	    MenuInflater inflater = getMenuInflater();
	    inflater.inflate(R.menu.mscan_menu, menu);
	    return true;
	}
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
	    // Handle item selection
	    switch (item.getItemId()) {
	    case R.id.displayData:
			Intent intent = new Intent(getApplication(), DisplayProcessedData.class);
			startActivity(intent); 
	        return true;
	    default:
	        return super.onOptionsItemSelected(item);
	    }
	}
}