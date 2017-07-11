#include "breakpad.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <algorithm>

#define private public
#include "client/linux/handler/exception_handler.h"
#include "client/linux/handler/minidump_descriptor.h"
#include "client/linux/dump_writer_common/ucontext_reader.h"

// static exception handler
static google_breakpad::ExceptionHandler* exceptionHandler;

// these values are compilation time constants
const size_t pt_size = sizeof(void*);
const size_t STACK_PAGES = 2;
const size_t BYTES_PRIOR = 256;

// these statics are queried at run time but the queries are idempotent (the same results expected each time)
static size_t page_size;
static size_t page_half;
static size_t mem_count;

// this only needs to be used once
static uintptr_t* sortedBuf = NULL;

// this one is a dummy marker. whatever equals it may be discarded.
static google_breakpad::AppMemory dummyChunk;

inline uintptr_t ToPage(uintptr_t address) {
  return address & (-page_size);
}

inline bool IsPageMapped(void* page_address) {
  unsigned char vector;
  return 0 /*OK*/ == mincore(page_address, page_size, &vector);
}

inline bool IsPageMapped(uintptr_t page_address) {
  return IsPageMapped(reinterpret_cast<void*>(page_address));
}

inline bool IsAddressMapped(uintptr_t address) {
  return IsPageMapped(ToPage(address));
}

void EnsureBuf() {
  if (sortedBuf == NULL) {
    size_t sizeOfBuf = page_size * STACK_PAGES;
    // could be malloc(sizeOfBuf) but I preferred a dedicated mapping
    sortedBuf = reinterpret_cast<uintptr_t*>(mmap(NULL, sizeOfBuf,
      PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    // assert sortedBuf != null
  }
}

class HeapContext {
private:
  google_breakpad::AppMemoryList* mList;

public:
  void Attach(google_breakpad::ExceptionHandler * eh) {
    google_breakpad::AppMemoryList& memoryList = eh->app_memory_list_;
    // preallocate mem_count dummy chunks
    for (int i = 0; i < mem_count; ++i) {
      memoryList.push_back(dummyChunk);
    }
    mList = &memoryList;
  }
  
  void TryIncludePage(google_breakpad::AppMemoryList::reverse_iterator& chunk, bool allowMerge, uintptr_t& fault, uintptr_t page_address) {
    if (page_address != fault) {
      // try merge
      if (allowMerge) {
        uintptr_t last_address = reinterpret_cast<uintptr_t>(chunk->ptr) + chunk->length;
        if (page_address >= last_address) {
          if (IsPageMapped(page_address)) {
            if (page_address > last_address) {
              --chunk;
              chunk->ptr = reinterpret_cast<void*>(page_address);
              chunk->length = page_size;
            } else {
              chunk->length += page_size;
            }
          } else {
            fault = page_address;
          }
        }
      } else {
        // map, then create chunk
        if (IsPageMapped(page_address)) {
          --chunk;
          chunk->ptr = reinterpret_cast<void*>(page_address);
          chunk->length = page_size;
        } else {
          fault = page_address;
        }
      }
    }
  }
  
  void CollectData(uintptr_t stackTop, uintptr_t stackSize) {
    memcpy(sortedBuf, reinterpret_cast<void*>(stackTop), stackSize);
    uintptr_t  ptr_count = stackSize / pt_size;
    uintptr_t* sortedPtr = sortedBuf;
    uintptr_t* sortedEnd = sortedBuf + ptr_count;
    std::sort(sortedPtr, sortedEnd); // qsort
    
    google_breakpad::AppMemoryList::reverse_iterator start = mList->rend();
    google_breakpad::AppMemoryList::reverse_iterator chunk = start;
    uintptr_t fault  = 0xffffffff;
    while (sortedPtr != sortedEnd) {
      uintptr_t stackAddr = *(sortedPtr++);
      if (stackAddr >= BYTES_PRIOR) {
        stackAddr -= BYTES_PRIOR;
      }
      uintptr_t stackPage = ToPage(stackAddr);
      TryIncludePage(chunk, chunk != start, fault, stackPage);
      if (stackAddr > stackPage + page_half) {
        TryIncludePage(chunk, chunk != start, fault, stackPage + page_size);
      }
    }
  }
};

bool HeapCallback(const void* crash_context, size_t crash_context_size, void* context) {
  // << We have a different source of information for the crashing thread.>> -- Google
  // * google-breakpad/src/client/linux/minidump_writer/minidump_writer.cc:322
  
  google_breakpad::ExceptionHandler::CrashContext* cc = (google_breakpad::ExceptionHandler::CrashContext*) crash_context;
  uintptr_t stackTop = ToPage(google_breakpad::UContextReader::GetStackPointer(&cc->context));
  
  // http://man7.org/linux/man-pages/man2/mincore.2.html
  if (IsPageMapped(stackTop)) {
    uintptr_t stackSize = page_size;
    
    // extra page. keep in sync with STACK_PAGES
    if (IsPageMapped(stackTop + page_size)) {
      stackSize += page_size;
    }

    HeapContext* userContext = reinterpret_cast<HeapContext*>(context);
    userContext->CollectData(stackTop, stackSize);
  }
  
  return false; // we haven't handled - only prepared the handling
}

bool DumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                  void* context,
                  bool succeeded) {
  printf("Dump path: %s\n", descriptor.path());
  return succeeded;
}

void setUpBreakpad(const char* path, bool moreDump) {
  google_breakpad::MinidumpDescriptor descriptor(path);
  // http://man7.org/linux/man-pages/man3/sysconf.3.html
  page_size = sysconf(_SC_PAGESIZE);
  page_half = page_size >> 1;
  mem_count = page_size * STACK_PAGES / pt_size;
   
  dummyChunk.ptr = &dummyChunk;
  dummyChunk.length = 0;

  // signal handler registration is an EH constructor side effect. upon exit, we are registered
  if (moreDump) {
    HeapContext* userContext = new HeapContext;
    exceptionHandler = new google_breakpad::ExceptionHandler(descriptor, NULL, DumpCallback, userContext, true, -1);
    exceptionHandler->set_crash_handler(HeapCallback);
    userContext->Attach(exceptionHandler);
    EnsureBuf();
  } else {
    exceptionHandler = new google_breakpad::ExceptionHandler(descriptor, NULL, DumpCallback, NULL, true, -1);
  }
}
