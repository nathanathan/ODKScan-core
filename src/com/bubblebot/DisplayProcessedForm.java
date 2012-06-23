package com.bubblebot;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.widget.Toast;

/**
 * 
 * This activity displays the image of a processed form
 */
public class DisplayProcessedForm extends Activity {

	//TODO: Look into using AccountAuthenticatorActivity for uploading data.
	
	private String photoName;
	private String templatePath;
    
	// Set up the UI and load the processed image
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.processed_form);

		Bundle extras = getIntent().getExtras(); 
		if (extras != null) {
			photoName = extras.getString("photoName");
			SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
			templatePath = settings.getString(photoName, "");
			if(templatePath == ""){
				Log.i("mScan", "Could not associate templatePath with photo.");
				return;
			}
		}
		/*
		setTitle(getResources().getString(R.string.Health_Center) + ": " +
                 //getSharedPreferences(getResources().getString(R.string.prefs_name), 0)
                 PreferenceManager.getDefaultSharedPreferences(getApplicationContext())
                 .getString("healthCenter", "unspecifiedHC"));
		*/
		/*
		MScanUtils.displayImageInWebView((WebView)findViewById(R.id.webview2),
				MScanUtils.getMarkedupPhotoPath(photoName));
		*/
		WebView myWebView = (WebView) findViewById(R.id.webview2);
		WebSettings webSettings = myWebView.getSettings();
		webSettings.setCacheMode(WebSettings.LOAD_NO_CACHE);
		webSettings.setBuiltInZoomControls(true);
		webSettings.setDefaultZoom(WebSettings.ZoomDensity.FAR);
		webSettings.setJavaScriptEnabled(true);

		myWebView.loadUrl("file://" + MScanUtils.getFormViewHTMLDir() + "formView.html" + "?" +
				"imageUrl=" + MScanUtils.getAlignedPhotoPath(photoName) + "&" +
				"jsonOutputUrl=" + MScanUtils.getJsonPath(photoName));
		myWebView.addJavascriptInterface(new JavaScriptInterface(getApplicationContext()), "Android");
	}
	public class JavaScriptInterface {
	    Context mContext;

	    /** Instantiate the interface and set the context */
	    JavaScriptInterface(Context c) {
	        mContext = c;
	    }
	    public void launchCollect(String field, int segment_idx, int field_idx) {
	    	//It would probably be better to launch collect directly if the form has already been exported.
	    	Log.i("mScan", "args: " + field + ' ' + segment_idx + ' ' + field_idx);
	    	Intent dataIntent = new Intent();
			dataIntent.putExtra("goto", field_idx);
			startCollect(dataIntent);
	    }
	}
	public void startCollect(Intent data) {
		if(data.getData() == null) {
			//No instance specified, create or find one with new activity.
			Intent intent = new Intent(getApplication(), MScan2CollectActivity.class);
			intent.putExtras(data);
			intent.putExtra("templatePath", templatePath);
			intent.putExtra("photoName", photoName);
			SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
			intent.putExtra("userName", settings.getString("userName", null));
			startActivityForResult(intent, 1);
			return;
		}
        //////////////
        Log.i("mScan", "Starting Collect...");
        //////////////
		//Initialize the intent that will start collect and use it to see if collect is installed.
	    Intent intent = new Intent();
	    //intent.setFlags(Intent.);
	    intent.setFlags(Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
	    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
	    intent.setComponent(new ComponentName("org.odk.collect.android",
	            "org.odk.collect.android.activities.FormEntryActivity"));
	    PackageManager packMan = getPackageManager();
	    if(packMan.queryIntentActivities(intent, 0).size() == 0){
	    	AlertDialog.Builder builder = new AlertDialog.Builder(this);
			builder.setMessage("ODK Collect was not found on this device.")
			       .setCancelable(false)
			       .setNeutralButton("Ok", new DialogInterface.OnClickListener() {
			           public void onClick(DialogInterface dialog, int id) {
			                dialog.cancel();
			                finish();
			           }
			       });
			AlertDialog alert = builder.create();
			alert.show();
	    	return;
	    }

	    intent.setAction(Intent.ACTION_EDIT);
	    intent.putExtras(data);
	    intent.setData(data.getData());
	    startActivityForResult(intent, 0);
	}
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if(requestCode == 0){
			
		} else if(requestCode == 1){
			if(resultCode == Activity.RESULT_OK) {
				startCollect(data);
			}
		}
		super.onActivityResult(requestCode, resultCode, data);
	}
	@Override
	public void onAttachedToWindow() {
		super.onAttachedToWindow();
		//Show the options menu when the activity is started (so people know it exists)
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
//			intent = new Intent(getApplication(), DisplayProcessedData.class);
//			intent.putExtra("photoName", photoName);
//			startActivity(intent); 
			return true;
		case R.id.uploadData:
			return true;
		case R.id.exportToODK:
			Log.i("mScan", "Using template: " + templatePath);
	    	Intent dataIntent = new Intent();
			dataIntent.putExtra("start", true);
			startCollect(dataIntent);
			return true;
		case R.id.saveData:
			//saveData();
			//TODO: Disable the button if this succeeds
			return true;
		case R.id.scanNewForm:
			intent = new Intent(getApplication(), BubbleCollect2.class);
			startActivity(intent);
			return true;
		case R.id.startOver:
			intent = new Intent(getApplication(), BubbleBot.class);
			startActivity(intent);
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}
    /**
     * Append the output to a CSV output file with the health center name included
     * The original output is not modified, and does not have any app specified data included in it.
     */
	/*
	private void saveData() {
		//SharedPreferences settings = getSharedPreferences(getResources().getString(R.string.prefs_name), 0);
		SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
		try {
			//The health center will be the currently selected HC,
			//not the HC selected when the photo was taken.
			//An activity state machine/graph might help here.
			JSONObject formRoot = JSONUtils.parseFileToJSONObject(MScanUtils.getJsonPath(photoName));
			JSONArray fields = formRoot.getJSONArray("fields");
			int fieldsLength = fields.length();
			
			if(fieldsLength == 0) {
				//TODO: make this function throw exceptions
				Log.i("mScan", "fieldsLength is zero");
				return;
			}
			
			File file = new File(MScanUtils.appFolder + "all-data.csv");
			 
			FileWriter fileWritter;
			BufferedWriter bufferWritter;
			
    		//if file doesn't exists, then create it
    		if(!file.exists()){
    			
    			file.createNewFile();
    			
    			fileWritter = new FileWriter(file.toString());
				bufferWritter = new BufferedWriter(fileWritter);
				
    			for(int i = 0; i < fieldsLength; i++){
    				bufferWritter.write(fields.getJSONObject(i).getString("label") + "," );
    			}
    			
    			bufferWritter.write("Health Center");
    			bufferWritter.newLine();
    		}
    		else{
    			fileWritter = new FileWriter(file.toString(), true);
				bufferWritter = new BufferedWriter(fileWritter);
    		}

    		//Generate the CSV from fields
    		for(int i = 0; i < fieldsLength; i++){
				JSONObject field = fields.getJSONObject(i);
				if( field.getJSONArray("segments").getJSONObject(0).getString("type").equals("bubble") ){
					Number[] segmentCounts = JSONUtils.getSegmentValues(field);
					bufferWritter.write(MScanUtils.sum(segmentCounts).intValue());
				}
				else{
					JSONArray textSegments = field.getJSONArray("segments");
					int textSegmentsLength = textSegments.length();
					for(int j = 0; j < textSegmentsLength; j++){
						bufferWritter.write(textSegments.getJSONObject(j).optString("value", ""));
					}
				}
				bufferWritter.write(",");
			}
    		bufferWritter.write(settings.getString("healthCenter", "unspecifiedHC"));
    		bufferWritter.newLine();
			bufferWritter.close();
			
		} catch (JSONException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			Log.i("mScan", "JSON excetion while saving data.");
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			Log.i("mScan", "IO excetion while saving data.");
		}
	}*/
}