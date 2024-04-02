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
// Stack-footprint reduction work done by Raksit Ashok
//
// Implementation note:
//
// We don't use heaps but only use stacks.  We want to reduce the
// stack consumption so that the symbolizer can run on small stacks.
//
// Here are some numbers collected with GCC 4.1.0 on x86:
// - sizeof(Elf32_Sym)  = 16
// - sizeof(Elf32_Shdr) = 40
// - sizeof(Elf64_Sym)  = 24
// - sizeof(Elf64_Shdr) = 64
//
// This implementation is intended to be async-signal-safe but uses
// some functions which are not guaranteed to be so, such as memchr()
// and memmove().  We assume they are async-signal-safe.
//
// Additional header can be specified by the GLOG_BUILD_CONFIG_INCLUDE
// macro to add platform specific defines (e.g. GLOG_OS_OPENBSD).

#ifdef GLOG_BUILD_CONFIG_INCLUDE
#  include GLOG_BUILD_CONFIG_INCLUDE
#endif  // GLOG_BUILD_CONFIG_INCLUDE

#include "symbolize.h"

#include "utilities.h"

#if defined(HAVE_SYMBOLIZE)

#  include <algorithm>
#  include <cstdlib>
#  include <cstring>
#  include <limits>

#  include "demangle.h"

// We don't use assert() since it's not guaranteed to be
// async-signal-safe.  Instead we define a minimal assertion
// macro. So far, we don't need pretty printing for __FILE__, etc.
#  define GLOG_SAFE_ASSERT(expr) ((expr) ? 0 : (std::abort(), 0))

namespace google {
inline namespace glog_internal_namespace_ {

namespace {

SymbolizeCallback g_symbolize_callback = nullptr;
SymbolizeOpenObjectFileCallback g_symbolize_open_object_file_callback = nullptr;

// This function wraps the Demangle function to provide an interface
// where the input symbol is demangled in-place.
// To keep stack consumption low, we would like this function to not
// get inlined.
ATTRIBUTE_NOINLINE
void DemangleInplace(char* out, size_t out_size) {
  char demangled[256];  // Big enough for sane demangled symbols.
  if (Demangle(out, demangled, sizeof(demangled))) {
    // Demangling succeeded. Copy to out if the space allows.
    size_t len = strlen(demangled);
    if (len + 1 <= out_size) {  // +1 for '\0'.
      GLOG_SAFE_ASSERT(len < sizeof(demangled));
      memmove(out, demangled, len + 1);
    }
  }
}

}  // namespace

void InstallSymbolizeCallback(SymbolizeCallback callback) {
  g_symbolize_callback = callback;
}

void InstallSymbolizeOpenObjectFileCallback(
    SymbolizeOpenObjectFileCallback callback) {
  g_symbolize_open_object_file_callback = callback;
}

}  // namespace glog_internal_namespace_
}  // namespace google

#  if defined(HAVE_LINK_H)

#    if defined(HAVE_DLFCN_H)
#      include <dlfcn.h>
#    endif
#    include <fcntl.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <unistd.h>

#    include <cerrno>
#    include <climits>
#    include <cstddef>
#    include <cstdint>
#    include <cstdio>
#    include <cstdlib>
#    include <cstring>

#    include "config.h"
#    include "glog/raw_logging.h"
#    include "symbolize.h"

