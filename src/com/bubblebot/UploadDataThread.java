//package com.bubblebot;
//
//import java.util.Date;
//
//import android.os.Handler;
//import android.util.Log;
//
//import com.google.fusiontables.ftclient.FtClient;
//
//public class UploadDataThread implements Runnable {
//	
//	private String photoName;
//	private Handler handler;
//	
//	private FtClient ftclient;
//	private long tableid = 1465132;
//	private String authToken;
//	
//	public UploadDataThread(Handler handler, String photoName, String authToken){
//		this.photoName = photoName;
//		this.handler = handler;
//		this.authToken = authToken;
//	}
//	//@Override
//	public void run() {
//		// Initialize FTClient
//		ftclient = new FtClient(authToken);
//		// Generate INSERT statement
//		StringBuilder insert = new StringBuilder();
//		insert.append("INSERT INTO ");
//		insert.append(tableid);
//		insert.append(" (JSON, CollectionDate) VALUES ");
//		insert.append("('");
//		Log.i("mScan", JSONUtils.generateSimplifiedJSONString(photoName));
//		insert.append(JSONUtils.generateSimplifiedJSONString(photoName));
//		insert.append("', '");
//		insert.append(new Date().toString());
//		insert.append("')");
//		String resultMsg = ftclient.query(insert.toString());
//		handler.sendEmptyMessage(resultMsg.startsWith("rowid") ? 1 : 0);//Sends 1 for success and 0 for failure
//	}
//
//}
