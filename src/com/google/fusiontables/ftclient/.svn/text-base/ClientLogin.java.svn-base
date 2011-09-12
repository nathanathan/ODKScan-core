// Copyright 2011 Google Inc. All Rights Reserved.

package com.google.fusiontables.ftclient;

import java.net.URLEncoder;

import com.google.fusiontables.url.RequestHandler;

/**
 * Helper class for getting auth token.
 * 
 * @author kbrisbin@google.com (Kathryn Hurley)
 */
public class ClientLogin {
  private final static String authURI =
      "https://www.google.com/accounts/ClientLogin";

  /**
   * Returns the auth token if the user is successfully authorized.
   *
   * @param username  the username
   * @param password  the password
   * @return the client login token, or null if not authorized
   */
  public static String authorize(String username, String password) {
    // Encode the body
    String body = "Email=" + URLEncoder.encode(username) + "&" +
      "Passwd=" + URLEncoder.encode(password) + "&" +
      "service=fusiontables&" +
      "accountType=HOSTED_OR_GOOGLE";

    // Send the response and parse results to get token
    String response = RequestHandler.sendHttpRequest(authURI, "POST",
        body, null);

    // If the response is length 4, the authorization was successful
    String[] splitResponse = response.trim().split("=");
    if (splitResponse.length == 4) {
      return splitResponse[3];
    }
    return null;
  }
}
