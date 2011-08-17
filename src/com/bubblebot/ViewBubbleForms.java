package com.bubblebot;

import java.io.File;
import java.io.FilenameFilter;
import android.app.ListActivity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.AdapterView.OnItemClickListener;

/* ViewBubbleForms activity
 * 
 * This activity displays a list of processed forms for viewing
 */
public class ViewBubbleForms extends ListActivity {

	// Initialize the application  
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		File dir = new File(MScanUtils.appFolder + MScanUtils.photoDir);		
		final String [] filenames = dir.list(new FilenameFilter() {
			public boolean accept (File dir, String name) {
				if (new File(dir,name).isDirectory())
					return false;
				return name.toLowerCase().endsWith(".jpg");
			}
		});

		setListAdapter(new ArrayAdapter<String>(this, R.layout.list_item, filenames));

		ListView lv = getListView();
		lv.setTextFilterEnabled(true);

		lv.setOnItemClickListener(new OnItemClickListener() {
			public void onItemClick(AdapterView<?> parent, View view,
					int position, long id) {

				Intent intent = new Intent(getApplication(), DisplayProcessedForm.class);
				String filename = filenames[(int) id];
				Log.i("mScan", filename.substring(0, filename.lastIndexOf(".jpg")));
				intent.putExtra("photoName", filename.substring(0, filename.lastIndexOf(".jpg")));
				startActivity(intent); 
			}
		});
	}
}
