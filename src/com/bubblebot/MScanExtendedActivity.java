package com.bubblebot;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.app.Activity;
import android.content.SharedPreferences;

/*
 * Extended activity is a collection of methods that are
 * useful to inherit throughout the mScan code.
 */

public class MScanExtendedActivity extends Activity {

	protected static final String photoDir = "photos/";
	protected static final String alignedPhotoDir = "alignedPhotos/";
	protected static final String jsonDir = "jsonOut/";
	protected static final String markupDir = "markedupPhotos/";

	protected String getPhotoPath(String photoName){
		return getAppFolder() + photoDir + photoName + ".jpg";
	}
	protected String getAlignedPhotoPath(String photoName){
		return getAppFolder() + alignedPhotoDir + photoName + ".jpg";
	}
	protected String getJsonPath(String photoName){
		return getAppFolder() + jsonDir + photoName + ".json";
	}
	protected String getMarkedupPhotoPath(String photoName){
		return getAppFolder() + markupDir + photoName + ".jpg";
	}
	protected String getAppFolder(){
		SharedPreferences settings = getSharedPreferences(getResources().getString(R.string.prefs_name), 0);
		return settings.getString("appFolder", "/sdcard/mScan/");
	}
	// This method copies the files within a given directory (with path relative to the assets folder).
	protected void extractAssets(String assetsDir, String outputPath) throws IOException{
		File dir = new File(outputPath);
		dir.mkdirs();
		String[] assets = getAssets().list(assetsDir);
		for(int i = 0; i < assets.length; i++){
			if(getAssets().list(assetsDir + "/" + assets[i]).length == 0){
				copyAsset(assetsDir + "/" + assets[i], outputPath + "/" + assets[i]);
			}
			else{
				extractAssets(assetsDir + "/" + assets[i], outputPath + "/" + assets[i]);
			}
		}
	}
	protected void copyAsset(String assetLoc, String outputLoc) throws IOException {
		//Log.i("mScan", "copy from " + assetLoc + " to " + outputLoc);
		InputStream fis = getAssets().open(assetLoc);
		File trainingExampleFile = new File(outputLoc);
		trainingExampleFile.createNewFile();
		FileOutputStream fos = new FileOutputStream(trainingExampleFile);

		// Transfer bytes from in to out
		byte[] buf = new byte[1024];
		int len;
		while ((len = fis.read(buf)) > 0) {
			fos.write(buf, 0, len);
		}

		fos.close();
		fis.close();
	}
}