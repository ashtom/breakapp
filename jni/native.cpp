#include <jni.h>

#include "breakpad.h"
#include "crash.h"

extern "C" {

void Java_com_hockeyapp_breakapp_MainActivity_setUpBreakpad(JNIEnv* env, jobject obj, jstring filepath) {
  const char *path = env->GetStringUTFChars(filepath, 0);
  setUpBreakpad(path);
}

void Java_com_hockeyapp_breakapp_MainActivity_nativeCrash(JNIEnv* env, jobject obj) {
  CrashWithStack();
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	return JNI_VERSION_1_6;
}
}
