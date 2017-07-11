#include "breakpad.h"
#include "client/linux/handler/exception_handler.h"
#include "client/linux/handler/minidump_descriptor.h"
#include <stdio.h>

static google_breakpad::ExceptionHandler* exceptionHandler;
bool DumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                  void* context,
                  bool succeeded) {
  printf("Dump path: %s\n", descriptor.path());
  return succeeded;
}

void setUpBreakpad(const char* path) {
  google_breakpad::MinidumpDescriptor descriptor(path);
  exceptionHandler = new google_breakpad::ExceptionHandler(descriptor, NULL, DumpCallback, NULL, true, -1);
}
