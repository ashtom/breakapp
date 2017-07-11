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

void swap(int*& left, int*& right) {
    int*tmp = left;
    left    = right;
    right   = tmp;
}

void bubble(int** first, int** last) {
    if (first < last) {
        bubble(&first[1], last);
        while (first < last) {
            if (*first[0] > *first[1]) {
                swap(first[0], first[1]);
            }
            ++first;
        }
    }
}

void Crash() {
    const int count = 10;
    const int last = count - 1;
    int values[count] = { 12, 34, 5, 7, 218, 923, -1, 0, -1, 5};
    int* refs[count];
    memset(&refs, 0, sizeof(refs));
    // let's introduce an error here and skip the base index (0)
    for (int i = 1; i < count; ++i) {
        refs[i] = &values[i];
    }
    // now let's do a bubble sort
    bubble(&refs[0], &refs[last]);
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
