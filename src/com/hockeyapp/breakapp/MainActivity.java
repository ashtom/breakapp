package com.hockeyapp.breakapp;

import net.hockeyapp.android.Constants;
import net.hockeyapp.android.CrashManager;
import net.hockeyapp.android.FeedbackManager;
import net.hockeyapp.android.NativeCrashManager;
import net.hockeyapp.android.UpdateManager;
import android.app.Activity;
import android.os.Bundle;
import android.view.View;

public class MainActivity extends Activity {
  private static final String HOCKEYAPP_ID = "98254247ac79b7cd96dbec27c53b7c9f";

  private native void setUpBreakpad(String filepath, boolean moreDump);  
  private native void nativeCrash();  

  static {  
    System.loadLibrary("native");  
  }  
  
  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    
    setContentView(R.layout.activity_main);
    Constants.loadFromContext(this);
    setUpBreakpad(Constants.FILES_PATH, true);
    NativeCrashManager.handleDumpFiles(this, HOCKEYAPP_ID);
  }
  
  public void onResume() {
    super.onResume();
    CrashManager.register(this, HOCKEYAPP_ID);
  }
  
  public void onCheckUpdatesClicked(View v) {
    UpdateManager.register(this, HOCKEYAPP_ID);
  }
  
  public void onSendFeedbackClicked(View v) {
    FeedbackManager.register(this, HOCKEYAPP_ID, null);
    FeedbackManager.showFeedbackActivity(this);
  }
  
  public void onNativeCrashClicked(View v) {
    nativeCrash();
  }
  
  public void onCrashClicked(View v) {
    View view = findViewById(0x123);
    view.setVisibility(View.INVISIBLE);
  }
}
