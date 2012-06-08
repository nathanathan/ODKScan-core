package com.bubblebot;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.kxml2.io.KXmlParser;
import org.kxml2.io.KXmlSerializer;
import org.kxml2.kdom.Document;
import org.kxml2.kdom.Element;
import org.kxml2.kdom.Node;
import org.xmlpull.v1.XmlPullParserException;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.ContentValues;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
/**
 * This activity converts mScan JSON files into XML files
 * that can be used with ODK and launches ODK Collect
 * @author nathan
 *
 */
public class MScan2CollectActivity extends Activity {
	
    private static final String COLLECT_FORMS_URI_STRING =
            "content://org.odk.collect.android.provider.odk.forms/forms";
    private static final Uri COLLECT_FORMS_CONTENT_URI =
            Uri.parse(COLLECT_FORMS_URI_STRING);
    private static final String COLLECT_INSTANCES_URI_STRING =
            "content://org.odk.collect.android.provider.odk.instances/instances";
    private static final Uri COLLECT_INSTANCES_CONTENT_URI =
            Uri.parse(COLLECT_INSTANCES_URI_STRING);
    private static final DateFormat COLLECT_INSTANCE_NAME_DATE_FORMAT =
            new SimpleDateFormat("yyyy-MM-dd_kk-mm-ss");

	private static final String LOG_TAG = "mScan";
	