namespace google {
inline namespace glog_internal_namespace_ {

namespace {

// Re-runs run until it doesn't cause EINTR.
// Similar to the TEMP_FAILURE_RETRY macro from GNU C.
template <class Functor>
auto FailureRetry(Functor run, int error = EINTR) noexcept(noexcept(run())) {
  decltype(run()) result;

  while ((result = run()) == -1 && errno == error) {
  }

  return result;
}

}  // namespace

// Read up to "count" bytes from "offset" in the file pointed by file
// descriptor "fd" into the buffer starting at "buf" while handling short reads
// and EINTR.  On success, return the number of bytes read.  Otherwise, return
// -1.
static ssize_t ReadFromOffset(const int fd, void* buf, const size_t count,
                              const size_t offset) {
  GLOG_SAFE_ASSERT(fd >= 0);
  GLOG_SAFE_ASSERT(count <=
                   static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
  char* buf0 = reinterpret_cast<char*>(buf);
  size_t num_bytes = 0;
  while (num_bytes < count) {
    ssize_t len = FailureRetry([fd, p = buf0 + num_bytes, n = count - num_bytes,
                                m = static_cast<off_t>(offset + num_bytes)] {
      return pread(fd, p, n, m);
    });
    if (len < 0) {  // There was an error other than EINTR.
      return -1;
    }
    if (len == 0) {  // Reached EOF.
      break;
    }
    num_bytes += static_cast<size_t>(len);
  }
  GLOG_SAFE_ASSERT(num_bytes <= count);
  return static_cast<ssize_t>(num_bytes);
}

// Try reading exactly "count" bytes from "offset" bytes in a file
// pointed by "fd" into the buffer starting at "buf" while handling
// short reads and EINTR.  On success, return true. Otherwise, return
// false.
static bool ReadFromOffsetExact(const int fd, void* buf, const size_t count,
                                const size_t offset) {
  ssize_t len = ReadFromOffset(fd, buf, count, offset);
  return static_cast<size_t>(len) == count;
}

// Returns elf_header.e_type if the file pointed by fd is an ELF binary.
static int FileGetElfType(const int fd) {
  ElfW(Ehdr) elf_header;
  if (!ReadFromOffsetExact(fd, &elf_header, sizeof(elf_header), 0)) {
    return -1;
  }
  if (memcmp(elf_header.e_ident, ELFMAG, SELFMAG) != 0) {
    return -1;
  }
  return elf_header.e_type;
}

// Read the section headers in the given ELF binary, and if a section
// of the specified type is found, set the output to this section header
// and return true.  Otherwise, return false.
// To keep stack consumption low, we would like this function to not get
// inlined.
static ATTRIBUTE_NOINLINE bool GetSectionHeaderByType(const int fd,
                                                      ElfW(Half) sh_num,
                                                      const size_t sh_offset,
                                                      ElfW(Word) type,
                                                      ElfW(Shdr) * out) {
  // Read at most 16 section headers at a time to save read calls.
  ElfW(Shdr) buf[16];
  for (size_t i = 0; i < sh_num;) {
    const size_t num_bytes_left = (sh_num - i) * sizeof(buf[0]);
    const size_t num_bytes_to_read =
        (sizeof(buf) > num_bytes_left) ? num_bytes_left : sizeof(buf);
    const ssize_t len = ReadFromOffset(fd, buf, num_bytes_to_read,
                                       sh_offset + i * sizeof(buf[0]));
    if (len == -1) {
      return false;
    }
    GLOG_SAFE_ASSERT(static_cast<size_t>(len) % sizeof(buf[0]) == 0);
    const size_t num_headers_in_buf = static_cast<size_t>(len) / sizeof(buf[0]);
    GLOG_SAFE_ASSERT(num_headers_in_buf <= sizeof(buf) / sizeof(buf[0]));
    for (size_t j = 0; j < num_headers_in_buf; ++j) {
      if (buf[j].sh_type == type) {
        *out = buf[j];
        return true;
      }
    }
    i += num_headers_in_buf;
  }
  return false;
}

// There is no particular reason to limit section name to 63 characters,
// but there has (as yet) been no need for anything longer either.
const int kMaxSectionNameLen = 64;

// name_len should include terminating '\0'.
bool GetSectionHeaderByName(int fd, const char* name, size_t name_len,
                            ElfW(Shdr) * out) {
  ElfW(Ehdr) elf_header;
  if (!ReadFromOffsetExact(fd, &elf_header, sizeof(elf_header), 0)) {
    return false;
  }

  ElfW(Shdr) shstrtab;
  size_t shstrtab_offset =
      (elf_header.e_shoff + static_cast<size_t>(elf_header.e_shentsize) *
                                static_cast<size_t>(elf_header.e_shstrndx));
  if (!ReadFromOffsetExact(fd, &shstrtab, sizeof(shstrtab), shstrtab_offset)) {
    return false;
  }

  for (size_t i = 0; i < elf_header.e_shnum; ++i) {
    size_t section_header_offset =
        (elf_header.e_shoff + elf_header.e_shentsize * i);
    if (!ReadFromOffsetExact(fd, out, sizeof(*out), section_header_offset)) {
      return false;
    }
    char header_name[kMaxSectionNameLen];
    if (sizeof(header_name) < name_len) {
      RAW_LOG(WARNING,
              "Section name '%s' is too long (%zu); "
              "section will not be found (even if present).",
              name, name_len);
      // No point in even trying.
      return false;
    }
    size_t name_offset = shstrtab.sh_offset + out->sh_name;
    ssize_t n_read = ReadFromOffset(fd, &header_name, name_len, name_offset);
    if (n_read == -1) {
      return false;
    } else if (static_cast<size_t>(n_read) != name_len) {
      // Short read -- name could be at end of file.
      continue;
    }
    if (memcmp(header_name, name, name_len) == 0) {
      return true;
    }
  }
  return false;
}

// Read a symbol table and look for the symbol containing the
// pc. Iterate over symbols in a symbol table and look for the symbol
// containing "pc".  On success, return true and write the symbol name
// to out.  Otherwise, return false.
// To keep stack consumption low, we would like this function to not get
// inlined.
static ATTRIBUTE_NOINLINE bool FindSymbol(uint64_t pc, const int fd, char* out,
                                          size_t out_size,
                                          uint64_t symbol_offset,
                                          const ElfW(Shdr) * strtab,
                                          const ElfW(Shdr) * symtab) {
  if (symtab == nullptr) {
    return false;
  }
  const size_t num_symbols = symtab->sh_size / symtab->sh_entsize;
  for (unsigned i = 0; i < num_symbols;) {
    size_t offset = symtab->sh_offset + i * symtab->sh_entsize;

    // If we are reading Elf64_Sym's, we want to limit this array to
    // 32 elements (to keep stack consumption low), otherwise we can
    // have a 64 element Elf32_Sym array.
#    if defined(__WORDSIZE) && __WORDSIZE == 64
    const size_t NUM_SYMBOLS = 32U;
#    else
    const size_t NUM_SYMBOLS = 64U;
#    endif

    // Read at most NUM_SYMBOLS symbols at once to save read() calls.
    ElfW(Sym) buf[NUM_SYMBOLS];
    size_t num_symbols_to_read = std::min(NUM_SYMBOLS, num_symbols - i);
    const ssize_t len =
        ReadFromOffset(fd, &buf, sizeof(buf[0]) * num_symbols_to_read, offset);
    GLOG_SAFE_ASSERT(static_cast<size_t>(len) % sizeof(buf[0]) == 0);
    const size_t num_symbols_in_buf = static_cast<size_t>(len) / sizeof(buf[0]);
    GLOG_SAFE_ASSERT(num_symbols_in_buf <= num_symbols_to_read);
    for (unsigned j = 0; j < num_symbols_in_buf; ++j) {
      const ElfW(Sym)& symbol = buf[j];
      uint64_t start_address = symbol.st_value;
      start_address += symbol_offset;
      uint64_t end_address = start_address + symbol.st_size;
      if (symbol.st_value != 0 &&  // Skip null value symbols.
          symbol.st_shndx != 0 &&  // Skip undefined symbols.
          start_address <= pc && pc < end_address) {
        ssize_t len1 = ReadFromOffset(fd, out, out_size,
                                      strtab->sh_offset + symbol.st_name);
        if (len1 <= 0 || memchr(out, '\0', out_size) == nullptr) {
          memset(out, 0, out_size);
          return false;
        }
        return true;  // Obtained the symbol name.
      }
    }
    i += num_symbols_in_buf;
  }
  return false;
}

// Get the symbol name of "pc" from the file pointed by "fd".  Process
// both regular and dynamic symbol tables if necessary.  On success,
// write the symbol name to "out" and return true.  Otherwise, return
// false.
static bool GetSymbolFromObjectFile(const int fd, uint64_t pc, char* out,
                                    size_t out_size, uint64_t base_address) {
  // Read the ELF header.
  ElfW(Ehdr) elf_header;
  if (!ReadFromOffsetExact(fd, &elf_header, sizeof(elf_header), 0)) {
    return false;
  }

  ElfW(Shdr) symtab, strtab;

  // Consult a regular symbol table first.
  if (GetSectionHeaderByType(fd, elf_header.e_shnum, elf_header.e_shoff,
                             SHT_SYMTAB, &symtab)) {
    if (!ReadFromOffsetExact(
            fd, &strtab, sizeof(strtab),
            elf_header.e_shoff + symtab.sh_link * sizeof(symtab))) {
      return false;
    }
    if (FindSymbol(pc, fd, out, out_size, base_address, &strtab, &symtab)) {
      return true;  // Found the symbol in a regular symbol table.
    }
  }

  // If the symbol is not found, then consult a dynamic symbol table.
  if (GetSectionHeaderByType(fd, elf_header.e_shnum, elf_header.e_shoff,
                             SHT_DYNSYM, &symtab)) {
    if (!ReadFromOffsetExact(
            fd, &strtab, sizeof(strtab),
            elf_header.e_shoff + symtab.sh_link * sizeof(symtab))) {
      return false;
    }
    if (FindSymbol(pc, fd, out, out_size, base_address, &strtab, &symtab)) {
      return true;  // Found the symbol in a dynamic symbol table.
    }
  }

  return false;
}

namespace {

// Helper class for reading lines from file.
//
// Note: we don't use ProcMapsIterator since the object is big (it has
// a 5k array member) and uses async-unsafe functions such as sscanf()
// and std::snprintf().
class LineReader {
 public:
  explicit LineReader(int fd, char* buf, size_t buf_len, size_t offset)
      : fd_(fd),
        buf_(buf),
        buf_len_(buf_len),
        offset_(offset),
        bol_(buf),
        eol_(buf),
        eod_(buf) {}

  // Read '\n'-terminated line from file.  On success, modify "bol"
  // and "eol", then return true.  Otherwise, return false.
  //
  // Note: if the last line doesn't end with '\n', the line will be
  // dropped.  It's an intentional behavior to make the code simple.
  bool ReadLine(const char** bol, const char** eol) {
    if (BufferIsEmpty()) {  // First time.
      const ssize_t num_bytes = ReadFromOffset(fd_, buf_, buf_len_, offset_);
      if (num_bytes <= 0) {  // EOF or error.
        return false;
      }
      offset_ += static_cast<size_t>(num_bytes);
      eod_ = buf_ + num_bytes;
      bol_ = buf_;
    } else {
      bol_ = eol_ + 1;  // Advance to the next line in the buffer.
      GLOG_SAFE_ASSERT(bol_ <= eod_);  // "bol_" can point to "eod_".
      if (!HasCompleteLine()) {
        const auto incomplete_line_length = static_cast<size_t>(eod_ - bol_);
        // Move the trailing incomplete line to the beginning.
        memmove(buf_, bol_, incomplete_line_length);
        // Read text from file and append it.
        char* const append_pos = buf_ + incomplete_line_length;
        const size_t capacity_left = buf_len_ - incomplete_line_length;
        const ssize_t num_bytes =
            ReadFromOffset(fd_, append_pos, capacity_left, offset_);
        if (num_bytes <= 0) {  // EOF or error.
          return false;
        }
        offset_ += static_cast<size_t>(num_bytes);
        eod_ = append_pos + num_bytes;
        bol_ = buf_;
      }
    }
    eol_ = FindLineFeed();
    if (eol_ == nullptr) {  // '\n' not found.  Malformed line.
      return false;
    }
    *eol_ = '\0';  // Replace '\n' with '\0'.

    *bol = bol_;
    *eol = eol_;
    return true;
  }

  // Beginning of line.
  const char* bol() { return bol_; }

  // End of line.
  const char* eol() { return eol_; }

 private:
  LineReader(const LineReader&) = delete;
  void operator=(const LineReader&) = delete;

  char* FindLineFeed() {
    return reinterpret_cast<char*>(
        memchr(bol_, '\n', static_cast<size_t>(eod_ - bol_)));
  }

  bool BufferIsEmpty() { return buf_ == eod_; }

  bool HasCompleteLine() {
    return !BufferIsEmpty() && FindLineFeed() != nullptr;
  }

  const int fd_;
  char* const buf_;
  const size_t buf_len_;
  size_t offset_;
  char* bol_;
  char* eol_;
  const char* eod_;  // End of data in "buf_".
};
}  // namespace

// Place the hex number read from "start" into "*hex".  The pointer to
// the first non-hex character or "end" is returned.
static char* GetHex(const char* start, const char* end, uint64_t* hex) {
  *hex = 0;
  const char* p;
  for (p = start; p < end; ++p) {
    int ch = *p;
    if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') ||
        (ch >= 'a' && ch <= 'f')) {
      *hex = (*hex << 4U) |
             (ch < 'A' ? static_cast<uint64_t>(ch - '0') : (ch & 0xF) + 9U);
    } else {  // Encountered the first non-hex character.
      break;
    }
  }
  GLOG_SAFE_ASSERT(p <= end);
  return const_cast<char*>(p);
}

// Searches for the object file (from /proc/self/maps) that contains
// the specified pc.  If found, sets |start_address| to the start address
// of where this object file is mapped in memory, sets the module base
// address into |base_address|, copies the object file name into
// |out_file_name|, and attempts to open the object file.  If the object
// file is opened successfully, returns the file descriptor.  Otherwise,
// returns -1.  |out_file_name_size| is the size of the file name buffer
// (including the null-terminator).
static ATTRIBUTE_NOINLINE FileDescriptor
OpenObjectFileContainingPcAndGetStartAddress(uint64_t pc,
                                             uint64_t& start_address,
                                             uint64_t& base_address,
                                             char* out_file_name,
                                             size_t out_file_name_size) {
  FileDescriptor maps_fd{
      FailureRetry([] { return open("/proc/self/maps", O_RDONLY); })};
  if (!maps_fd) {
    return nullptr;
  }

  FileDescriptor mem_fd{
      FailureRetry([] { return open("/proc/self/mem", O_RDONLY); })};
  if (!mem_fd) {
    return nullptr;
  }

  // Iterate over maps and look for the map containing the pc.  Then
  // look into the symbol tables inside.
  char buf[1024];  // Big enough for line of sane /proc/self/maps
  LineReader reader(maps_fd.get(), buf, sizeof(buf), 0);
  while (true) {
    const char* cursor;
    const char* eol;
    if (!reader.ReadLine(&cursor, &eol)) {  // EOF or malformed line.
      return nullptr;
    }

    // Start parsing line in /proc/self/maps.  Here is an example:
    //
    // 08048000-0804c000 r-xp 00000000 08:01 2142121    /bin/cat
    //
    // We want start address (08048000), end address (0804c000), flags
    // (r-xp) and file name (/bin/cat).

    // Read start address.
    cursor = GetHex(cursor, eol, &start_address);
    if (cursor == eol || *cursor != '-') {
      return nullptr;  // Malformed line.
    }
    ++cursor;  // Skip '-'.

    // Read end address.
    uint64_t end_address;
    cursor = GetHex(cursor, eol, &end_address);
    if (cursor == eol || *cursor != ' ') {
      return nullptr;  // Malformed line.
    }
    ++cursor;  // Skip ' '.

    // Read flags.  Skip flags until we encounter a space or eol.
    const char* const flags_start = cursor;
    while (cursor < eol && *cursor != ' ') {
      ++cursor;
    }
    // We expect at least four letters for flags (ex. "r-xp").
    if (cursor == eol || cursor < flags_start + 4) {
      return nullptr;  // Malformed line.
    }

    // Determine the base address by reading ELF headers in process memory.
    ElfW(Ehdr) ehdr;
    // Skip non-readable maps.
    if (flags_start[0] == 'r' &&
        ReadFromOffsetExact(mem_fd.get(), &ehdr, sizeof(ElfW(Ehdr)),
                            start_address) &&
        memcmp(ehdr.e_ident, ELFMAG, SELFMAG) == 0) {
      switch (ehdr.e_type) {
        case ET_EXEC:
          base_address = 0;
          break;
        case ET_DYN:
          // Find the segment containing file offset 0. This will correspond
          // to the ELF header that we just read. Normally this will have
          // virtual address 0, but this is not guaranteed. We must subtract
          // the virtual address from the address where the ELF header was
          // mapped to get the base address.
          //
          // If we fail to find a segment for file offset 0, use the address
          // of the ELF header as the base address.
          base_address = start_address;
          for (unsigned i = 0; i != ehdr.e_phnum; ++i) {
            ElfW(Phdr) phdr;
            if (ReadFromOffsetExact(
                    mem_fd.get(), &phdr, sizeof(phdr),
                    start_address + ehdr.e_phoff + i * sizeof(phdr)) &&
                phdr.p_type == PT_LOAD && phdr.p_offset == 0) {
              base_address = start_address - phdr.p_vaddr;
              break;
            }
          }
          break;
        default:
          // ET_REL or ET_CORE. These aren't directly executable, so they don't
          // affect the base address.
          break;
      }
    }

    // Check start and end addresses.
    if (start_address > pc || pc >= end_address) {
      continue;  // We skip this map.  PC isn't in this map.
    }

    // Check flags.  We are only interested in "r*x" maps.
    if (flags_start[0] != 'r' || flags_start[2] != 'x') {
      continue;  // We skip this map.
    }
    ++cursor;  // Skip ' '.

    // Read file offset.
    uint64_t file_offset;
    cursor = GetHex(cursor, eol, &file_offset);
    if (cursor == eol || *cursor != ' ') {
      return nullptr;  // Malformed line.
    }
    ++cursor;  // Skip ' '.

    // Skip to file name.  "cursor" now points to dev.  We need to
    // skip at least two spaces for dev and inode.
    int num_spaces = 0;
    while (cursor < eol) {
      if (*cursor == ' ') {
        ++num_spaces;
      } else if (num_spaces >= 2) {
        // The first non-space character after skipping two spaces
        // is the beginning of the file name.
        break;
      }
      ++cursor;
    }
    if (cursor == eol) {
      return nullptr;  // Malformed line.
    }

    strncpy(out_file_name, cursor, out_file_name_size);
    // Making sure |out_file_name| is always null-terminated.
    out_file_name[out_file_name_size - 1] = '\0';

    // Finally, "cursor" now points to file name of our interest.
    return FileDescriptor{
        FailureRetry([cursor] { return open(cursor, O_RDONLY); })};
  }
}

// POSIX doesn't define any async-signal safe function for converting
// an integer to ASCII. We'll have to define our own version.
// itoa_r() converts an (unsigned) integer to ASCII. It returns "buf", if the
// conversion was successful or nullptr otherwise. It never writes more than
// "sz" bytes. Output will be truncated as needed, and a NUL character is always
// appended.
// NOTE: code from sandbox/linux/seccomp-bpf/demo.cc.
static char* itoa_r(uintptr_t i, char* buf, size_t sz, unsigned base,
                    size_t padding) {
  // Make sure we can write at least one NUL byte.
  size_t n = 1;
  if (n > sz) {
    return nullptr;
  }

  if (base < 2 || base > 16) {
    buf[0] = '\000';
    return nullptr;
  }

  char* start = buf;

  // Loop until we have converted the entire number. Output at least one
  // character (i.e. '0').
  char* ptr = start;
  do {
    // Make sure there is still enough space left in our output buffer.
    if (++n > sz) {
      buf[0] = '\000';
      return nullptr;
    }

    // Output the next digit.
    *ptr++ = "0123456789abcdef"[i % base];
    i /= base;

    if (padding > 0) {
      padding--;
    }
  } while (i > 0 || padding > 0);

  // Terminate the output with a NUL character.
  *ptr = '\000';

  // Conversion to ASCII actually resulted in the digits being in reverse
  // order. We can't easily generate them in forward order, as we can't tell
  // the number of characters needed until we are done converting.
  // So, now, we reverse the string (except for the possible "-" sign).
  while (--ptr > start) {
    char ch = *ptr;
    *ptr = *start;
    *start++ = ch;
  }
  return buf;
}

// Safely appends string |source| to string |dest|.  Never writes past the
// buffer size |dest_size| and guarantees that |dest| is null-terminated.
static void SafeAppendString(const char* source, char* dest, size_t dest_size) {
  size_t dest_string_length = strlen(dest);
  GLOG_SAFE_ASSERT(dest_string_length < dest_size);
  dest += dest_string_length;
  dest_size -= dest_string_length;
  strncpy(dest, source, dest_size);
  // Making sure |dest| is always null-terminated.
  dest[dest_size - 1] = '\0';
}

// Converts a 64-bit value into a hex string, and safely appends it to |dest|.
// Never writes past the buffer size |dest_size| and guarantees that |dest| is
// null-terminated.
static void SafeAppendHexNumber(uint64_t value, char* dest, size_t dest_size) {
  // 64-bit numbers in hex can have up to 16 digits.
  char buf[17] = {'\0'};
  SafeAppendString(itoa_r(value, buf, sizeof(buf), 16, 0), dest, dest_size);
}

// The implementation of our symbolization routine.  If it
// successfully finds the symbol containing "pc" and obtains the
// symbol name, returns true and write the symbol name to "out".
// Otherwise, returns false. If Callback function is installed via
// InstallSymbolizeCallback(), the function is also called in this function,
// and "out" is used as its output.
// To keep stack consumption low, we would like this function to not
// get inlined.
static ATTRIBUTE_NOINLINE bool SymbolizeAndDemangle(
    void* pc, char* out, size_t out_size, SymbolizeOptions /*options*/) {
  auto pc0 = reinterpret_cast<uintptr_t>(pc);
  uint64_t start_address = 0;
  uint64_t base_address = 0;
  FileDescriptor object_fd;

  if (out_size < 1) {
    return false;
  }
  out[0] = '\0';
  SafeAppendString("(", out, out_size);

  if (g_symbolize_open_object_file_callback) {
    object_fd.reset(g_symbolize_open_object_file_callback(
        pc0, start_address, base_address, out + 1, out_size - 1));
  } else {
    object_fd = OpenObjectFileContainingPcAndGetStartAddress(
        pc0, start_address, base_address, out + 1, out_size - 1);
  }

#    if defined(PRINT_UNSYMBOLIZED_STACK_TRACES)
  {
#    else
  // Check whether a file name was returned.
  if (!object_fd) {
#    endif
    if (out[1]) {
      // The object file containing PC was determined successfully however the
      // object file was not opened successfully.  This is still considered
      // success because the object file name and offset are known and tools
      // like asan_symbolize.py can be used for the symbolization.
      out[out_size - 1] = '\0';  // Making sure |out| is always null-terminated.
      SafeAppendString("+0x", out, out_size);
      SafeAppendHexNumber(pc0 - base_address, out, out_size);
      SafeAppendString(")", out, out_size);
      return true;
    }
    // Failed to determine the object file containing PC.  Bail out.
    return false;
  }
  int elf_type = FileGetElfType(object_fd.get());
  if (elf_type == -1) {
    return false;
  }
  if (g_symbolize_callback) {
    // Run the call back if it's installed.
    // Note: relocation (and much of the rest of this code) will be
    // wrong for prelinked shared libraries and PIE executables.
    uint64_t relocation = (elf_type == ET_DYN) ? start_address : 0;
    int num_bytes_written =
        g_symbolize_callback(object_fd.get(), pc, out, out_size, relocation);
    if (num_bytes_written > 0) {
      out += static_cast<size_t>(num_bytes_written);
      out_size -= static_cast<size_t>(num_bytes_written);
    }
  }
  if (!GetSymbolFromObjectFile(object_fd.get(), pc0, out, out_size,
                               base_address)) {
    if (out[1] && !g_symbolize_callback) {
      // The object file containing PC was opened successfully however the
      // symbol was not found. The object may have been stripped. This is still
      // considered success because the object file name and offset are known
      // and tools like asan_symbolize.py can be used for the symbolization.
      out[out_size - 1] = '\0';  // Making sure |out| is always null-terminated.
      SafeAppendString("+0x", out, out_size);
      SafeAppendHexNumber(pc0 - base_address, out, out_size);
      SafeAppendString(")", out, out_size);
      return true;
    }
    return false;
  }

  // Symbolization succeeded.  Now we try to demangle the symbol.
  DemangleInplace(out, out_size);
  return true;
}

}  // namespace glog_internal_namespace_
}  // namespace google

#  elif defined(GLOG_OS_MACOSX) && defined(HAVE_DLADDR)

#    include <dlfcn.h>

#    include <cstring>

namespace google {
inline namespace glog_internal_namespace_ {

static ATTRIBUTE_NOINLINE bool SymbolizeAndDemangle(
    void* pc, char* out, size_t out_size, SymbolizeOptions /*options*/) {
  Dl_info info;
  if (dladdr(pc, &info)) {
    if (info.dli_sname) {
      if (strlen(info.dli_sname) < out_size) {
        strcpy(out, info.dli_sname);
        // Symbolization succeeded.  Now we try to demangle the symbol.
        DemangleInplace(out, out_size);
        return true;
      }
    }
  }
  return false;
}

}  // namespace glog_internal_namespace_
}  // namespace google

#  elif defined(GLOG_OS_WINDOWS) || defined(GLOG_OS_CYGWIN)

#    include <dbghelp.h>
#    include <windows.h>

namespace google {
inline namespace glog_internal_namespace_ {

namespace {

class SymInitializer final {
 public:
  HANDLE process;
  bool ready;
  SymInitializer() : process(GetCurrentProcess()), ready(false) {
    // Initialize the symbol handler.
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms680344(v=vs.85).aspx
    // Defer symbol loading.
    // We do not request undecorated symbols with SYMOPT_UNDNAME
    // because the mangling library calls UnDecorateSymbolName.
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    if (SymInitialize(process, nullptr, true)) {
      ready = true;
    }
  }
  ~SymInitializer() {
    SymCleanup(process);
    // We do not need to close `HANDLE process` because it's a "pseudo handle."
  }

  SymInitializer(const SymInitializer&) = delete;
  SymInitializer& operator=(const SymInitializer&) = delete;
  SymInitializer(SymInitializer&&) = delete;
  SymInitializer& operator=(SymInitializer&&) = delete;
};

}  // namespace

static ATTRIBUTE_NOINLINE bool SymbolizeAndDemangle(void* pc, char* out,
                                                    size_t out_size,
                                                    SymbolizeOptions options) {
  const static SymInitializer symInitializer;
  if (!symInitializer.ready) {
    return false;
  }
  // Resolve symbol information from address.
  // https://msdn.microsoft.com/en-us/library/windows/desktop/ms680578(v=vs.85).aspx
  char buf[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
  SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(buf);
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = MAX_SYM_NAME;
  // We use the ANSI version to ensure the string type is always `char *`.
  // This could break if a symbol has Unicode in it.
  BOOL ret = SymFromAddr(symInitializer.process, reinterpret_cast<DWORD64>(pc),
                         0, symbol);
  std::size_t namelen = static_cast<size_t>(symbol->NameLen);
  if (ret && namelen < out_size) {
    std::strncpy(out, symbol->Name, namelen);
    out[namelen] = '\0';

    DWORD displacement;
    IMAGEHLP_LINE64 line{sizeof(IMAGEHLP_LINE64)};

    BOOL found = FALSE;

    if ((options & SymbolizeOptions::kNoLineNumbers) !=
        SymbolizeOptions::kNoLineNumbers) {
      found = SymGetLineFromAddr64(symInitializer.process,
                                   reinterpret_cast<DWORD64>(pc), &displacement,
                                   &line);
    }

    // Symbolization succeeded.  Now we try to demangle the symbol.
    DemangleInplace(out, out_size);
    out_size -= std::strlen(out);

    if (found) {
      std::size_t fnlen = std::strlen(line.FileName);
      // Determine the number of digits (base 10) necessary to represent the
      // line number
      std::size_t digits = 1;  // At least one digit required
      for (DWORD value = line.LineNumber; (value /= 10) != 0; ++digits) {
      }
      constexpr std::size_t extralen = 4;  // space + parens () + :
      const std::size_t suffixlen = fnlen + extralen + fnlen + digits;

      if (suffixlen < out_size) {
        out_size -= std::snprintf(out + namelen, out_size, " (%s:%lu)",
                                  line.FileName, line.LineNumber);
      }
    }

    return true;
  }
  return false;
}

}  // namespace glog_internal_namespace_
}  // namespace google

#  else
#    error BUG: HAVE_SYMBOLIZE was wrongly set
#  endif

namespace google {
inline namespace glog_internal_namespace_ {

bool Symbolize(void* pc, char* out, size_t out_size, SymbolizeOptions options) {
  return SymbolizeAndDemangle(pc, out, out_size, options);
}

}  // namespace glog_internal_namespace_
}  // namespace google

#endif
