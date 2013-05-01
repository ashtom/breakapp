This demo app shows how to use Breakpad with HockeyApp on Android. The app consists of both Java and C/C++ code (using the NDK). Crashes in C/C++ are handled by Breakpad and then uploaded to HockeyApp for further processing.

## Setup

1. Clone this repository:

        cd ~/Projects
        git clone git://github.com/ashtom/breakapp.git
        
2. Checkout Google Breakpad in the same directory:

        svn checkout http://google-breakpad.googlecode.com/svn/trunk/ Breakpad

3. Build Breakpad:

        cd ~/Projects/Breakpad
        ./configure && make
        
4. Build the native code:

        cd ~/Projects/BreakApp
        ndk-build

5. Open the Java project in Eclipse and run it on your Android device.

## Details

* If you want to see the crashes in your HockeyApp account, please adjust the value of HOCKEYAPP_ID in MainActivity.java.
* As usual, crashes are sent at the next start of the app.
* The minidump file is processed on HockeyApp and inserted into the crash log for grouping and further inspection. This might take a minute or two.
* Stack traces are not symbolicated. You can download the minidump file from the "Meta" tab to symbolicate on your machine.