	private String photoName;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		try {
			//Initialize the intent that will start collect and use it to see if collect is installed.
		    Intent intent = new Intent();
		    intent.setFlags(Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
		    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		    intent.setComponent(new ComponentName("org.odk.collect.android",
		            "org.odk.collect.android.activities.FormEntryActivity"));
		    PackageManager packMan = getPackageManager();
		    if(packMan.queryIntentActivities(intent, 0).size() == 0){
		    	throw new Exception("ODK Collect was not found on this device.");
		    }
			
			//Read in parameters from the intent's extras.
			Bundle extras = getIntent().getExtras();

			if(extras == null){ throw new Exception("No parameters specified"); }
			
			photoName = extras.getString("photoName");
			String jsonOutPath = MScanUtils.getJsonPath(photoName);
			String templatePath = extras.getString("templatePath");

			if(jsonOutPath == null){ throw new Exception("jsonOutPath is null"); }
			if(templatePath == null){ throw new Exception("Could not identify template."); }
			
			Log.i(LOG_TAG,"jsonOutPath : " + jsonOutPath);
			Log.i(LOG_TAG,"templatePath : " + templatePath);
			
			String templateName = new File(templatePath).getName();
			String jsonPath = new File(templatePath, "template.json").getPath();
			String xFormPath = new File(templatePath, templateName + ".xml").getPath();

			//////////////
			Log.i(LOG_TAG, "Checking if there is no xform or the xform is out of date.");
			//////////////
			File xformFile = new File(xFormPath);
			if( !xformFile.exists() || 
				new File(jsonPath).lastModified() > xformFile.lastModified()){
				//////////////
				Log.i(LOG_TAG, "Unregistering any existing old versions of xform.");
				//////////////
			    String [] deleteArgs = { templateName };
		        int deleteResult = getContentResolver().delete(COLLECT_FORMS_CONTENT_URI, "jrFormId like ?", deleteArgs);
		        Log.w(LOG_TAG, "Removing " + deleteResult + " rows.");
				//////////////
				Log.i(LOG_TAG, "Creating new xform.");
				//////////////
			    buildXForm(jsonPath, xFormPath, templateName, templateName);
			}
			String jrFormId = verifyFormInCollect(xFormPath, templateName);
			//////////////
			Log.i(LOG_TAG, "Checking if the form instance is already registered with collect.");
			//////////////
			int instanceId;
		    String instanceName = templateName
		    		+ '_' + photoName + '_'
		    		+ COLLECT_INSTANCE_NAME_DATE_FORMAT.format(new Date(new File(templatePath).lastModified()));
		    String instancePath = "/sdcard/odk/instances/" + instanceName + "/";
		    (new File(instancePath)).mkdirs();
		    String instanceFilePath = instancePath + instanceName + ".xml";
			String selection = "instanceFilePath = ?";
	        String[] selectionArgs = { instanceFilePath };
	        Cursor c = getContentResolver().query(COLLECT_INSTANCES_CONTENT_URI, null, selection, selectionArgs, null);
	        //Log.i(LOG_TAG, Arrays.toString(c.getColumnNames()));
	    	if(c.moveToFirst()){
	    		//////////////
				Log.i(LOG_TAG, "Registered odk instance found.");
				//////////////
	    		instanceId = c.getInt(c.getColumnIndex("_id"));
	    	}
	    	else{
				//////////////
				Log.i(LOG_TAG, "Registered odk instance not found, creating one...");
				//////////////
	    		jsonOut2XFormInstance(jsonOutPath, xFormPath, instancePath, instanceName);
	            ContentValues insertValues = new ContentValues();
	            insertValues.put("displayName", instanceName);
	            insertValues.put("instanceFilePath", instanceFilePath);
	            insertValues.put("jrFormId", jrFormId);
	            Uri insertResult = getContentResolver().insert(
	                    COLLECT_INSTANCES_CONTENT_URI, insertValues);
	            instanceId = Integer.valueOf(insertResult.getLastPathSegment());
	    	}
	    	c.close();
			Log.i(LOG_TAG, "instanceId: " + instanceId);
	        //////////////
	        Log.i(LOG_TAG, "Starting Collect...");
	        //////////////
		    intent.setAction(Intent.ACTION_EDIT);
		    intent.putExtra("start", true);
		    intent.setData(Uri.parse(COLLECT_INSTANCES_URI_STRING + "/" + instanceId));
		    startActivity(intent);
		    finish();
		    
		} catch (Exception e) {
			//Display an error dialog if something goes wrong.
			Log.i(LOG_TAG, e.toString());
			AlertDialog.Builder builder = new AlertDialog.Builder(this);
			builder.setMessage(e.toString())
			       .setCancelable(false)
			       .setNeutralButton("Ok", new DialogInterface.OnClickListener() {
			           public void onClick(DialogInterface dialog, int id) {
			                dialog.cancel();
			                finish();
			           }
			       });
			AlertDialog alert = builder.create();
			alert.show();
		}
		
	}
	/**
     * Verify that the form is in collect and put it in collect if it is not.
     * @param filepath
     * @return jrFormId
     */
	private String verifyFormInCollect(String filepath, String jrFormId) {
        String[] projection = { "jrFormId" };
        String selection = "formFilePath = ?";
        String[] selectionArgs = { filepath };
        Cursor c = managedQuery(COLLECT_FORMS_CONTENT_URI, projection,
                selection, selectionArgs, null);
        if (c.getCount() != 0) {
            c.moveToFirst();
            String value = c.getString(c.getColumnIndex("jrFormId"));
            c.close();
            return value;
        }
		//////////////
		Log.i(LOG_TAG, "Registering the new xform with collect.");
		//////////////
        ContentValues insertValues = new ContentValues();
        insertValues.put("displayName", filepath);
        insertValues.put("jrFormId", jrFormId);
        insertValues.put("formFilePath", filepath);
        getContentResolver().insert(COLLECT_FORMS_CONTENT_URI, insertValues);
        return jrFormId;
    }
	/**
	 * Generates an instance of an xform at xFormPath from the JSON output file
	 */
	private void jsonOut2XFormInstance(String jsonOutFile, String xFormPath, String instancePath, String instanceName)
			throws JSONException, IOException, XmlPullParserException {
		//////////////
	    Log.i(LOG_TAG, "Reading the xform...");
	    //////////////
	    Document formDoc = new Document();
	    KXmlParser formParser = new KXmlParser();
        formParser.setInput(new FileReader(xFormPath));
        formDoc.parse(formParser);
	    //////////////
	    Log.i(LOG_TAG, "Getting the relevant elements...");
	    //////////////
	    String namespace = formDoc.getRootElement().getNamespace();
        Element hhtmlEl = formDoc.getElement(namespace, "h:html");
        Element hheadEl = hhtmlEl.getElement(namespace, "h:head");
        Element modelEl = hheadEl.getElement(namespace, "model");
        Element instanceEl = modelEl.getElement(namespace, "instance");
        Element dataEl = instanceEl.getElement(0);
		String jrFormId = dataEl.getAttributeValue(namespace, "id");
        Element instance = new Element();
        instance.setName(dataEl.getName());
        instance.setAttribute("", "id", jrFormId);
        instance.addChild(Node.ELEMENT, instance.createElement("", "xformstarttime"));
        instance.addChild(Node.ELEMENT, instance.createElement("", "xformendtime"));
        //////////////
        Log.i(LOG_TAG, "Parsing the JSON output:");
        //////////////
        JSONObject formRoot = JSONUtils.parseFileToJSONObject(jsonOutFile);
		JSONArray fields = formRoot.getJSONArray("fields");
		int fieldsLength = fields.length();
		if(fieldsLength == 0){
			throw new JSONException("There are no fields in the json output file.");
		}
		//////////////
		Log.i(LOG_TAG, "Transfering the values from the JSON output into the xform instance:");
		//////////////
		for(int i = 0; i < fieldsLength; i++){
			JSONObject field = fields.optJSONObject(i);
			String fieldName = field.getString("name");
			JSONArray segments = field.getJSONArray("segments");
			//Add segment images
			for(int j = 0; j < segments.length(); j++){
				JSONObject segment = segments.getJSONObject(j);
				String imageName = fieldName + "_image_" + j;
				Element fieldImageElement = instance.createElement("", imageName);
				if(!segment.has("image_path") || segment.isNull("image_path") ){
					fieldImageElement.addChild(Node.TEXT, "segmentNotAligned.jpg");
					instance.addChild(Node.ELEMENT, fieldImageElement);
					continue;
				}
				String imagePath = segment.getString("image_path");
				fieldImageElement.addChild(Node.TEXT, new File(imagePath).getName());
				instance.addChild(Node.ELEMENT, fieldImageElement);
				//Copy segment image
				InputStream fis = new FileInputStream(imagePath);
				FileOutputStream fos = new FileOutputStream(instancePath + new File(imagePath).getName());
				// Transfer bytes from in to out
				byte[] buf = new byte[1024];
				int len;
				while ((len = fis.read(buf)) > 0) {
					fos.write(buf, 0, len);
				}
				fos.close();
				fis.close();
			}
			//Create instance element for field value:
			Element fieldElement = instance.createElement("", fieldName);
			fieldElement.addChild(Node.TEXT, "" + field.optString("value"));
			instance.addChild(Node.ELEMENT, fieldElement);
		}
        //////////////
        Log.i(LOG_TAG, "Outputing the instance file:");
        //////////////
	    String instanceFilePath = instancePath + instanceName + ".xml";
	    writeXMLToFile(instance, instanceFilePath);
	}
	/**
	 * Write the given XML tree out to a file
	 * @param elementTree
	 * @param outputPath
	 * @throws IOException
	 */
    private void writeXMLToFile(Element elementTree, String outputPath) throws IOException {
	    File instanceFile = new File(outputPath);
	    instanceFile.createNewFile();
	    FileWriter instanceWriter = new FileWriter(instanceFile);
	    KXmlSerializer instanceSerializer = new KXmlSerializer();
        instanceSerializer.setOutput(instanceWriter);
        elementTree.write(instanceSerializer);
        instanceSerializer.endDocument();
        instanceSerializer.flush();
        instanceWriter.close();
	}
    /**
     * Builds an xfrom from a json template and writes it out to the specified file.
     * Note:
     * It is important to distinguish between an xform and an xform instance.
     * This builds an xform.
     * @param templatePath
     * @param outputPath
     * @param title
     * @param id
     * @throws IOException
     * @throws JSONException 
     */
    public static void buildXForm(String templatePath, String outputPath, String title, String id) throws IOException, JSONException {
		Log.i(LOG_TAG, templatePath);
		JSONObject formRoot = JSONUtils.applyInheritance( JSONUtils.parseFileToJSONObject(templatePath) );

		Log.i(LOG_TAG, "Parsed");
		JSONArray initFields = formRoot.getJSONArray("fields");
		int initFieldsLength = initFields.length();
		
		//Make fields array with no null values
		JSONArray fields = new JSONArray();
		for(int i = 0; i < initFieldsLength; i++){
			JSONObject field = initFields.optJSONObject(i);
			if(field != null){
				fields.put(initFields.getJSONObject(i));
			}
			else{
				Log.i(LOG_TAG, "Null");
			}
		}
		int fieldsLength = fields.length();
		
		//Get the field names and labels:
		String [] fieldNames = new String[fieldsLength];
		String [] fieldLabels = new String[fieldsLength];
		for(int i = 0; i < fieldsLength; i++){
			JSONObject field = fields.getJSONObject(i);
			if(field.has("name")){
				fieldNames[i] = field.getString("name");
				fieldLabels[i] = field.optString("label", field.getString("name"));
			}
			else{
				Log.i(LOG_TAG, "Field " + i + " has no name.");
				//throw new JSONException("Field " + i + " has no name or label.");
			}
		}
    	
        FileWriter writer = new FileWriter(outputPath);
        writer.write("<h:html xmlns=\"http://www.w3.org/2002/xforms\" " +
                "xmlns:h=\"http://www.w3.org/1999/xhtml\" " +
                "xmlns:ev=\"http://www.w3.org/2001/xml-events\" " +
                "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" " +
                "xmlns:jr=\"http://openrosa.org/javarosa\">");
        writer.write("<h:head>");
        writer.write("<h:title>" + title + "</h:title>");
        writer.write("<model>");
        writer.write("<instance>");
        writer.write("<data id=\"" + id + "\">");
        writer.write("<xformstarttime/>");
        writer.write("<xformendtime/>");
        for(int i = 0; i < fieldsLength; i++){
        	JSONObject field = fields.getJSONObject(i);
        	JSONArray segments = field.getJSONArray("segments");
        	for(int j = 0; j < segments.length(); j++){
	            writer.write("<" + fieldNames[i] + "_image_" + j + "/>");
        	}
        	writer.write("<" + fieldNames[i] + "/>");
        }
        writer.write("</data>");
        writer.write("</instance>");
        writer.write("<itext>");
        writer.write("<translation lang=\"eng\">");
        for(int i = 0; i < fieldsLength; i++){
            writer.write("<text id=\"/data/" + fieldNames[i] + ":label\">");
            writer.write("<value>" + fieldLabels[i] + "</value>");
            writer.write("</text>");
        }
        writer.write("</translation>");
        writer.write("</itext>");
        writer.write("<bind nodeset=\"/data/xformstarttime\" type=\"dateTime\" jr:preload=\"timestamp\" jr:preloadParams=\"start\"/>");
        writer.write("<bind nodeset=\"/data/xformendtime\" type=\"dateTime\" jr:preload=\"timestamp\" jr:preloadParams=\"end\"/>");
        for(int i = 0; i < fieldsLength; i++){
        	JSONObject field = fields.getJSONObject(i);
        	JSONArray segments = field.getJSONArray("segments");
        	for(int j = 0; j < segments.length(); j++){
	            writer.write("<bind nodeset=\"/data/" + fieldNames[i] + "_image_" + j + "\" " +
	            		    "appearance=\"web\"" +
				            "readonly=\"true()\" " + 
				            "type=\"binary\"/>");
        	}
        }
        writer.write("</model>");
        writer.write("</h:head>");
        writer.write("<h:body>");
        for(int i = 0; i < fieldsLength; i++){
        	JSONObject field = fields.getJSONObject(i);
        	JSONArray segments = field.getJSONArray("segments");
        	
        	writer.write("<group appearance=\"field-list\">");
        	
        	String type = field.optString("type", "input");
        	writer.write("<" + type +" ref=\"/data/" + fieldNames[i] + "\">");
        	writer.write("<label ref=\"jr:itext('/data/" + fieldNames[i] + ":label')\"/>");
        	//TODO: Make a xform_body_tag field instead?
        	//Advantage is simpler code and more flexibility for extending
        	if( type.equals("select") || type.equals("select1") ){
        		//Get the items from the first segment.
        		JSONObject segment = segments.getJSONObject(0);
                JSONArray items = segment.getJSONArray("items");
                for(int j = 0; j < items.length(); j++){
                	JSONObject item = items.getJSONObject(j);
	                writer.write("<item>");
	                writer.write("<label>" + item.getString("label") + "</label>");
	                writer.write("<value>" + item.getString("value") + "</value>");
	                writer.write("</item>");
                }
                
        	}
            writer.write("</" + type + ">");
            for(int j = 0; j < segments.length(); j++){
	            writer.write("<upload ref=\"/data/" + fieldNames[i] + "_image_" + j  + "\" " +
	    		             "mediatype=\"image/*\" />");
            }
            writer.write("</group>");
        }
        writer.write("</h:body>");
        writer.write("</h:html>");
        writer.close();
    }
}