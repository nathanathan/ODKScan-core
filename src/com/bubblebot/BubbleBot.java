package com.bubblebot;


import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.view.View;
import android.widget.AdapterView;
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
	SharedPreferences settings;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.bubble_bot); // Setup the UI
		
		//settings = getSharedPreferences(getResources().getString(R.string.prefs_name), 0);
		settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
		
		if(!checkSDCard()) return;
		
		checkVersion();
		
		hookupButtonHandlers();
		
		setupSpinner();
	}
	
	//Checks if the app is up-to-date and runs the setup if necessary
	private void checkVersion() {
		int currentVersion = settings.getInt("version", 0);
		if(currentVersion < RunSetup.version) {
			//TODO: It might be simpler to have all the runnables in separate files.
			pd = ProgressDialog.show(this, "Please wait...", "Extracting assets", true);
			Thread thread = new Thread(new RunSetup(handler, settings, getAssets()));
			thread.start();
		}
	}

	private void hookupButtonHandlers() {
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

	public void setupSpinner() {
		final String [] healthCenterNames = getResources().getStringArray(R.array.healthCenterNames);
		Spinner spinny = (Spinner) findViewById(R.id.healthCenterSpinner);
		ArrayAdapter<String> adapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, healthCenterNames);
		adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
		spinny.setPrompt("Select Health Center");
		spinny.setAdapter(adapter);
		spinny.setOnItemSelectedListener(new Spinner.OnItemSelectedListener(){

			public void onItemSelected(AdapterView<?> parent, View v, int position, long id) {
				
				String healthCenter = parent.getItemAtPosition(position).toString();
				
				SharedPreferences.Editor editor = settings.edit();
				editor.putString("healthCenter", healthCenter);
				//Note: I was worried about a bug where the healthCenter setting wouldn't match the spinner if it got reinitialized,
				//      but I think they will always match because onItemSelected seems to be called when the spinner is initialized.
				editor.commit();
				//Toast.makeText(parent.getContext(), parent.getItemAtPosition(position).toString(), Toast.LENGTH_LONG).show();
			}

			public void onNothingSelected(AdapterView<?> parent) {
				Toast.makeText(parent.getContext(), "Nothing selected.", Toast.LENGTH_LONG).show();
			}
			
		});
	}
	private boolean checkSDCard() {
		//http://developer.android.com/guide/topics/data/data-storage.html#filesExternal
		String state = Environment.getExternalStorageState();

		String errorMsg = "";
		
		if (Environment.MEDIA_MOUNTED.equals(state)) {
			 // We can read and write the media
			// Now Check that there is room to store more images
			final int APROX_IMAGE_SIZE = 1000000;
			long usableSpace = MScanUtils.getUsableSpace(MScanUtils.appFolder);
			if(usableSpace >= 0 && usableSpace < 4 * APROX_IMAGE_SIZE) {
				errorMsg = "It looks like there isn't enough space to store more images.";
			}
		} else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
			errorMsg = "We cannot write the media.";
		} else {
			errorMsg = "We can neither read nor write the media.";
		}
		if(errorMsg.isEmpty()){
			return true;
		}
		AlertDialog alert = new AlertDialog.Builder(this).create();
		alert.setMessage(errorMsg);
		alert.show();
		return false;
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