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

	TextView data;
	Button button;
	File dir = new File("/sdcard/mScan/");

	String filename = "";

	// Set up the UI
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		//

		super.onCreate(savedInstanceState);

		


		/*
       super.onCreate(savedInstanceState);
       setContentView(R.layout.processed_data);

       Bundle extras = getIntent().getExtras(); 
       if(extras !=null)
       {
    	   filename = extras.getString("file") + ".j";
       }
		 */
		//read in captured data from textfile


		try {

			String[] bubbleCounts = getBubbleCounts("sdcard/BubbleBot/output.json");

			setListAdapter(new ArrayAdapter<String>(this, R.layout.list_item, bubbleCounts));

			ListView lv = getListView();
			lv.setTextFilterEnabled(true);

			lv.setOnItemClickListener(new OnItemClickListener() {
				public void onItemClick(AdapterView<?> parent, View view,
						int position, long id) {
					// When clicked, show a toast with the TextView text
					Toast.makeText(getApplicationContext(), ((TextView) view).getText(),
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
		/*
		data = (TextView) findViewById(R.id.text);
		data.setText(outtext);
		
       button = (Button) findViewById(R.id.button);
       button.setOnClickListener(new View.OnClickListener() {  
	       public void onClick(View v) {
	    	   //Back to the menu
	    	   Intent intent = new Intent(getApplication(), BubbleBot.class);
	    	   intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
   			   startActivity(intent); 
	       }
	    });*/ 
	}

	private String[] getBubbleCounts(String bvFilename) throws JSONException, IOException {
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


		JSONArray jArray1 = new JSONArray(text.toString());
		JSONArray jArray = jArray1.getJSONObject(0).getJSONArray("segments");
		String[] bubbleCounts = new String[jArray.length()];

		for (int i = 0; i < jArray.length(); i++) {
			JSONArray bubbleValArray = jArray.getJSONObject(i).getJSONArray("bubble_vals");
			int numFilled = 0;
			for (int j = 0; j < bubbleValArray.length(); j++) {
				if(bubbleValArray.getBoolean(j)){
					numFilled++;
				}
			}
			bubbleCounts[i] = "Filled Bubbles: " + numFilled;
		}

		return bubbleCounts;
	}
}
