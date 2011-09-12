package com.bubblebot;

import java.util.Date;

import android.os.Handler;
import android.util.Log;

import com.google.fusiontables.ftclient.ClientLogin;
import com.google.fusiontables.ftclient.FtClient;

public class UploadDataThread implements Runnable {
	
	private String photoName;
	private Handler handler;
	
	private FtClient ftclient;
	private long tableid = 1465132;
	private String username = "<user>";
	private String password = "<password>";
	
	public UploadDataThread(Handler handler, String photoName){
		this.photoName = photoName;
		this.handler = handler;
	}
	@Override
	public void run() {
		// Initialize FTClient
		String token = ClientLogin.authorize(username, password);
		ftclient = new FtClient(token);
		// Generate INSERT statement
		StringBuilder insert = new StringBuilder();
		insert.append("INSERT INTO ");
		insert.append(tableid);
		insert.append(" (JSON, CollectionDate) VALUES ");
		insert.append("('");
		Log.i("mScan", JSONUtils.generateSimplifiedJSONString(photoName));
		insert.append(JSONUtils.generateSimplifiedJSONString(photoName));
		insert.append("', '");
		insert.append(new Date().toString());
		insert.append("')");
		ftclient.query(insert.toString());
		handler.sendEmptyMessage(0);
	}

}
