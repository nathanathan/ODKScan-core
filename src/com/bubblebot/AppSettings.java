package com.bubblebot;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;

public class AppSettings extends Activity {
	
	private CheckBox formDetectionCheckBox;
	private CheckBox calibrationCheckBox;
	private SharedPreferences settings;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.app_settings); // Setup the UI
		
		settings = getSharedPreferences(getResources().getString(R.string.prefs_name), 0);
		
		formDetectionCheckBox = (CheckBox) findViewById(R.id.formDetectionCheckbox);
		formDetectionCheckBox.setOnCheckedChangeListener(new OnCheckedChangeListener()
		{
			//@Override
			public void onCheckedChanged(CompoundButton arg0, boolean isChecked) {
				SharedPreferences.Editor editor = settings.edit();
				editor.putBoolean("doFormDetection", isChecked);
				editor.commit();
			}
		});
		
		calibrationCheckBox = (CheckBox) findViewById(R.id.calibrationCheckbox);
		calibrationCheckBox.setOnCheckedChangeListener(new OnCheckedChangeListener()
		{
			//@Override
			public void onCheckedChanged(CompoundButton arg0, boolean isChecked) {
				SharedPreferences.Editor editor = settings.edit();
				editor.putBoolean("calibrate", isChecked);
				editor.commit();
			}
		});
	}
	@Override
	protected void onResume() {
		formDetectionCheckBox.setChecked(settings.getBoolean("doFormDetection", false));
		calibrationCheckBox.setChecked(settings.getBoolean("calibrate", false));
		super.onResume();
	}
}