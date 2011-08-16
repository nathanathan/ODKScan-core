package com.bubblebot;

import java.io.File;
import java.lang.reflect.Method;
import java.util.Date;

import android.util.Log;
import android.view.View;
import android.webkit.WebSettings;
import android.webkit.WebView;

public class MScanUtils {
	//Prevent instantiations
	private MScanUtils(){}
	
    public static void displayImageInWebView(WebView myWebView, String imagePath){
		myWebView.getSettings().setCacheMode(WebSettings.LOAD_NO_CACHE);
		myWebView.getSettings().setBuiltInZoomControls(true);
		myWebView.getSettings().setDefaultZoom(WebSettings.ZoomDensity.FAR);
		myWebView.setVisibility(View.VISIBLE);
		
		// HTML is used to display the image.
		String html =   "<body bgcolor=\"Black\"><center> <img src=\"file:///" +
						imagePath +
						// Appending the time stamp to the filename is a hack to prevent caching.
						"?" + new Date().getTime() + 
						"\" width=\"500\" > </center></body>";
		       
		myWebView.loadDataWithBaseURL("file:///unnecessairy/",
								 html, "text/html", "utf-8", "");
    }
    //Check if the given class has the given method.
    //Useful for dealing with android comparability issues (see getUsableSpace)
	public static <T> boolean hasMethod(Class<T> c, String method){
		Method methods[] = c.getMethods();
		for(int i = 0; i < methods.length; i++){
			if(methods[i].getName().equals(method)){
				return true;
			}
		}
		return false;
	}
	public static long getUsableSpace(String folder) {
		if(MScanUtils.hasMethod(File.class, "getUsableSpace")){
			return (new File(folder)).getUsableSpace();
		}
		return -1;
	}
	public static void clearDir(String dirPath){
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
