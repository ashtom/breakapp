#include <jni.h>
#include <stdio.h>
#include "client/linux/handler/exception_handler.h"
#include "client/linux/handler/minidump_descriptor.h"

static google_breakpad::ExceptionHandler* exceptionHandler;
bool DumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                  void* context,
                  bool succeeded) {
  printf("Dump path: %s\n", descriptor.path());
  return succeeded;
}

void Crash() {
  volatile int* a = reinterpret_cast<volatile int*>(NULL);
  *a = 1;
}

extern "C" {

void Java_com_hockeyapp_breakapp_MainActivity_setUpBreakpad(JNIEnv* env, jobject obj, jstring filepath) {
  const char *path = env->GetStringUTFChars(filepath, 0);
  google_breakpad::MinidumpDescriptor descriptor(path);
  exceptionHandler = new google_breakpad::ExceptionHandler(descriptor, NULL, DumpCallback, NULL, true, -1);
}

void Java_com_hockeyapp_breakapp_MainActivity_nativeCrash(JNIEnv* env, jobject obj) {
  Crash();
}

}
