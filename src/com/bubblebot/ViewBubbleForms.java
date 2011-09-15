package com.bubblebot;

import java.io.File;
import java.io.FilenameFilter;
import java.util.Date;

import com.google.api.client.googleapis.extensions.android2.auth.GoogleAccountManager;

import android.accounts.Account;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ListActivity;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.TextView;

/* ViewBubbleForms activity
 * 
 * This activity displays a list of processed forms for viewing
 */
public class ViewBubbleForms extends ListActivity {

	private String [] photoNames;
	private ArrayAdapter<String> myAdapter;

	// Initialize the application  
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		File dir = new File(MScanUtils.appFolder + MScanUtils.photoDir);	

		photoNames = dir.list(new FilenameFilter() {
			public boolean accept (File dir, String name) {
				if (new File(dir,name).isDirectory())
					return false;
				return name.toLowerCase().endsWith(".jpg");
			}
		});


		for(int i = 0; i < photoNames.length; i++){
			photoNames[i] = photoNames[i].substring(0, photoNames[i].lastIndexOf(".jpg"));
		}

		myAdapter = new ArrayAdapter<String>(this, R.layout.filename_list_item, photoNames) {
			@Override
			public View getView(int position, View convertView, ViewGroup parent) {

				LinearLayout view = (convertView != null) ? (LinearLayout) convertView : createView(parent);

				String photoName = photoNames[position];

				TextView photoStatus = (TextView) view.findViewById(R.id.photoStatus);
				if (new File(MScanUtils.getJsonPath(photoName)).exists()) {
					photoStatus.setTextColor(Color.parseColor("#00FF00"));
				}
				else if(new File(MScanUtils.getAlignedPhotoPath(photoName)).exists()){
					photoStatus.setTextColor(Color.parseColor("#FFFF00"));
				}
				else{
					photoStatus.setTextColor(Color.parseColor("#FF0000"));
				}

				TextView nameView = (TextView) view.findViewById(R.id.photoname);
				nameView.setText(photoName);

				TextView type = (TextView) view.findViewById(R.id.createdTime);
				type.setText(new Date(new File(MScanUtils.getPhotoPath(photoName)).lastModified()).toString());

				return view;
			}

			private LinearLayout createView(ViewGroup parent) {
				LinearLayout item = (LinearLayout) getLayoutInflater().inflate(R.layout.filename_list_item,
						parent, false);
				return item;
			}
		};

		setListAdapter(myAdapter);

		ListView lv = getListView();

		lv.setOnItemClickListener(new OnItemClickListener() {
			public void onItemClick(AdapterView<?> parent, View view,
					int position, long id) {

				String photoName = photoNames[position];

				if (new File(MScanUtils.getJsonPath(photoName)).exists()) {
					Intent intent = new Intent(getApplication(), DisplayProcessedForm.class);
					intent.putExtra("photoName", photoName);
					startActivity(intent); 
				}
				else if(new File(MScanUtils.getAlignedPhotoPath(photoName)).exists()){
					Intent intent = new Intent(getApplication(), AfterPhotoTaken.class);
					intent.putExtra("photoName", photoName);
					intent.putExtra("preAligned", true);
					startActivity(intent); 
				}
			}
		});

	}
	@Override
	public void onResume(){
		super.onResume();
		myAdapter.notifyDataSetChanged();
	}
}
