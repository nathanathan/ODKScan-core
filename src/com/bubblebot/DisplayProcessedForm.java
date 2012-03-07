package com.bubblebot;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.kxml2.io.KXmlParser;
import org.kxml2.io.KXmlSerializer;
import org.kxml2.kdom.Document;
import org.kxml2.kdom.Element;
import org.kxml2.kdom.Node;
import org.xmlpull.v1.XmlPullParserException;

import android.accounts.Account;
import android.accounts.AccountManager;

import com.bubblebot.jni.bubblebot;
import com.google.api.client.googleapis.extensions.android2.auth.GoogleAccountManager;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.ContentValues;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
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
	private static final int DIALOG_ACCOUNTS = 1110011;
	private static final int DIALOG_FAILURE = 11011011;
	private static final int DIALOG_SUCCESS = 11001111;
	private static final String AUTH_TOKEN_TYPE = "fusiontables";
    
	// Set up the UI and load the processed image
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.processed_form);

		Bundle extras = getIntent().getExtras(); 
		if (extras != null) {
			photoName = extras.getString("photoName");
			templatePath = extras.getString("templatePath");
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
	protected Dialog onCreateDialog(int id, Bundle args) {
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		switch (id) {
		case DIALOG_ACCOUNTS:
			builder.setTitle("Select a Google account");
			
			final GoogleAccountManager manager = new GoogleAccountManager(this);
			
			final Account [] accounts = manager.getAccounts();
			
			if(accounts.length > 0){
				String[] names = new String[accounts.length];
				for (int i = 0; i < accounts.length; i++) {
					names[i] = accounts[i].name;
				}
				
				builder.setItems(names, new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						gotAccount(manager, accounts[which]);
					}
				});
			}
			break;
		case DIALOG_SUCCESS:
			builder.setTitle("Success!")
		       .setCancelable(false)
		       .setPositiveButton("Hooray!", new DialogInterface.OnClickListener() {
		           public void onClick(DialogInterface dialog, int id) {
		                dialog.dismiss();
		           }
		       });
			break;
		case DIALOG_FAILURE:
			builder.setTitle("Failure :(")
			   .setMessage(args.getString("reason"))
		       .setCancelable(false)
		       .setPositiveButton("OK", new DialogInterface.OnClickListener() {
		           public void onClick(DialogInterface dialog, int id) {
		                dialog.dismiss();
		           }
		       });
			break;
		default:
			return null;
		}
		return builder.create();
	}
	private void gotAccount(final GoogleAccountManager manager, final Account account) {

		pd = ProgressDialog.show(this, "", "Uploading Data...", true, true);

		try {
			final Bundle bundle = 
				manager.manager.getAuthToken(account, AUTH_TOKEN_TYPE, true, null, null).getResult();
			
			if (bundle.containsKey(AccountManager.KEY_AUTHTOKEN)) {
				new Thread(new UploadDataThread(handler, photoName, 
						bundle.getString(AccountManager.KEY_AUTHTOKEN))).start();
			}
			else{
				pd.dismiss();
				Bundle args = new Bundle();
				args.putString("reason", "Could not obtain authentication token from the account manager." +
										 "You might need to give the app permission to access your accounts," +
										 "check the pull-down menu tray for notifications.");
				showDialog(DIALOG_FAILURE, args);
			}
		} catch (Exception e) {
			pd.dismiss();
			Bundle args = new Bundle();
			args.putString("reason", "Exception when getting auth token.");
			showDialog(DIALOG_FAILURE, args);
		}
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
			//TODO: Put the upload stuff into another file/activity to keep things cleaner
			//It should throw exceptions that can be used for the failure dialog
			NetworkInfo neti = ((ConnectivityManager) this.getSystemService(Context.CONNECTIVITY_SERVICE)).getActiveNetworkInfo();
			if(neti != null && neti.isConnected()){
				showDialog(DIALOG_ACCOUNTS);
			}
			else{
				Bundle args = new Bundle();
				args.putString("reason", "Connectivity is not available.");
				showDialog(DIALOG_FAILURE, args);
			}
			return true;
		case R.id.exportToODK:
			Log.i("mScan", "Using template: " + templatePath);
			intent = new Intent(getApplication(), MScan2CollectActivity.class);
			intent.putExtra("templatePath", templatePath);
			intent.putExtra("jsonOutPath", MScanUtils.getJsonPath(photoName));
			startActivity(intent);
			/*
			try {
				openInCollect(templatePath);
			} catch (JSONException e) {
				// TODO Auto-generated catch block
				Log.i("mScan", "Err: " + e.toString());
			} catch (IOException e) {
				// TODO Auto-generated catch block
				Log.i("mScan", "Err: " +  e.toString());
			}*/
			return true;
		case R.id.saveData:
			saveData();
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
	}
	private Handler handler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			pd.dismiss();
			if(msg.what == 1){
				showDialog(DIALOG_SUCCESS);
			}
			else{
				Bundle args = new Bundle();
				args.putString("reason", "Count not upload.");
				showDialog(DIALOG_FAILURE, args);
			}
		}
	};
}