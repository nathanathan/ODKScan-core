package com.bubblebot;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

import android.app.ListActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
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

		Bundle extras = getIntent().getExtras();
		if (extras == null) return;
		String photoName = extras.getString("photoName");
		
		try {
			
			JSONObject bubbleVals = parseFileToJSONObject( MScanUtils.getJsonPath(photoName) );
			
			//TODO: replace this with some kind of expanding menu thing so that clicking on a field
			//		allows users to see things broken down by segment.
			
			String[] fields = getFields(bubbleVals);
			
			setListAdapter(new ArrayAdapter<String>(this, R.layout.list_item, fields));

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
			Log.i("mScan", "JSON parsing problems.");
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
	    	Log.i("mScan", "IO problems.");
			e.printStackTrace();
		}
	}
	private JSONObject parseFileToJSONObject(String bvFilename) throws JSONException, IOException {
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

		return new JSONObject(text.toString());
	}
	public JSONArray parseFileToJSONArray(String bvFilename) throws JSONException, IOException {
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
	public Integer sum(Integer[] integerArray){
		int sum = 0;
		for(int i = 0; i < integerArray.length; i++){
			sum += integerArray[i];
		}
		return sum;
	}
	private String[] getFields(JSONObject bubbleVals) throws JSONException {
		JSONArray fields = bubbleVals.getJSONArray("fields");
		String[] fieldString = new String[fields.length()];
		for(int i = 0; i < fields.length(); i++){
			Integer[] segmentCounts = getSegmentCounts(fields.getJSONObject(i));
			fieldString[i] = fields.getJSONObject(i).getString("label") + " : \n" + sum(segmentCounts);
		}
		return fieldString;
	}
	public Integer[] getFieldCounts(JSONObject bubbleVals) throws JSONException {
		JSONArray fields = bubbleVals.getJSONArray("fields");
		Integer[] fieldCounts = new Integer[fields.length()];
		for(int i = 0; i < fields.length(); i++){
			Integer[] segmentCounts = getSegmentCounts(fields.getJSONObject(i));
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
