package com.bubblebot;

import android.accounts.Account;
import android.accounts.AccountManager;
import com.google.api.client.googleapis.extensions.android2.auth.GoogleAccountManager;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.webkit.WebView;
import android.widget.TextView;

/* DisplayProcessedForm activity
 * 
 * This activity displays the image of a processed form
 */
public class DisplayProcessedForm extends Activity {

	private TextView text;
	private String photoName;

	private ProgressDialog pd;
	private static final int DIALOG_ACCOUNTS = 1110011;
	private static final int DIALOG_FAILURE = 11011011;
	private static final int DIALOG_SUCCESS = 11001111;
	private static final String AUTH_TOKEN_TYPE = "fusiontables";
	

	// Set up the UI and load the processed image
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.processed_form);

		Bundle extras = getIntent().getExtras(); 
		if (extras != null) {
			photoName = extras.getString("photoName");
		}

		text = (TextView) findViewById(R.id.text);
		text.setText("Displaying " + photoName);

		MScanUtils.displayImageInWebView((WebView)findViewById(R.id.webview2),
				MScanUtils.getMarkedupPhotoPath(photoName));
	}
	@Override
	public void onAttachedToWindow() {
		super.onAttachedToWindow();
		//Show the options menu when the activity is started (so people know it exists)
		openOptionsMenu();
	}
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.mscan_menu, menu);
		return true;
	}
	@Override
	protected Dialog onCreateDialog(int id, Bundle args) {
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		switch (id) {
		case DIALOG_ACCOUNTS:
			builder.setTitle("Select a Google account");
			final GoogleAccountManager manager = new GoogleAccountManager(this);
			final Account[] accounts = manager.getAccounts();
			final int size = accounts.length;
			String[] names = new String[size];
			for (int i = 0; i < size; i++) {
				names[i] = accounts[i].name;
			}
			builder.setItems(names, new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int which) {
					gotAccount(manager, accounts[which]);
				}
			});
			break;
		case DIALOG_SUCCESS:
			builder.setTitle("Success!")
		       .setCancelable(false)
		       .setPositiveButton("Hooray!", new DialogInterface.OnClickListener() {
		           public void onClick(DialogInterface dialog, int id) {
		                dialog.dismiss();
		           }
		       });
			break;
		case DIALOG_FAILURE:
			builder.setTitle("Failure :(")
			   .setMessage(args.getString("reason"))
		       .setCancelable(false)
		       .setPositiveButton("OK", new DialogInterface.OnClickListener() {
		           public void onClick(DialogInterface dialog, int id) {
		                dialog.dismiss();
		           }
		       });
			break;
		default:
			return null;
		}
		return builder.create();
	}
	private void gotAccount(final GoogleAccountManager manager, final Account account) {

		pd = ProgressDialog.show(this, "", "Uploading Data...", true, true);

		try {
			final Bundle bundle = 
				manager.manager.getAuthToken(account, AUTH_TOKEN_TYPE, true, null, null).getResult();
			
			if (bundle.containsKey(AccountManager.KEY_AUTHTOKEN)) {
				new Thread(new UploadDataThread(handler, photoName, 
						bundle.getString(AccountManager.KEY_AUTHTOKEN))).start();
			}
			else{
				pd.dismiss();
				Bundle args = new Bundle();
				args.putString("reason", "Could not obtain authentication token from the account manager.");
				showDialog(DIALOG_FAILURE, args);
			}
		} catch (Exception e) {
			pd.dismiss();
			Bundle args = new Bundle();
			args.putString("reason", "Exception when getting auth token.");
			showDialog(DIALOG_FAILURE, args);
		}
	}
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle item selection
		Intent intent;
		switch (item.getItemId()) {
		case R.id.displayData:
			intent = new Intent(getApplication(), DisplayProcessedData.class);
			intent.putExtra("photoName", photoName);
			startActivity(intent); 
			return true;
		case R.id.uploadData:
			ConnectivityManager cm = (ConnectivityManager) this.getSystemService(Context.CONNECTIVITY_SERVICE);
			if(cm.getActiveNetworkInfo().isConnected()){
				showDialog(DIALOG_ACCOUNTS);
			}
			else{
				Bundle args = new Bundle();
				args.putString("reason", "Connectivity is not available.");
				showDialog(DIALOG_FAILURE, args);
			}
			return true;
		case R.id.scanNewForm:
			intent = new Intent(getApplication(), BubbleCollect2.class);
			startActivity(intent);
			return true;
		default:
			return super.onOptionsItemSelected(item);
		}
	}
	private Handler handler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			pd.dismiss();
			if(msg.what == 1){
				showDialog(DIALOG_SUCCESS);
			}
			else{
				Bundle args = new Bundle();
				args.putString("reason", "Count not upload.");
				showDialog(DIALOG_FAILURE, args);
			}
		}
	};
}