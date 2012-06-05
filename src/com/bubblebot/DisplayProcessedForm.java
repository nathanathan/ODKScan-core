package com.bubblebot;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.webkit.WebView;

/**
 * 
 * This activity displays the image of a processed form
 */
public class DisplayProcessedForm extends Activity {

	//TODO: Look into using AccountAuthenticatorActivity for uploading data.
	
	private String photoName;
	private String templatePath;
	
	private ProgressDialog pd;
    
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
		
		setTitle(getResources().getString(R.string.Health_Center) + ": " +
                 //getSharedPreferences(getResources().getString(R.string.prefs_name), 0)
                 PreferenceManager.getDefaultSharedPreferences(getApplicationContext())
                 .getString("healthCenter", "unspecifiedHC"));
		
		MScanUtils.displayImageInWebView((WebView)findViewById(R.id.webview2),
				MScanUtils.getMarkedupPhotoPath(photoName));
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
			intent = new Intent(getApplication(), MScan2CollectActivity.class);
			intent.putExtra("templatePath", templatePath);
			intent.putExtra("photoName", photoName);
			startActivity(intent);
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