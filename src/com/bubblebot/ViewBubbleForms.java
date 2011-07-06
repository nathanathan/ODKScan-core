package com.bubblebot;

import java.io.File;
import java.io.FilenameFilter;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.LinearLayout;

/* ViewBubbleForms activity
 * 
 * This activity displays a list of processed forms for viewing
 */
public class ViewBubbleForms extends Activity {
	
	// Initialize the application  
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.view_bubble_forms);
		LinearLayout layout = (LinearLayout) findViewById(R.id.ViewFormsLayout);
		
		File dir = new File("/sdcard/mScan");		
		final String [] filenames = dir.list(new FilenameFilter() {
			public boolean accept (File dir, String name) {
				if (new File(dir,name).isDirectory())
					return false;
				return name.toLowerCase().endsWith(".jpg");
			}
		});
		
		// Add the list of processed form filenames to the UI
		for(int i = 0; i < filenames.length; i++) {
			final Button button = new Button(this);
			final String fname = filenames[i];
			LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(LayoutParams.FILL_PARENT, 100);
			layoutParams.setMargins(3, 3, 3, 3);
			button.setLayoutParams(layoutParams);
			button.setText(fname);
			
		    button.setOnClickListener(new View.OnClickListener() {
		       public void onClick(View v) {
		    	   //Find a way to pass in the filename for the relevant image - displaying a set image for now
		    	   Intent intent = new Intent(getApplication(), DisplayProcessedForm.class);
		    	   intent.putExtra("file", fname);
	   			   startActivity(intent); 
		       }
		    });
			layout.addView(button);
		}		
	}

	@Override
	protected void onPause() {
		super.onPause();
	}

	@Override
	protected void onResume() {
		super.onResume();
		
	}
}
