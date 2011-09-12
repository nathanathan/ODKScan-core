package com.bubblebot;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.webkit.WebView;
import android.widget.TextView;

/* DisplayProcessedForm activity
 * 
 * This activity displays the image of a processed form
 */
public class DisplayProcessedForm extends Activity {

	private TextView text;
	private String photoName;

	private ProgressDialog pd;

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

		MScanUtils.displayImageInWebView((WebView)findViewById(R.id.webview2),
				MScanUtils.getMarkedupPhotoPath(photoName));
	}
	@Override
	public void onAttachedToWindow() {
		super.onAttachedToWindow();
		openOptionsMenu();
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
		Intent intent;
		switch (item.getItemId()) {
		case R.id.displayData:
			intent = new Intent(getApplication(), DisplayProcessedData.class);
			intent.putExtra("photoName", photoName);
			startActivity(intent); 
			return true;
		case R.id.uploadData:
			ConnectivityManager cm = (ConnectivityManager) this.getSystemService(Context.CONNECTIVITY_SERVICE);
			if(cm.getActiveNetworkInfo().isConnected()){
				
				//TODO: make dialog here
				
				pd = ProgressDialog.show(this, "", "Uploading Data...", true, true);
				Thread thread = new Thread(new UploadDataThread(handler, photoName));
				thread.start();
			}
			else{
				pd = ProgressDialog.show(this, "", "Sorry, Internet connectivity is not available.", true, true);
			}
			return true;
		case R.id.scanNewForm:
			intent = new Intent(getApplication(), BubbleCollect2.class);
			startActivity(intent);
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}
	private Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
        	pd.dismiss();
        }
	};
}