// Copyright 2011 Google Inc. All Rights Reserved.

package com.google.fusiontables.url;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpUriRequest;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.util.EntityUtils;

import android.util.Log;

/**
 * Creates and sends http POST and GET requests. Returns string-formatted
 * response.
 * 
 * @author kbrisbin@google.com (Kathryn Hurley)
 */
public class RequestHandler {
  private static final String TAG = "REQUEST HANDLER";

  /**
   * Send an HTTP POST or GET request.
   *
   * @param uri the URL
   * @param method either POST or GET
   * @param body the body of the HTTP request, can be null
   * @param headers a map of any headers to add to the request, can be null
   * @return the string response, or null if there was an error
   */
  public static String sendHttpRequest(String uri, String method,
      String body, Map<String, String> headers)  {

    try {
      HttpClient httpclient = new DefaultHttpClient();
      HttpUriRequest request;
      
      // Initialize the request
      if (method  == "POST") {
        request = new HttpPost(uri);
        request.setHeader("Content-Type", "application/x-www-form-urlencoded");

        HttpEntity requestBody = new StringEntity(body);
        ((HttpPost) request).setEntity(requestBody);
        
      } else {
        request = new HttpGet(uri);
      }

      // Set the headers
      if (headers != null) {
        setHeaders(headers, request);
      }

      // Execute the request
      HttpResponse response = httpclient.execute(request);

      // Read the response
      HttpEntity entity = response.getEntity();
      String responseContent = EntityUtils.toString(entity);

      return responseContent;

    } catch (UnsupportedEncodingException e) {
      Log.e(TAG, "Message could not be encoded: " + body, e);
    } catch (ClientProtocolException e) {
      Log.e(TAG, "Incorrect protocol: " + uri, e);
    } catch (IOException e) {
      Log.e(TAG, "Error accessing resource: " + uri, e);
    }

    return null;
  }
  
  /**
   * Adds the headers to the request.
   *
   * @param headers a map of any headers to add to the request, can be null
   * @param request the http request
   */
  private static void setHeaders(Map<String, String> headers,
      HttpUriRequest request) {
    Set<String> keys = headers.keySet();
    Iterator<String> it = keys.iterator();
    String header = "";
    while (it.hasNext()) {
      header = it.next();
      request.setHeader(header, headers.get(header));
    }
  }
}
