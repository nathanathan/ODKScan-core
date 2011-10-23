package com.bubblebot;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.os.Handler;
import android.util.Log;
/*
 * The RunSetup class is used to extract the assets on a separate thread.
 * It also does some version control stuff.
 */
public class RunSetup implements Runnable {
	
	public static final int version = 60;
	public static final boolean clearOldData = false;
	
	private SharedPreferences settings;
	private AssetManager assets;
	private Handler handler;
	
	public RunSetup(Handler handler, SharedPreferences settings, AssetManager assets){
		this.handler = handler;
		this.settings = settings;
		this.assets = assets;
	}
	public void run() {

		SharedPreferences.Editor editor = settings.edit();
		
		if(clearOldData){
			clearDir(MScanUtils.appFolder + MScanUtils.photoDir);
			clearDir(MScanUtils.appFolder + MScanUtils.alignedPhotoDir);
			clearDir(MScanUtils.appFolder + MScanUtils.jsonDir);
			clearDir(MScanUtils.appFolder + MScanUtils.markupDir);
			editor.clear();
		}
		
		// Set up directories if they do not exist
		(new File(MScanUtils.appFolder + MScanUtils.photoDir)).mkdirs();
		(new File(MScanUtils.appFolder + MScanUtils.alignedPhotoDir)).mkdirs();
		(new File(MScanUtils.appFolder + MScanUtils.jsonDir)).mkdirs();
		(new File(MScanUtils.appFolder + MScanUtils.markupDir)).mkdirs();
		
		try {
			//Create a .nomedia file to prevent the images from showing up in he gallery.
			//This might not be working... hiding the folder might work.
			new File(MScanUtils.appFolder + ".nomedia").createNewFile();
			
			String traininExamplesPath =  MScanUtils.appFolder + MScanUtils.trainingExampleDir;
			String formTemplatesPath = MScanUtils.appFolder + MScanUtils.templateDir;
			
			clearDir(traininExamplesPath);
			clearDir(formTemplatesPath);
			
			//TODO: make the command below work.
			//extractAssets("/", MScanUtils.appFolder);
			extractAssets(MScanUtils.trainingExampleDir, traininExamplesPath);
			extractAssets(MScanUtils.templateDir, formTemplatesPath);
			copyAsset("camera.yml", MScanUtils.appFolder + "camera.yml");
			editor.putInt("version", version);
			
		} catch (IOException e) {
			// TODO: Terminate the app if this fails.
			e.printStackTrace();
			Log.i("mScan", "Extration Error");
		}
		editor.commit();
		handler.sendEmptyMessage(0);
	}
	// This method copies the files within a given directory (with path relative to the assets folder).
	protected void extractAssets(String assetsDir, String outputPath) throws IOException{
		
		assetsDir = removeSuffix(assetsDir, "/");
		outputPath = removeSuffix(outputPath, "/");
		
		File dir = new File(outputPath);
		dir.mkdirs();
		String[] assetNames = assets.list(assetsDir);
		for(int i = 0; i < assetNames.length; i++){
			if(assets.list(assetsDir + "/" + assetNames[i]).length == 0){
				copyAsset(assetsDir + "/" + assetNames[i], outputPath + "/" + assetNames[i]);
			}
			else{
				extractAssets(assetsDir + "/" + assetNames[i], outputPath + "/" + assetNames[i]);
			}
		}
	}
	protected String removeSuffix(String s, String suffix){
		if(s.endsWith(suffix)){
			return s.substring(0, s.length() - suffix.length());
		}
		return s;
	}
	protected void copyAsset(String assetLoc, String outputLoc) throws IOException {
		//Log.i("mScan", "copy from " + assetLoc + " to " + outputLoc);
		InputStream fis = assets.open(assetLoc);
		File trainingExampleFile = new File(outputLoc);
		trainingExampleFile.createNewFile();
		FileOutputStream fos = new FileOutputStream(trainingExampleFile);

		// Transfer bytes from in to out
		byte[] buf = new byte[1024];
		int len;
		while ((len = fis.read(buf)) > 0) {
			fos.write(buf, 0, len);
		}

		fos.close();
		fis.close();
	}
	public void clearDir(String dirPath){
		
		dirPath = removeSuffix(dirPath, "/");
		
		File dir = new File(dirPath);
		if(dir.exists()){
			String[] files = dir.list();
			for(int i = 0; i < files.length; i++){
				File file = new File(dirPath + "/" + files[i]);
				Log.i("mScan", dirPath + "/" + files[i]);
				if(file.isDirectory()){
					clearDir(dirPath + "/" + files[i]);
				}
				else{
					file.delete();
				}
			}
			dir.delete();
		}
		else{
			Log.i("mScan", "clearDir error");
		}
	}
}
