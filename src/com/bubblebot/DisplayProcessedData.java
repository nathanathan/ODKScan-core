package com.bubblebot;

import java.io.IOException;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.LinearLayout;
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

	private static final int SAVE_CHANGES = 2;
	
	ArrayAdapter<JSONObject> arrayAdapter;
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
			
			arrayAdapter = new ArrayAdapter<JSONObject>(this, R.layout.data_list_item, fields) {
				@Override
				public View getView(int position, View convertView, ViewGroup parent) {
					
					LinearLayout view = (convertView != null) ? (LinearLayout) convertView : createView(parent);
					
					final JSONObject field = fields[position];
					
					TextView fieldName = (TextView) view.findViewById(R.id.fieldName);
					TextView fieldCount = (TextView) view.findViewById(R.id.fieldCount);
					try {
						fieldName.setText(field.getString("label"));
						//TODO: Figure out a better way to display the data so that fields can have multiple segment types.
						if( field.getJSONArray("segments").getJSONObject(0).getString("type").equals("bubble") ){
							Integer[] segmentCounts = JSONUtils.getSegmentCounts(field);
							fieldCount.setText("" + MScanUtils.sum(segmentCounts));
						}
						else{
							fieldCount.setText(field.getJSONArray("segments").getJSONObject(0).getString("type"));
						}
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
					// TODO: replace this with some kind of expanding menu thing so that clicking on a field
					//		 allows users to see things broken down by segment.
					// When clicked, show a toast with the TextView text
					/*
					try {
						fields[position].getJSONArray("segments").getJSONObject(0).put("type", "hello");
					} catch (JSONException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					*/
					showSegmentValues(position);

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
	protected void showSegmentValues(final int position) {
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setTitle("Segment values");
		try{
			JSONArray segments = fields[position].getJSONArray("segments");
			String[] vals = new String[segments.length()];
			for (int i = 0; i < vals.length; i++) {
				if(fields[position].getJSONArray("segments").getJSONObject(i).has("value")){
					Log.i("mScan", "test");
				}
				Log.i("mScan", "test2");
				vals[i] = segments.getJSONObject(i).optString("value");
			}
			builder.setItems(vals, new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int which) {
					showSegmentEditor(position, which);
				}
			});
			builder.show();
		} catch (JSONException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	protected void showSegmentEditor(final int fidx, final int sidx) {
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setTitle("Set value");

		// Set an EditText view to get user input 
		final EditText input = new EditText(this);
		builder.setView(input);
		builder.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int whichButton) {
				String value = input.getText().toString();
				try {
					fields[fidx].getJSONArray("segments").getJSONObject(sidx).put("value", value);
					Log.i("mScan", "fidx: "+ fidx + "\t sidx: " + sidx);
					arrayAdapter.notifyDataSetChanged();
					dialog.dismiss();
				} catch (JSONException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		});
		builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int whichButton) {
				dialog.dismiss();
			}
		});
		builder.show();
	}
	@Override
	public void onBackPressed() {
		showDialog(SAVE_CHANGES);
	}
	@Override
	protected Dialog onCreateDialog(int id, final Bundle args) {
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		
		switch (id) {
		case SAVE_CHANGES:
			builder.setTitle("Save Changes?");
			builder.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int whichButton) {
					dialog.dismiss();
					superBackPress();
				}
			});
			builder.setNegativeButton("No", new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int whichButton) {
					dialog.dismiss();
					superBackPress();
				}
			});
			break;
		default:
			return null;
		}
		return builder.create();
	}
	protected void superBackPress() {
		super.onBackPressed();
	}
}
