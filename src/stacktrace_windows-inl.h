// Copyright (c) 2000 - 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Andrew Schwartzmeyer
//
// Windows implementation - just use CaptureStackBackTrace

#include "port.h"
#include "stacktrace.h"
#include "Dbghelp.h"
#include <vector>

_START_GOOGLE_NAMESPACE_

static bool ready_to_run = false;
class StackTraceInit {
public:
  HANDLE hProcess;
  StackTraceInit() {
    // Initialize the symbol handler
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms680344(v=vs.85).aspx
    hProcess = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    SymInitialize(hProcess, NULL, true);
    ready_to_run = true;
  }
  ~StackTraceInit() {
    SymCleanup(hProcess);
    ready_to_run = false;
  }
};

static const StackTraceInit module_initializer;  // Force initialization

// If you change this function, also change GetStackFrames below.
int GetStackTrace(void** result, int max_depth, int skip_count) {
  if (!ready_to_run) {
    return 0;
  }
  skip_count++;  // we want to skip the current frame as well
  if (max_depth > 64) {
    max_depth = 64;
  }
  std::vector<void*> stack(max_depth);
  // This API is thread-safe (moreover it walks only the current thread).
  int size = CaptureStackBackTrace(skip_count, max_depth, &stack[0], NULL);
  for (int i = 0; i < size; ++i) {
    // Resolve symbol information from address.
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;
    SymFromAddr(module_initializer.hProcess, reinterpret_cast<DWORD64>(stack[i]), 0, symbol);
    result[i] = stack[i];
  }

  return size;
}

_END_GOOGLE_NAMESPACE_
