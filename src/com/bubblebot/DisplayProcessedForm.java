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
import android.net.NetworkInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.webkit.WebView;

/* DisplayProcessedForm activity
 * 
 * This activity displays the image of a processed form
 */
public class DisplayProcessedForm extends Activity {

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

		setTitle("Displaying " + photoName);
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
			
			final Account [] accounts = manager.getAccounts();
			
			if(accounts.length > 0){
				String[] names = new String[accounts.length];
				for (int i = 0; i < accounts.length; i++) {
					names[i] = accounts[i].name;
				}
				
				builder.setItems(names, new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int which) {
						gotAccount(manager, accounts[which]);
					}
				});
			}
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
				args.putString("reason", "Could not obtain authentication token from the account manager." +
										 "You might need to give the app permission to access your accounts," +
										 "check the pull-down menu tray for notifications.");
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
			//TODO: Put the upload stuff into another file/activity to keep things cleaner
			NetworkInfo neti = ((ConnectivityManager) this.getSystemService(Context.CONNECTIVITY_SERVICE)).getActiveNetworkInfo();
			if(neti != null && neti.isConnected()){
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
		case R.id.startOver:
			intent = new Intent(getApplication(), BubbleBot.class);
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