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
// Author: Ray Sidney and many others
//
// Broken out from logging.cc by Soren Lassen
// logging_unittest.cc covers the functionality herein

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <string>

#include "glog/raw_logging.h"

// glog doesn't have annotation
#define ANNOTATE_BENIGN_RACE(address, description)

using std::string;

namespace google {

inline namespace glog_internal_namespace_ {

// Optimized implementation of fnmatch that does not require 0-termination
// of its arguments and does not allocate any memory.
// It supports only "*" and "?" wildcards.
// This version is implemented iteratively rather than recursively.
GLOG_NO_EXPORT bool SafeFNMatch_(const char* pattern, size_t patt_len,
                                 const char* str, size_t str_len) {
  size_t p = 0, s = 0;
  // star_idx holds the index of the last '*' encountered.
  // match_idx holds the index in str corresponding to that '*' match.
  size_t star_idx = std::numeric_limits<size_t>::max();
  size_t match_idx = 0;

  while (s < str_len) {
    if (p < patt_len && (pattern[p] == str[s] || pattern[p] == '?')) {
      // Characters match (or we have a '?') so advance both indices.
      ++p;
      ++s;
    } else if (p < patt_len && pattern[p] == '*') {
      // Record the position of '*' and the current string index.
      star_idx = p;
      match_idx = s;
      ++p;
    } else if (star_idx != std::numeric_limits<size_t>::max()) {
      // No direct match, but we have seen a '*' before.
      // Backtrack: assume '*' matches one more character.
      p = star_idx + 1;
      s = ++match_idx;
    } else {
      // No match and no '*' to backtrack to.
      return false;
    }
  }

  // Check for remaining '*' in the pattern.
  while (p < patt_len && pattern[p] == '*') {
    ++p;
  }
  return p == patt_len;
}

}  // namespace glog_internal_namespace_

using glog_internal_namespace_::SafeFNMatch_;

// Structure holding per-module logging level info.
struct VModuleInfo {
  string module_pattern;
  mutable int32 vlog_level;  // Conceptually atomic but kept simple for performance.
  const VModuleInfo* next;
};

// Global variables controlling per-module logging levels.
static std::mutex vmodule_mutex;
static VModuleInfo* vmodule_list = nullptr;
static SiteFlag* cached_site_list = nullptr;
static bool inited_vmodule = false;

// Initializes the module-specific logging levels based on FLAGS_vmodule.
static void VLOG2Initializer() {
  inited_vmodule = false;
  const char* vmodule = FLAGS_vmodule.c_str();
  VModuleInfo* head = nullptr;
  VModuleInfo* tail = nullptr;
  while (*vmodule != '\0') {
    const char* sep = strchr(vmodule, '=');
    if (sep == nullptr) break;
    string pattern(vmodule, static_cast<size_t>(sep - vmodule));
    int module_level;
    if (sscanf(sep, "=%d", &module_level) == 1) {
      auto* info = new VModuleInfo;
      info->module_pattern = pattern;
      info->vlog_level = module_level;
      info->next = nullptr;
      if (head) {
        tail->next = info;
      } else {
        head = info;
      }
      tail = info;
    }
    // Skip past this entry (find the next comma).
    vmodule = strchr(sep, ',');
    if (vmodule == nullptr) break;
    ++vmodule;  // Skip the comma.
  }
  if (head) {
    tail->next = vmodule_list;
    vmodule_list = head;
  }
  inited_vmodule = true;
}

// Sets the VLOG level for a given module pattern.
int SetVLOGLevel(const char* module_pattern, int log_level) {
  int result = FLAGS_v;
  const size_t pattern_len = strlen(module_pattern);
  bool found = false;
  {
    std::lock_guard<std::mutex> l(vmodule_mutex);
    for (const VModuleInfo* info = vmodule_list; info != nullptr; info = info->next) {
      if (info->module_pattern == module_pattern) {
        if (!found) {
          result = info->vlog_level;
          found = true;
        }
        info->vlog_level = log_level;
      } else if (!found &&
                 SafeFNMatch_(info->module_pattern.c_str(),
                              info->module_pattern.size(),
                              module_pattern, pattern_len)) {
        result = info->vlog_level;
        found = true;
      }
    }
    if (!found) {
      auto* info = new VModuleInfo;
      info->module_pattern = module_pattern;
      info->vlog_level = log_level;
      info->next = vmodule_list;
      vmodule_list = info;

      // Update any cached site flags that match this module.
      SiteFlag** item_ptr = &cached_site_list;
      SiteFlag* item = cached_site_list;
      while (item) {
        if (SafeFNMatch_(module_pattern, pattern_len, item->base_name,
                         item->base_len)) {
          item->level = &info->vlog_level;
          *item_ptr = item->next;  // Remove the item from the list.
        } else {
          item_ptr = &item->next;
        }
        item = *item_ptr;
      }
    }
  }
  RAW_VLOG(1, "Set VLOG level for \"%s\" to %d", module_pattern, log_level);
  return result;
}

// Initializes the VLOG site flag and returns whether logging should occur.
bool InitVLOG3__(SiteFlag* site_flag, int32* level_default, const char* fname,
                 int32 verbose_level) {
  std::lock_guard<std::mutex> l(vmodule_mutex);
  bool read_vmodule_flag = inited_vmodule;
  if (!read_vmodule_flag) {
    VLOG2Initializer();
  }

  // Save errno in case any recoverable error occurs.
  int old_errno = errno;
  int32* site_flag_value = level_default;

  // Get the base file name (strip directory path).
  const char* base = strrchr(fname, '/');
#ifdef _WIN32
  if (!base) {
    base = strrchr(fname, '\\');
  }
#endif
  base = base ? (base + 1) : fname;
  const char* base_end = strchr(base, '.');
  size_t base_length = base_end ? static_cast<size_t>(base_end - base) : strlen(base);

  // Trim any trailing "-inl" if present.
  if (base_length >= 4 && memcmp(base + base_length - 4, "-inl", 4) == 0) {
    base_length -= 4;
  }

  // Search for a matching module override.
  for (const VModuleInfo* info = vmodule_list; info != nullptr; info = info->next) {
    if (SafeFNMatch_(info->module_pattern.c_str(), info->module_pattern.size(),
                     base, base_length)) {
      site_flag_value = &info->vlog_level;
      break;
    }
  }

  // Cache the level pointer in the site flag.
  ANNOTATE_BENIGN_RACE(site_flag,
      "*site_flag may be written by several threads, but the value will be the same");
  if (read_vmodule_flag) {
    site_flag->level = site_flag_value;
    if (site_flag_value == level_default && !site_flag->base_name) {
      site_flag->base_name = base;
      site_flag->base_len = base_length;
      site_flag->next = cached_site_list;
      cached_site_list = site_flag;
    }
  }

  // Restore errno and return whether logging should proceed.
  errno = old_errno;
  return *site_flag_value >= verbose_level;
}

}  // namespace google
