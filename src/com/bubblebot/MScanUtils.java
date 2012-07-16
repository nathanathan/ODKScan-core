package com.bubblebot;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Date;

import android.view.View;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.os.StatFs;
/**
 * MScanUtils contains methods and data shared across the mScan application.
 */
public class MScanUtils {
	//Prevent instantiations
	private MScanUtils(){}

	public static final boolean DebugMode = true;
	
	public static final String appFolder = "/sdcard/ODKScan/";
	
	public static String getOutputDirPath() {
		return appFolder + "output/";
	}
	public static String getOutputPath(String photoName){
		return getOutputDirPath() + photoName + "/";
	}
	public static String getPhotoPath(String photoName){
		return getOutputPath(photoName) + "photo.jpg";
	}
	public static String getAlignedPhotoPath(String photoName){
		return getOutputPath(photoName) + "aligned.jpg";
	}
	public static String getJsonPath(String photoName){
		return getOutputPath(photoName) + "output.json";
	}
	public static String getMarkedupPhotoPath(String photoName){
		return getOutputPath(photoName) + "markedup.jpg";
	}
	public static String getTemplateDirPath() {
		return appFolder + "form_templates/";
	}
	public static String getTrainingExampleDirPath() {
		return appFolder + "training_examples/";
	}
	public static String getCalibPath() {
		return appFolder + "camera.yml";
	}
	public static String getFormViewHTMLDir() {
		return appFolder + "transcription/";
	}
    public static void displayImageInWebView(WebView myWebView, String imagePath){
		myWebView.getSettings().setCacheMode(WebSettings.LOAD_NO_CACHE);
		myWebView.getSettings().setBuiltInZoomControls(true);
		myWebView.getSettings().setDefaultZoom(WebSettings.ZoomDensity.FAR);
		myWebView.setVisibility(View.VISIBLE);
		
		// HTML is used to display the image.
		String html =   "<body bgcolor=\"White\">" +
						//"<p>" +  + "</p>" +
						"<center> <img src=\"file:///" +
						imagePath +
						// Appending the time stamp to the filename is a hack to prevent caching.
						"?" + new Date().getTime() + 
						"\" width=\"500\" > </center></body>";
		       
		myWebView.loadDataWithBaseURL("file:///unnecessairy/",
								 html, "text/html", "utf-8", "");
    }
//    /**
//     * Check if the given class has the given method.
//     * Useful for dealing with android comparability issues (see getUsableSpace)
//     * @param c
//     * @param method
//     * @return
//     */
//	public static <T> boolean hasMethod(Class<T> c, String method){
//		Method methods[] = c.getMethods();
//		for(int i = 0; i < methods.length; i++){
//			if(methods[i].getName().equals(method)){
//				return true;
//			}
//		}
//		return false;
//	}
	/**
	 * Returns the available space in bytes.
	 * @param folder
	 * @return
	 */
	public static long getUsableSpace(String folder) {
		/*
		if(MScanUtils.hasMethod(File.class, "getUsableSpace")){
			return new File(folder).getUsableSpace();
			return 999999999;
		} 
		return -1;
		*/
		StatFs sfs = new StatFs(folder);
		return sfs.getAvailableBlocks() *  sfs.getBlockSize();
	}
	//Replace this with code from parseFileToJSONObject
    private static String readFileAsString(String filePath)
    		throws java.io.IOException{
        StringBuffer fileData = new StringBuffer(1000);
        BufferedReader reader = new BufferedReader(
                new FileReader(filePath));
        char[] buf = new char[1024];
        int numRead=0;
        while((numRead=reader.read(buf)) != -1){
            String readData = String.valueOf(buf, 0, numRead);
            fileData.append(readData);
            buf = new char[1024];
        }
        reader.close();
        return fileData.toString();
    }
    
	public static String getTemplatePath(String photoName) {
		if(new File(getOutputPath(photoName)).exists()){
			String templateValueFile = getOutputPath(photoName) + "template";
			if(new File(templateValueFile).exists()){
				try {
					return readFileAsString(templateValueFile);
				} catch (IOException e) {
					return null;
				}
			}
		}
		return null;
	}
	public static void setTemplatePath(String photoName, String templatePath) throws IOException {
		String templateValueFile = getOutputPath(photoName) + "template";
		if(new File(templateValueFile).createNewFile()){
			BufferedWriter out = new BufferedWriter(new FileWriter(templateValueFile));
			out.write(templatePath);
			out.close();
		}
	}
}