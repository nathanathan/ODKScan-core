package com.bubblebot;

import java.io.File;
import java.io.FilenameFilter;

import android.os.Bundle;
import android.preference.PreferenceActivity;

//TODO: Replace with PreferenceActivity
public class AppSettings extends PreferenceActivity {
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            addPreferencesFromResource(R.xml.prefs);
            
            ListPreferenceMultiSelect multiSelectPreference = (ListPreferenceMultiSelect)findPreference("select_templates");
            
            //TODO: This probably has some bugs when forms are added. Would need to test.
            
            //Get the available templates:
    		File dir = new File(MScanUtils.getTemplateDirPath());	

    		String[] templateNames = dir.list(new FilenameFilter() {
    			public boolean accept (File dir, String name) {
    				if (new File(dir,name).isDirectory())
    					return false;
    				return name.toLowerCase().endsWith(".json");
    			}
    		});
            
    		//Remove suffixes and set paths
    		String[] templatePaths = new String[templateNames.length];
    		for(int i = 0; i < templateNames.length; i++){
    			templateNames[i] = templateNames[i].replace(".json", "");
    			templatePaths[i] = MScanUtils.getTemplateDirPath() + templateNames[i];
    		}
    		
            multiSelectPreference.setEntries(templateNames);
            multiSelectPreference.setEntryValues(templatePaths);
    }
}