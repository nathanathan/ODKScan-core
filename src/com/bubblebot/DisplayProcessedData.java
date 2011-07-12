package com.bubblebot;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

import android.app.Activity;
import android.app.ListActivity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/* BubbleProcessData activity
 * 
 * This activity displays the digitized information of a processed form
 */
public class DisplayProcessedData  extends ListActivity {

	// Set up the UI
	@Override
	protected void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);


		/*
       Bundle extras = getIntent().getExtras(); 
       if(extras !=null)
       {
    	   filename = extras.getString("file") + ".j";
       }
		 */
		//read in captured data from textfile


		try {
			
			JSONArray bubbleVals = parseFileToJSONArray("sdcard/BubbleBot/output.json");
			//TODO: replace this with some kind of expanding menu thing so that clicking on a field
			//		allows users to see things broken down by segment.
			Integer[] fieldCounts = getSegmentCounts(bubbleVals.getJSONObject(0));
			
			setListAdapter(new ArrayAdapter<Integer>(this, R.layout.list_item, fieldCounts));

			ListView lv = getListView();
			lv.setTextFilterEnabled(true);

			lv.setOnItemClickListener(new OnItemClickListener() {
				public void onItemClick(AdapterView<?> parent, View view,
						int position, long id) {
					// When clicked, show a toast with the TextView text
					Toast.makeText(getApplicationContext(), ""+id,
							Toast.LENGTH_SHORT).show();
				}
			});


		} catch (JSONException e) {
			// TODO Auto-generated catch block
			Log.i("Nathan", "JSON parsing problems.");
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
	    	Log.i("Nathan", "IO problems.");
			e.printStackTrace();
		}
	}

	private JSONArray parseFileToJSONArray(String bvFilename) throws JSONException, IOException {
		File jsonFile = new File(bvFilename);

		//Read text from file
		StringBuilder text = new StringBuilder();
		BufferedReader br = new BufferedReader(new FileReader(jsonFile));
		String line;

		while ((line = br.readLine()) != null) 
		{
			text.append(line);
			text.append('\n');
		}

		return new JSONArray(text.toString());
	}
	private Integer sum(Integer[] integerArray){
		int sum = 0;
		for(int i = 0; i < integerArray.length; i++){
			sum += integerArray[i];
		}
		return sum;
	}
	private Integer[] getFieldCounts(JSONArray bubbleVals) throws JSONException {
		Integer[] fieldCounts = new Integer[bubbleVals.length()];
		for(int i = 0; i < bubbleVals.length(); i++){
			Integer[] segmentCounts = getSegmentCounts(bubbleVals.getJSONObject(i));
			fieldCounts[i] = sum(segmentCounts);
		}
		return fieldCounts;
	}
	private Integer[] getSegmentCounts(JSONObject field) throws JSONException {

		JSONArray jArray = field.getJSONArray("segments");
		Integer[] bubbleCounts = new Integer[jArray.length()];

		for (int i = 0; i < jArray.length(); i++) {
			JSONArray bubbleValArray = jArray.getJSONObject(i).getJSONArray("bubbles");
			int numFilled = 0;
			for (int j = 0; j < bubbleValArray.length(); j++) {
				if( bubbleValArray.getJSONObject(j).getBoolean("value") ){
					numFilled++;
				}
			}
			bubbleCounts[i] = numFilled;
		}
		return bubbleCounts;
	}
}
