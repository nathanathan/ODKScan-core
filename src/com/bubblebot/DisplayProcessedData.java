package com.bubblebot;

import java.io.IOException;

import android.app.ListActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONException;
import org.json.JSONObject;

/* BubbleProcessData activity
 * 
 * This activity displays the digitized information of a processed form
 */
public class DisplayProcessedData  extends ListActivity {

	private JSONObject [] fields;
	// Set up the UI
	@Override
	protected void onCreate(Bundle savedInstanceState) {

		super.onCreate(savedInstanceState);

		Bundle extras = getIntent().getExtras();
		
		if (extras == null) return;
		
		String photoName = extras.getString("photoName");
		
		try {
			JSONObject bubbleVals = JSONUtils.parseFileToJSONObject( MScanUtils.getJsonPath(photoName) );
			
			fields = JSONUtils.JSONArray2Array( bubbleVals.getJSONArray("fields") );
			
			ArrayAdapter<JSONObject> arrayAdapter = new ArrayAdapter<JSONObject>(this, R.layout.data_list_item, fields) {
				@Override
				public View getView(int position, View convertView, ViewGroup parent) {
					
					LinearLayout view = (convertView != null) ? (LinearLayout) convertView : createView(parent);
					
					final JSONObject field = fields[position];
					
					TextView fieldName = (TextView) view.findViewById(R.id.fieldName);
					TextView fieldCount = (TextView) view.findViewById(R.id.fieldCount);
					try {
						fieldName.setText(field.getString("label"));
						Integer[] segmentCounts = JSONUtils.getSegmentCounts(field);
						fieldCount.setText("" + MScanUtils.sum(segmentCounts));
					} catch (JSONException e) {
						fieldName.setText( "ERROR" );
						fieldCount.setText("NA");
					}
					return view;
				}

				private LinearLayout createView(ViewGroup parent) {
					LinearLayout item = (LinearLayout) getLayoutInflater().inflate(R.layout.data_list_item,
							parent, false);
					return item;
				}
			};
			
			setListAdapter(arrayAdapter);

			ListView lv = getListView();
			
			lv.setOnItemClickListener(new OnItemClickListener() {
				public void onItemClick(AdapterView<?> parent, View view,
										int position, long id) {
					//TODO: replace this with some kind of expanding menu thing so that clicking on a field
					//		allows users to see things broken down by segment.
					// When clicked, show a toast with the TextView text
					Toast.makeText(getApplicationContext(), ""+position,
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
}
