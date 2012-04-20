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
		//setContentView(R.layout.main);

		try {
			//Read in parameters from the intent's extras.
			Bundle extras = getIntent().getExtras();

			if(extras == null){ throw new Exception("No parameters specified"); }
			
			photoName = extras.getString("photoName");
			String jsonOutPath = MScanUtils.getJsonPath(photoName);
			String templatePath = extras.getString("templatePath");

			if(jsonOutPath == null){ throw new Exception("jsonOutPath is null"); }
			if(templatePath == null){
				//////////////
				Log.i(LOG_TAG, "templatePath is null. Getting template from jsonOut:");
				//////////////
				JSONObject formRoot = JSONUtils.parseFileToJSONObject(jsonOutPath);
				if ( formRoot.has("name") ){
					templatePath = formRoot.getString("name");
				}
				else{
					throw new Exception("Could not identify template.");
				}
			}
			
			Log.i(LOG_TAG,"jsonOutPath : " + jsonOutPath);
			Log.i(LOG_TAG,"templatePath : " + templatePath);
			
			String templateName = new File(templatePath).getName();
			String xFormPath = templatePath + ".xml";
			String jsonPath = templatePath + ".json";
			
			//If there is no xform or the xform is out of date create it.
			File xformFile = new File(xFormPath);
			if( !xformFile.exists() || true || //REMOVE THIS
				new File(jsonPath).lastModified() > xformFile.lastModified()){
				//////////////
				Log.i(LOG_TAG, "Unregistering old versions in collect.");
				//////////////
			    String [] deleteArgs = { templateName };
		        int deleteResult = getContentResolver().delete(COLLECT_FORMS_CONTENT_URI, "jrFormId like ?", deleteArgs);
		        Log.w(LOG_TAG, "removing " + deleteResult + " rows.");
				//////////////
				Log.i(LOG_TAG, "Creating the xform");
				//////////////
			    buildXForm(jsonPath, xFormPath, templateName, templateName);
			    /*
				//////////////
				Log.i(LOG_TAG, "Registering the new xform with collect.");
				//////////////
		        ContentValues insertValues = new ContentValues();
		        insertValues.put("formFilePath", xformPath);
		        insertValues.put("displayName", templateName);
		        insertValues.put("jrFormId", templateName);
		        Uri insertResult = getContentResolver().insert(COLLECT_FORMS_CONTENT_URI, insertValues);
		      
		        // launch Collect
		        int formId = Integer.valueOf(insertResult.getLastPathSegment());
		        Intent intent = new Intent();
		        intent.setComponent(new ComponentName("org.odk.collect.android",
		                "org.odk.collect.android.activities.FormEntryActivity"));
		        intent.setAction(Intent.ACTION_EDIT);
		        intent.setData(Uri.parse(COLLECT_FORMS_URI_STRING + "/" + formId));
		        startActivityForResult(intent, ODK_COLLECT_ADDROW_RETURN);
 				*/
			}

			int instanceId = jsonOut2XFormInstance(jsonOutPath, xFormPath);
	        //////////////
	        Log.i(LOG_TAG, "Start Collect:");
	        //////////////
		    Intent intent = new Intent();
		    intent.setFlags(Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
		    intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		    intent.setComponent(new ComponentName("org.odk.collect.android",
		            "org.odk.collect.android.activities.FormEntryActivity"));
		    intent.setAction(Intent.ACTION_EDIT);
		    intent.setData(Uri.parse(COLLECT_INSTANCES_URI_STRING + "/" +
		            instanceId));
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
    private static String xmlTagSanitize(String string) {
    	return string.replaceAll(" ", "_")
		             .replaceAll("[-.:']", "")
		             .replace("0", "zero")
		             .replace("1", "one")
		             .replace("2", "two")
		             .replace("3", "three")
		             .replace("4", "four")
		             .replace("5", "five")
		             .replace("6", "six")
		             .replace("7", "seven")
		             .replace("8", "eight")
		             .replace("9", "nine")
		             .toLowerCase();
	}
	/**
     * Verify that the form is in collect and put it in collect if it is not.
     * @param filepath
     * @return
     */
	private String verifyFormInCollect(String filepath, String jrFormId) {
        Log.d("CSTF", "filepath<" + filepath + ">");

        String[] projection = { "jrFormId" };
        String selection = "formFilePath = ?";
        String[] selectionArgs = { filepath };
        Cursor c = managedQuery(COLLECT_FORMS_CONTENT_URI, projection,
                selection, selectionArgs, null);
        /*
        String [] columnNames = c.getColumnNames();
        for(int i = 0; i < columnNames.length; i++){
        	 Log.d("CSTF", columnNames[i]);
        }
        */
        if (c.getCount() != 0) {
            c.moveToFirst();
            String value = c.getString(c.getColumnIndexOrThrow("jrFormId"));
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
	 * Generates an instance of an xform at templatePath from the JSON output file
	 * and registers it with ODK Collect.
	 * @param templatePath
	 * @param jsonOutFile
	 * @return the instanceId
	 * @throws JSONException
	 * @throws IOException
	 * @throws XmlPullParserException 
	 */
	private int jsonOut2XFormInstance(String jsonOutFile, String tempaltePath) throws JSONException, IOException, XmlPullParserException {
		//////////////
		Log.i(LOG_TAG, "Initializing the xform instance:");
		//////////////
        File formFile = new File(tempaltePath);
        String formFileName = formFile.getName();
	    String instanceName = ((formFileName.endsWith(".xml") ?
	            formFileName.substring(0, formFileName.length() - 4) :
                formFileName)) +
                COLLECT_INSTANCE_NAME_DATE_FORMAT.format(new Date());
	    String instancePath = "/sdcard/odk/instances/" + instanceName + "/";
	    (new File(instancePath)).mkdirs();
		//////////////
	    Log.i(LOG_TAG, "Reading the xform...");
	    //////////////
	    Document formDoc = new Document();
	    KXmlParser formParser = new KXmlParser();
        formParser.setInput(new FileReader(tempaltePath));
        formDoc.parse(formParser);
	    //////////////
	    Log.i(LOG_TAG, "Getting the relevant elements...");
	    //////////////
	    String namespace = formDoc.getRootElement().getNamespace();
        Element hhtmlEl = formDoc.getElement(namespace, "h:html");
        Element hheadEl = hhtmlEl.getElement(namespace, "h:head");
        Element modelEl = hheadEl.getElement(namespace, "model");
        Element instanceEl = modelEl.getElement(namespace, "instance");
        Element dataEl = instanceEl.getElement(0);//TODO: Why was this 1 and not 0? Is there a bug in tables?
		String jrFormId = dataEl.getAttributeValue(namespace, "id");
		verifyFormInCollect(tempaltePath, jrFormId);
        
        Element instance = new Element();
        instance.setName(dataEl.getName());
        Log.d(LOG_TAG, "jrFormId in collect():" + jrFormId);
        instance.setAttribute("", "id", jrFormId);
        //////////////
        Log.i(LOG_TAG, "Parsing the JSON output:");
        //////////////
        JSONObject formRoot = JSONUtils.parseFileToJSONObject(jsonOutFile);
		JSONArray fields = formRoot.getJSONArray("fields");
		int fieldsLength = fields.length();
		if(fieldsLength == 0) {
			throw new JSONException("There are no files in the json output file.");
		}
		//////////////
		Log.i(LOG_TAG, "Adding missing name properties to the fields:");
		//////////////
		for (int i = 0; i < fieldsLength; i++) {
			
			JSONObject field = fields.optJSONObject(i);
			
			if (field.has("name")) {
				continue;
			}
			else if (field.has("label")) {
				field.put("name", xmlTagSanitize(field.getString("label")));
				//fields.put(i, field);
			}
			else{
				throw new JSONException("Field " + i + " has no name or label.");
			}
		}
		//////////////
		Log.i(LOG_TAG, "Transfering the values from the JSON output into the xform instance:");
		//////////////
		for (int i = 0; i < fieldsLength; i++) {
			JSONObject field = fields.optJSONObject(i);
			String fieldName = field.getString("name");
			
			JSONArray segments = field.getJSONArray("segments");
			//Add segment images
			for (int j = 0; j < segments.length(); j++) {
				Element fieldImageElement = instance.createElement("", fieldName + "_image_" + j);
				String imageName;
				try{
					imageName = field.getString("name") + "_" + j + ".jpg";
				}
				catch(JSONException e){
					imageName = field.getString("label") + "_" + j + ".jpg";
				}
				fieldImageElement.addChild(0, Node.TEXT, imageName);
				instance.addChild(i, Node.ELEMENT, fieldImageElement);
				
				//Copy segment image
				InputStream fis = new FileInputStream(MScanUtils.getOutputPath(photoName) + "segments/" + imageName);
				FileOutputStream fos = new FileOutputStream(instancePath + imageName);
				// Transfer bytes from in to out
				byte[] buf = new byte[1024];
				int len;
				while ((len = fis.read(buf)) > 0) {
					fos.write(buf, 0, len);
				}
				fos.close();
				fis.close();
			}
			
			Element fieldElement = instance.createElement("", fieldName);
        	String out_type = field.optString("out_type");
        	if( out_type.equals("number") ){
				Number[] segmentCounts = JSONUtils.getSegmentValues(field);
				fieldElement.addChild(0, Node.TEXT, "" + MScanUtils.sum(segmentCounts).intValue());
			}
			else if( out_type.equals("select") ){
				String selected = "";
				for(int j = 0; j < segments.length(); j++) {
					JSONArray bubbles = segments.getJSONObject(j).getJSONArray("bubbles");
    				for(int k = 0; k < bubbles.length(); k++) {
    					if(bubbles.getJSONObject(k).getInt("value") != 0){
    						selected += k + " ";
    					}
    				}
				}
				fieldElement.addChild(0, Node.TEXT, "" + selected);
			}
			else if( out_type.equals("select1") ){
				//TODO: Clean up this redundant code.
				String selected = "";
				for(int j = 0; j < segments.length(); j++) {
					JSONArray bubbles = segments.getJSONObject(j).getJSONArray("bubbles");
    				for(int k = 0; k < bubbles.length(); k++) {
    					if(bubbles.getJSONObject(k).getInt("value") != 0){
    						selected = "" + k;
    					}
    				}
				}
				fieldElement.addChild(0, Node.TEXT, "" + selected);
			}
			else{
				throw new JSONException("Unknown out_type: " + out_type);
			}
			instance.addChild(i, Node.ELEMENT, fieldElement);
		}
        
        //////////////
        Log.i(LOG_TAG, "Outputing the instance file:");
        //////////////
	    String instanceFilePath = instancePath + instanceName + ".xml";
	    writeXMLToFile(instance, instanceFilePath);
	    //////////////
        Log.i(LOG_TAG, "Registering the instance with Collect:");
        //////////////
        return registerInstance(instanceName, instanceFilePath, jrFormId);
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
	 * Register the given xform with Collect
	 * @param name
	 * @param filepath
	 * @param jrFormId
	 * @return instanceId
	 */
    private int registerInstance(String name, String filepath, String jrFormId) {
        ContentValues insertValues = new ContentValues();
        insertValues.put("displayName", name);
        insertValues.put("instanceFilePath", filepath);
        insertValues.put("jrFormId", jrFormId);
        Uri insertResult = getContentResolver().insert(
                COLLECT_INSTANCES_CONTENT_URI, insertValues);
        return Integer.valueOf(insertResult.getLastPathSegment());
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
			else if(field.has("label")){
				fieldNames[i] = xmlTagSanitize(field.getString("label"));
				fieldLabels[i] = field.getString("label");
			}
			else{
				Log.i(LOG_TAG, "Field " + i + " has no name or label.");
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
        for(int i = 0; i < fieldsLength; i++){
        	JSONObject field = fields.getJSONObject(i);
        	JSONArray segments = field.getJSONArray("segments");
        	for(int j = 0; j < segments.length(); j++){
	            writer.write("<bind nodeset=\"/data/" + fieldNames[i] + "_image_" + j + "\" " +
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
        	
        	String out_type = field.optString("out_type", "");
        	//TODO: Make a xform_body_tag field instead?
        	//Advantage is simpler code and more flexibility for extending
        	if( out_type.equals("select") || out_type.equals("select1") ){
        		
        		writer.write("<" + out_type +" ref=\"/data/" + fieldNames[i] + "\">");
                writer.write("<label ref=\"jr:itext('/data/" + fieldNames[i] + ":label')\"/>");
                
                JSONArray items = field.getJSONArray("bubble_labels");
                for(int j = 0; j < items.length(); j++){
	                writer.write("<item>");
	                writer.write("<label>" + items.getString(j) + "</label>");
	                writer.write("<value>" + j + "</value>");
	                writer.write("</item>");
                }
                
                writer.write("</" + out_type + ">");
        	}
        	else{
                writer.write("<input ref=\"/data/" + fieldNames[i] + "\">");
                writer.write("<label ref=\"jr:itext('/data/" + fieldNames[i] + ":label')\"/>");
                writer.write("</input>");
        	}
        	
            for(int j = 0; j < segments.length(); j++){
	            writer.write("<upload ref=\"/data/" + fieldNames[i] + "_image_" + j  + "\" "
	    				+ "mediatype=\"image/*\" />");
            }
            writer.write("</group>");
        }
        writer.write("</h:body>");
        writer.write("</h:html>");
        writer.close();

    }
}