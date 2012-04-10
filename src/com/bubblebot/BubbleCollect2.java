package com.bubblebot;

import java.io.File;
import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.provider.MediaStore;
import android.util.Log;
/*
 * BubbleCollect2 launches the Android camera app and collects the picture taken.
 */
//TODO: Add some kind of error handling for when the device doesn't have a camera app.
//		It's an unlikely scenario so this isn't a priority.
public class BubbleCollect2 extends Activity {

	private static final int TAKE_PICTURE = 12346789;
	private String photoName;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		photoName = getUniqueName("img");
		
		File outputPath = new File(MScanUtils.getOutputPath(photoName));
		
		//Create output folder
		if(!(new File(outputPath, "segments")).mkdirs()){
			Log.i("mScan", "Could not create output folder.");
		}

		Uri imageUri = Uri.fromFile( new File(MScanUtils.getPhotoPath(photoName)) );
		Intent intent = new Intent(android.provider.MediaStore.ACTION_IMAGE_CAPTURE);
		
		//Intent intent = new Intent("android.media.action.IMAGE_CAPTURE");
		intent.putExtra(MediaStore.EXTRA_OUTPUT, imageUri);
		
		//intent.putExtra(MediaStore.EXTRA_SCREEN_ORIENTATION, "");
		//intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
		
		//TODO: I don't think I need this flag.
		intent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
		startActivityForResult(intent, TAKE_PICTURE);
	}
	@Override
	public void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		Log.i("mScan", "Camera activity result: " + requestCode);
		switch (requestCode) {
		case TAKE_PICTURE:
			finishActivity(TAKE_PICTURE);
			if (resultCode == Activity.RESULT_OK) {
				Intent intent = new Intent(getApplication(), AfterPhotoTaken.class);
				intent.putExtra("photoName", photoName);

				if( new File(MScanUtils.getPhotoPath(photoName)).exists() ) {
					Log.i("mScan", "Starting BubbleProcess activity with "
							+ MScanUtils.getPhotoPath(photoName) + "...");
					startActivity(intent);
				}
				else{
					Log.i("mScan", "photo name: " + photoName);
					Log.i("mScan", "I think the photo couldn't be written for some reason.");
				}
			}
			else if(resultCode == Activity.RESULT_FIRST_USER){
				Log.i("mScan", "First User");
			}
			else if(resultCode == Activity.RESULT_CANCELED){
				Log.i("mScan", "Canceled");
			}
			finish(); //maybe important... activities are complicated
		}
	}
	//This method will generate a unique name with the given prefix by appending
	//the current value of a counter, then incrementing the counter.
	//Each prefix used has its own counter stored in the share preferences.
	protected String getUniqueName(String prefix) {
		//SharedPreferences settings = getSharedPreferences(getResources().getString(R.string.prefs_name), 0);
		SharedPreferences settings = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
		int uid = settings.getInt(prefix, 0);
		SharedPreferences.Editor editor = settings.edit();
		editor.putInt(prefix, uid + 1);
		editor.commit();
		return prefix + uid;
	}
}