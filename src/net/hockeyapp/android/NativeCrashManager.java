package net.hockeyapp.android;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.FilenameFilter;
import java.io.OutputStream;
import java.net.URL;
import java.security.KeyStore;
import java.util.Date;
import java.util.UUID;

import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManagerFactory;

import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.mime.MultipartEntity;
import org.apache.http.entity.mime.content.FileBody;
import org.apache.http.impl.client.DefaultHttpClient;

import android.app.Activity;
import android.util.Log;

public class NativeCrashManager {
  public static void handleDumpFiles(Activity activity, String identifier) {
    String[] filenames = searchForDumpFiles();
    for (String dumpFilename : filenames) {
      String logFilename = createLogFile();
      if (logFilename != null) {
        uploadDumpAndLog(activity, identifier, dumpFilename, logFilename);
      }
    }
  }
  
  public static String createLogFile() {
    final Date now = new Date();

    try {
      // Create filename from a random uuid
      String filename = UUID.randomUUID().toString();
      String path = Constants.FILES_PATH + "/" + filename + ".faketrace";
      Log.d(Constants.TAG, "Writing unhandled exception to: " + path);
      
      // Write the stacktrace to disk
      BufferedWriter write = new BufferedWriter(new FileWriter(path));
      write.write("Package: " + Constants.APP_PACKAGE + "\n");
      write.write("Version Code: " + Constants.APP_VERSION + "\n");
      write.write("Version Name: " + Constants.APP_VERSION_NAME + "\n");
      write.write("Android: " + Constants.ANDROID_VERSION + "\n");
      write.write("Manufacturer: " + Constants.PHONE_MANUFACTURER + "\n");
      write.write("Model: " + Constants.PHONE_MODEL + "\n");
      write.write("Date: " + now + "\n");
      write.write("\n");
      write.write("MinidumpContainer");
      write.flush();
      write.close();
      
      return filename + ".faketrace";
    } 
    catch (Exception another) {
    }
    
    return null;
  }
  
  public static void uploadDumpAndLog(final Activity activity, final String identifier, final String dumpFilename, final String logFilename) {
    new Thread() {
      @Override
      public void run() {
        uploadWithHttpClient(identifier, dumpFilename, logFilename); /// existing implementation, moved to a function
//        uploadWithHttpsURLConnection(identifier, dumpFilename, logFilename); /// new proposed implementation, has a few issues still
        
        activity.deleteFile(logFilename);
        activity.deleteFile(dumpFilename);
      }
    }.start();
  }
  
  private static void uploadWithHttpClient(String identifier, String dumpFilename, String logFilename)
  {
    try {
      DefaultHttpClient httpClient = new DefaultHttpClient(); 
      HttpPost httpPost = new HttpPost("https://rink.hockeyapp.net/api/2/apps/" + identifier + "/crashes/upload");
      
      MultipartEntity entity = new MultipartEntity();
      
      File dumpFile = new File(Constants.FILES_PATH, dumpFilename);
      entity.addPart("attachment0", new FileBody(dumpFile));
      
      File logFile = new File(Constants.FILES_PATH, logFilename);
      entity.addPart("log", new FileBody(logFile));
      
      httpPost.setEntity(entity);
      
      httpClient.execute(httpPost);
    }
    catch (Exception e) {
      e.printStackTrace();
    }
  }
  
  private static void uploadWithHttpsURLConnection(String identifier, String dumpFilename, String logFilename)
  {
    /// It doesn't seem to matter if we use HttpsURLConnection or HttpURLConnection.
    /// The HTTP status code comes back the same, they seem to be based off the Content-Type settings.
    HttpsURLConnection urlConnection = null;
    try {
      URL crashURL = new URL("https://rink.hockeyapp.net/api/2/apps/" + identifier + "/crashes/upload");
      
      /// Use for HttpsURLConnection
      KeyStore keystore = null;
      String algorithm = TrustManagerFactory.getDefaultAlgorithm();
      TrustManagerFactory tmf = TrustManagerFactory.getInstance(algorithm);
      tmf.init(keystore); /// passing a null KeyStore uses the default system KeyStore.
      SSLContext context = SSLContext.getInstance("TLS");
      context.init(null, tmf.getTrustManagers(), null);
      
      urlConnection = (HttpsURLConnection) crashURL.openConnection();
      urlConnection.setSSLSocketFactory(context.getSocketFactory()); /// do not set if using HttpURLConnection
      
      urlConnection.setRequestMethod("POST");
      urlConnection.setDoOutput(true); /// needed for writing to the output stream
      urlConnection.setDoInput(true); /// not sure if necessary or not
      urlConnection.setReadTimeout(10000);
      urlConnection.setConnectTimeout(15000);
      
      MultipartEntity entity = new MultipartEntity();
      
      File dumpFile = new File(Constants.FILES_PATH, dumpFilename);
      entity.addPart("attachment0", new FileBody(dumpFile));

      File logFile = new File(Constants.FILES_PATH, logFilename);
      entity.addPart("log", new FileBody(logFile));
      
      urlConnection.setRequestProperty("Connection", "Keep-Alive");
      urlConnection.addRequestProperty("Content-Length", Long.toString(entity.getContentLength()));
      
      /// Content-Type settings tried: 1, 2, and 3
      /// 1 - Deprecated methods: getName, getValue.
      /// Gets 201 statusCode: Created: The request has been fulfilled and resulted in a new resource being created.
      /// No crash is seen on HockeyApp though.
//      String name = entity.getContentType().getName();
//      String value = entity.getContentType().getValue();
//      urlConnection.addRequestProperty(name, value);
      
      /// 2 - Custom multipart boundary. Gets 401 statusCode.
      /// HockeyApp guys: What should be sent up for a WWW-Authenticate header to overcome this error?
      urlConnection.addRequestProperty("Content-Type", "multipart/form-data; boundary=--HockeyAppBoundary--");
      
      /// 3 - No Content-Type. Gets 415 statusCode.
      
      OutputStream os = urlConnection.getOutputStream();
      entity.writeTo(os);
      os.close();
      
      urlConnection.connect();
      
      int statusCode = urlConnection.getResponseCode();
      if (statusCode == HttpsURLConnection.HTTP_OK) {
        Log.d(Constants.TAG, "crash uploaded successfully");
      }
      else {
        Log.d(Constants.TAG, "ERROR: " + statusCode);
      }
    }
    catch (Exception e) {
      e.printStackTrace();
    }
    finally {
      if (urlConnection != null) {
        urlConnection.disconnect();
      }
    }
  }
  
  private static String[] searchForDumpFiles() {
    if (Constants.FILES_PATH != null) {
      // Try to create the files folder if it doesn't exist
      File dir = new File(Constants.FILES_PATH + "/");
      boolean created = dir.mkdir();
      if (!created && !dir.exists()) {
        return new String[0];
      }
  
      // Filter for ".dmp" files
      FilenameFilter filter = new FilenameFilter() { 
        public boolean accept(File dir, String name) {
          return name.endsWith(".dmp"); 
        } 
      }; 
      return dir.list(filter);
    }
    else {
      Log.d(Constants.TAG, "Can't search for exception as file path is null.");
      return new String[0];
    }
  }
}
