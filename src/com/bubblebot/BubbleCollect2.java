package com.bubblebot;

import java.io.File;
import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;

public class BubbleCollect2 extends Activity {
	
	private static final int TAKE_PICTURE = 12346789;
	private Uri imageUri;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Log.i("Nathan", "Create called by:" + savedInstanceState.CREATOR.toString());
		
		Intent intent = new Intent(android.provider.MediaStore.ACTION_IMAGE_CAPTURE);
		File photo = new File(getResources().getString(R.string.app_folder),  "lastPictureTaken.jpg");
	    intent.putExtra(MediaStore.EXTRA_OUTPUT, Uri.fromFile(photo));
	    imageUri = Uri.fromFile(photo);
	   // intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
	    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_SINGLE_TOP);
	    startActivityForResult(intent, TAKE_PICTURE);
	}
	@Override
	public void onActivityResult(int requestCode, int resultCode, Intent data) {
	    super.onActivityResult(requestCode, resultCode, data);
	    switch (requestCode) {
	    case TAKE_PICTURE:
	    	Log.i("Nathan", ":"+resultCode);
	    	Log.i("Nathan", ":"+imageUri.toString());
	    	
	    	finishActivity(TAKE_PICTURE);
	    
	        if (resultCode == Activity.RESULT_OK) {
	    		Intent intent = new Intent(getApplication(), AfterPhotoTaken.class);
	    		intent.putExtra("file", imageUri.getPath());
	    		Log.i("Nathan", "Starting BubbleProcess activity with "
	    				+ imageUri.getPath() + "...");
	    		startActivity(intent);
	        }
	        else if(resultCode == Activity.RESULT_FIRST_USER){
	        	Log.i("Nathan", "First User");
	        }
	        else if(resultCode == Activity.RESULT_CANCELED){
	        	Log.i("Nathan", "Canceled");
	    		Intent intent = new Intent(getApplication(), AfterPhotoTaken.class);
	    		intent.putExtra("file", imageUri.getPath());
	    		Log.i("Nathan", "Starting BubbleProcess activity with "
	    				+ imageUri.getPath() + "...");
	    		startActivity(intent);
	        }
	        finish();
	    }
	}
}