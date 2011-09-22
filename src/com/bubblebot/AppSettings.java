package com.bubblebot;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;

public class AppSettings extends Activity {
	
	private CheckBox cb;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.app_settings); // Setup the UI
		
		cb = (CheckBox) findViewById(R.id.formDetectionCheckbox);
		
		cb.setOnCheckedChangeListener(new OnCheckedChangeListener()
		{
			@Override
			public void onCheckedChanged(CompoundButton arg0, boolean isChecked) {
				SharedPreferences settings = getSharedPreferences(getResources().getString(R.string.prefs_name), 0);
				SharedPreferences.Editor editor = settings.edit();
				editor.putBoolean("doFormDetection", isChecked);
				editor.commit();
			}
		});
	}
	@Override
	protected void onResume() {
		SharedPreferences settings = getSharedPreferences(getResources().getString(R.string.prefs_name), 0);
		cb.setChecked(settings.getBoolean("doFormDetection", false));
		super.onResume();
	}
}