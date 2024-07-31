// Copyright (c) 2024, Google Inc.
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
// Author: Satoru Takabayashi
//
// This library provides Symbolize() function that symbolizes program
// counters to their corresponding symbol names on linux platforms.
// This library has a minimal implementation of an ELF symbol table
// reader (i.e. it doesn't depend on libelf, etc.).
//
// The algorithm used in Symbolize() is as follows.
//
//   1. Go through a list of maps in /proc/self/maps and find the map
//   containing the program counter.
//
//   2. Open the mapped file and find a regular symbol table inside.
//   Iterate over symbols in the symbol table and look for the symbol
//   containing the program counter.  If such a symbol is found,
//   obtain the symbol name, and demangle the symbol if possible.
//   If the symbol isn't found in the regular symbol table (binary is
//   stripped), try the same thing with a dynamic symbol table.
//
// Note that Symbolize() is originally implemented to be used in
// FailureSignalHandler() in base/google.cc.  Hence it doesn't use
// malloc() and other unsafe operations.  It should be both
// thread-safe and async-signal-safe.

#ifndef GLOG_INTERNAL_SYMBOLIZE_H
#define GLOG_INTERNAL_SYMBOLIZE_H

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "config.h"
#include "glog/platform.h"

#if defined(HAVE_LINK_H)
#  include <link.h>  // For ElfW() macro.
#elif defined(HAVE_ELF_H)
#  include <elf.h>
#elif defined(HAVE_SYS_EXEC_ELF_H)
#  include <sys/exec_elf.h>
#endif

#if defined(GLOG_USE_GLOG_EXPORT)
#  include "glog/export.h"
#endif

#if !defined(GLOG_NO_EXPORT)
#  error "symbolize.h" was not included correctly.
#endif

// We prefer to let the build system detect the availability of certain features
// such as symbolization support. HAVE_SYMBOLIZE should therefore be defined by
// the build system in general unless there is a good reason to perform the
// detection using the preprocessor.
#ifndef GLOG_NO_SYMBOLIZE_DETECTION
#  ifndef HAVE_SYMBOLIZE
// defined by gcc
#    if defined(HAVE_ELF_H) || defined(HAVE_SYS_EXEC_ELF_H)
#      define HAVE_SYMBOLIZE
#    elif defined(GLOG_OS_MACOSX) && defined(HAVE_DLADDR)
// Use dladdr to symbolize.
#      define HAVE_SYMBOLIZE
#    elif defined(GLOG_OS_WINDOWS)
// Use DbgHelp to symbolize
#      define HAVE_SYMBOLIZE
#    endif
#  endif  // !defined(HAVE_SYMBOLIZE)
#endif    // !defined(GLOG_NO_SYMBOLIZE_DETECTION)

#ifdef HAVE_SYMBOLIZE

#  if !defined(SIZEOF_VOID_P) && defined(__SIZEOF_POINTER__)
#    define SIZEOF_VOID_P __SIZEOF_POINTER__
#  endif

#  if defined(HAVE_ELF_H) || defined(HAVE_SYS_EXEC_ELF_H)

// If there is no ElfW macro, let's define it by ourself.
#    ifndef ElfW
#      if SIZEOF_VOID_P == 4
#        define ElfW(type) Elf32_##type
#      elif SIZEOF_VOID_P == 8
#        define ElfW(type) Elf64_##type
#      else
#        error "Unknown sizeof(void *)"
#      endif
#    endif

namespace google {
inline namespace glog_internal_namespace_ {

// Gets the section header for the given name, if it exists. Returns true on
// success. Otherwise, returns false.
GLOG_NO_EXPORT
bool GetSectionHeaderByName(int fd, const char* name, size_t name_len,
                            ElfW(Shdr) * out);

}  // namespace glog_internal_namespace_
}  // namespace google

#  endif

namespace google {
inline namespace glog_internal_namespace_ {

// Restrictions on the callbacks that follow:
//  - The callbacks must not use heaps but only use stacks.
//  - The callbacks must be async-signal-safe.

// Installs a callback function, which will be called right before a symbol name
// is printed. The callback is intended to be used for showing a file name and a
// line number preceding a symbol name.
// "fd" is a file descriptor of the object file containing the program
// counter "pc". The callback function should write output to "out"
// and return the size of the output written. On error, the callback
// function should return -1.
using SymbolizeCallback = int (*)(int, void*, char*, size_t, uint64_t);
GLOG_NO_EXPORT
void InstallSymbolizeCallback(SymbolizeCallback callback);

// Installs a callback function, which will be called instead of
// OpenObjectFileContainingPcAndGetStartAddress.  The callback is expected
// to searches for the object file (from /proc/self/maps) that contains
// the specified pc.  If found, sets |start_address| to the start address
// of where this object file is mapped in memory, sets the module base
// address into |base_address|, copies the object file name into
// |out_file_name|, and attempts to open the object file.  If the object
// file is opened successfully, returns the file descriptor.  Otherwise,
// returns -1.  |out_file_name_size| is the size of the file name buffer
// (including the null-terminator).
using SymbolizeOpenObjectFileCallback = int (*)(uint64_t, uint64_t&, uint64_t&,
                                                char*, size_t);
GLOG_NO_EXPORT
void InstallSymbolizeOpenObjectFileCallback(
    SymbolizeOpenObjectFileCallback callback);

}  // namespace glog_internal_namespace_
}  // namespace google

#endif

namespace google {
inline namespace glog_internal_namespace_ {

#if defined(HAVE_SYMBOLIZE)

enum class SymbolizeOptions {
  // No additional options.
  kNone = 0,
  // Do not display source and line numbers in the symbolized output.
  kNoLineNumbers = 1
};

constexpr SymbolizeOptions operator&(SymbolizeOptions lhs,
                                     SymbolizeOptions rhs) noexcept {
  return static_cast<SymbolizeOptions>(
      static_cast<std::underlying_type_t<SymbolizeOptions>>(lhs) &
      static_cast<std::underlying_type_t<SymbolizeOptions>>(rhs));
}

constexpr SymbolizeOptions operator|(SymbolizeOptions lhs,
                                     SymbolizeOptions rhs) noexcept {
  return static_cast<SymbolizeOptions>(
      static_cast<std::underlying_type_t<SymbolizeOptions>>(lhs) |
      static_cast<std::underlying_type_t<SymbolizeOptions>>(rhs));
}

// Symbolizes a program counter.  On success, returns true and write the
// symbol name to "out".  The symbol name is demangled if possible
// (supports symbols generated by GCC 3.x or newer).  Otherwise,
// returns false.
GLOG_NO_EXPORT bool Symbolize(
    void* pc, char* out, size_t out_size,
    SymbolizeOptions options = SymbolizeOptions::kNone);

#endif  // defined(HAVE_SYMBOLIZE)

}  // namespace glog_internal_namespace_
}  // namespace google

#endif  // GLOG_INTERNAL_SYMBOLIZE_H
