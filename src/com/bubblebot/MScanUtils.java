package com.bubblebot;

import java.io.File;
import java.lang.reflect.Method;
import java.util.Date;

import android.view.View;
import android.webkit.WebSettings;
import android.webkit.WebView;
/*
 * MScanUtils contains methods and data shared across the mScan application.
 */
public class MScanUtils {
	//Prevent instantiations
	private MScanUtils(){}

	public static final boolean DebugMode = false;
	
	public static final String appFolder = "/sdcard/mScan/";
	public static final String photoDir = "photos/";
	public static final String alignedPhotoDir = "alignedPhotos/";
	public static final String jsonDir = "jsonOut/";
	public static final String markupDir = "markedupPhotos/";
	public static final String trainingExampleDir = "training_examples/";
	public static final String templateDir = "form_templates/";

	public static String getPhotoPath(String photoName){
		return appFolder + photoDir + photoName + ".jpg";
	}
	public static String getAlignedPhotoPath(String photoName){
		return appFolder + alignedPhotoDir + photoName + ".jpg";
	}
	public static String getJsonPath(String photoName){
		return appFolder + jsonDir + photoName + ".json";
	}
	public static String getMarkedupPhotoPath(String photoName){
		return appFolder + markupDir + photoName + ".jpg";
	}
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
			return new File(folder).getUsableSpace();
		} 
		return -1;
	}
	public static Integer sum(Integer[] integerArray){
		int sum = 0;
		for(int i = 0; i < integerArray.length; i++){
			sum += integerArray[i];
		}
		return sum;
	}
}
