package com.bubblebot;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import android.content.SharedPreferences;
import android.content.res.AssetManager;
import android.os.Handler;
import android.util.Log;
/**
 * The RunSetup class is used to extract the assets while running on a separate thread so as to avoid locking up the UI.
 * It also does some version control stuff.
 */
public class RunSetup implements Runnable {
	
	public static final int version = 88;
	public static final boolean clearOldData = true;
	
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
			rmdir(new File(MScanUtils.appFolder));
			editor.clear();
		}
		
		// Create necessary directories if they do not exist
		(new File(MScanUtils.appFolder + MScanUtils.photoDir)).mkdirs();
		(new File(MScanUtils.appFolder + MScanUtils.alignedPhotoDir)).mkdirs();
		(new File(MScanUtils.appFolder + MScanUtils.jsonDir)).mkdirs();
		(new File(MScanUtils.appFolder + MScanUtils.markupDir)).mkdirs();
		
		try {
			//Creates a .nomedia file to prevent the images from showing up in he gallery.
			new File(MScanUtils.appFolder + ".nomedia").createNewFile();
			
			File trainingExamplesDir =  new File(MScanUtils.appFolder, MScanUtils.trainingExampleDir);
			File formTemplatesDir = new File(MScanUtils.appFolder, MScanUtils.templateDir);
			
			//Note: If clearOldData is true these directories will be deleted twice.
			rmdir(trainingExamplesDir);
			rmdir(formTemplatesDir);

			extractAssets(new File(""), new File(MScanUtils.appFolder));
			
			editor.putInt("version", version);
			
		} catch (IOException e) {
			// TODO: Terminate the app if this fails.
			e.printStackTrace();
			Log.i("mScan", "Extration Error");
		}
		editor.commit();
		handler.sendEmptyMessage(0);
	}
	/**
	 * Recursively copy all the assets in the specified assets directory to the specified output directory
	 * @param assetsDir
	 * @param outputDir
	 * @throws IOException
	 */
	protected void extractAssets(File assetsDir, File outputDir) throws IOException{

		outputDir.mkdirs();
		String[] assetNames = assets.list(assetsDir.toString());
		for(int i = 0; i < assetNames.length; i++){
			
			File nextAssetsDir = new File(assetsDir, assetNames[i]);
			File nextOutputDir = new File(outputDir, assetNames[i]);
			
			
			if(assets.list(nextAssetsDir.toString()).length == 0){
				copyAsset(nextAssetsDir, nextOutputDir);
			}
			else{
				extractAssets(nextAssetsDir, nextOutputDir);
			}
		}
	}
	/**
	 * Copy a single asset file to the specified directory.
	 * @param assetDir
	 * @param outputDir
	 * @throws IOException
	 */
	protected void copyAsset(File assetDir, File outputDir) throws IOException {
		
		Log.i("mScan", "Copying " + assetDir + " to " + outputDir.toString());
		
		InputStream fis = assets.open(assetDir.toString());

		outputDir.createNewFile();
		FileOutputStream fos = new FileOutputStream(outputDir);

		// Transfer bytes from in to out
		byte[] buf = new byte[1024];
		int len;
		while ((len = fis.read(buf)) > 0) {
			fos.write(buf, 0, len);
		}

		fos.close();
		fis.close();
	}
	/**
	 * Recursively remove all the files in a directory, then the directory.
	 * @param dir
	 */
	public void rmdir(File dir){
		
		if(dir.exists()){
			String[] files = dir.list();
			for(int i = 0; i < files.length; i++){
				File file = new File(dir, files[i]);
				Log.i("mScan", "Removing: " + file.toString());
				if(file.isDirectory()){
					rmdir(file);
				}
				else{
					file.delete();
				}
			}
			dir.delete();
		}
		else{
			Log.i("mScan", "Cound not remove directory, it does not exist:" + dir.toString());
		}
	}
}
