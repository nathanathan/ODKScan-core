package com.bubblebot;


import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import android.widget.Toast;

/* Bubblebot activity
 * 
 * This is the main activity. It displays the main menu
 * and launch the corresponding activity based on the user's selection
 */
public class BubbleBot extends Activity {

	ProgressDialog pd;
	public boolean spaceAlerted = false;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.bubble_bot); // Setup the UI
		
		SharedPreferences settings = getSharedPreferences(getResources().getString(R.string.prefs_name), 0);
		int currentVersion = settings.getInt("version", 0);
		if(currentVersion < RunSetup.version) {
			//TODO: It might be simpler to have all the runnables in separate files.
			pd = ProgressDialog.show(this, "Please wait...", "Extracting assets", true);
			Thread thread = new Thread(new RunSetup(handler, settings, getAssets()));
			thread.start();
		}
		
		SetupSpinner();
		
		// Hook up handler for scan form button
		Button scanForm = (Button) findViewById(R.id.ScanButton);
		scanForm.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				//Intent intent = new Intent(getApplication(), BubbleCollect.class);
				Intent intent = new Intent(getApplication(), BubbleCollect2.class);
				//intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
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
		
		// Hook up handler for bubblebot settings button
		Button settingsButton = (Button) findViewById(R.id.SettingsButton);
		settingsButton.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				Intent intent = new Intent(getApplication(), AppSettings.class);
				startActivity(intent); 
			}
		});
	}

	// Method to initialize UI
	public void SetupSpinner()
	{
		final String [] healthCenterNames = getResources().getStringArray(R.array.healthCenterNames);
		Spinner spinny = (Spinner) findViewById(R.id.healthCenterSpinner);
		ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, healthCenterNames);
		
		/*
		spinny.setOnItemClickListener(new OnItemClickListener(){

			public void onItemClick(AdapterView<?> arg0, View arg1, int arg2,
					long arg3) {
				Toast.makeText(getApplicationContext(), healthCenterNames[arg2], 5).show();
			}

        });
		*/
		adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		spinny.setAdapter(adapter);
		spinny.setOnItemSelectedListener(new mySpinnerListener());
	}
	
	class mySpinnerListener implements Spinner.OnItemSelectedListener
	{
		public void onItemSelected(AdapterView<?> parent, View v, int position, long id) {
			// TODO Auto-generated method stub
			Toast.makeText(parent.getContext(), parent.getItemAtPosition(position).toString(), Toast.LENGTH_LONG).show();
		}
		public void onNothingSelected(AdapterView<?> parent) {
			// TODO Auto-generated method stub
			// Do nothing.
		}
	
	}
	
	
	@Override
	protected void onResume() {
		super.onResume();
		final int APROX_IMAGE_SIZE = 1000000;
		long usableSpace = MScanUtils.getUsableSpace(MScanUtils.appFolder);
		if(!spaceAlerted && usableSpace >= 0 && usableSpace < 4 * APROX_IMAGE_SIZE) {
			AlertDialog alert = new AlertDialog.Builder(this).create();
			alert.setMessage("It looks like there isn't enough space to store more images.");
			alert.show();
			spaceAlerted = true;
		}
	}
	@Override
	public void onBackPressed() {
		//This override is needed in order to avoid going back to the AfterPhotoTaken activity
		Intent setIntent = new Intent(Intent.ACTION_MAIN);
        setIntent.addCategory(Intent.CATEGORY_HOME);
        setIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(setIntent);
	}
	private Handler handler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
        	pd.dismiss();
        }
	};
}