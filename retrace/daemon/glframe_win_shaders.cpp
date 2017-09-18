/**************************************************************************
 *
 * Copyright 2015 Intel Corporation
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Authors:
 *   Mark Janes <mark.a.janes@intel.com>
 **************************************************************************/

#include "glframe_stderr.hpp"

#include <iostream>
#include <sstream>
#include "windows.h"

#include "glframe_state.hpp"

using glretrace::WinShaders;
using glretrace::StateTrack;

void WinShaders::init() {
  // open the directory
  const DWORD pid = GetCurrentProcessId();
  std::stringstream shader_dump_dir;
  shader_dump_dir << "C:\\Intel\\IGC\\frameretrace_server.exe_" << pid << "\\";
  m_dump_dir = shader_dump_dir.str();
  shader_dump_dir << "*";
  m_dump_pattern = shader_dump_dir.str();
  poll(0, NULL);
}


void 
WinShaders::poll(int current_program, StateTrack *cb) {
  HANDLE dir_handle;
  WIN32_FIND_DATA found;
  dir_handle = FindFirstFile(m_dump_pattern.c_str(), &found);

  bool valid = dir_handle != INVALID_HANDLE_VALUE;
  std::vector<char> buf(1024);
  while (valid) {
    std::stringstream full_path_s;
    while (valid && found.cFileName[0] == '.')
      // skip .. and .
      valid = FindNextFile(dir_handle, &found);

    if (!valid)
      break;

    full_path_s << m_dump_dir << found.cFileName;
    const std::string full_path(full_path_s.str());
    ShaderType st = kShaderTypeUnknown;
    AssemblyType at = kAssemblyTypeUnknown;
    if (strncmp("VS_", found.cFileName, 3) == 0)
      st = kVertex;
    else if (strncmp("PS_", found.cFileName, 3) == 0)
      st = kFragment;

    if (st != kShaderTypeUnknown) {
      if (strstr(found.cFileName, "_simd8.asm") != 0)
        at = kSimd8;
      else if (strstr(found.cFileName, "_simd16.asm") != 0)
        at = kSimd16;
      else if (strstr(found.cFileName, "_simd32.asm") != 0)
        at = kSimd32;
      else if (strstr(found.cFileName, "_push_analysis.ll") != 0)
        at = kPushAnalysis;
      else if (strstr(found.cFileName, "_optimized.ll") != 0)
        at = kOptimized;
      else if (strstr(found.cFileName, "_layout.ll") != 0)
        at = kLayout;
      else if (strstr(found.cFileName, "_genir-lowering.ll") != 0)
        at = kGenIrLowering;
      else if (strstr(found.cFileName, "_constcoalescing.ll") != 0)
        at = kConstCoalescing;
      else if (strstr(found.cFileName, "_CodeHoisting.ll") != 0)
        at = kCodeHoisting;
      else if (strstr(found.cFileName, "_beforeUnification.ll") != 0)
        at = kBeforeUnification;
      else if (strstr(found.cFileName, "_beforeOptimization.ll") != 0)
        at = kBeforeOptimization;
      else if (strstr(found.cFileName, "_afterUnification.ll") != 0)
        at = kAfterUnification;
      else if (strstr(found.cFileName, ".ll") != 0)
        at = kOriginal;
    }

    if (cb && st != kShaderTypeUnknown && at != kAssemblyTypeUnknown) {
      // read file, make callback
      std::ifstream t(full_path);
      std::stringstream content;
      content << t.rdbuf();
      cb->onAssembly(st, at, content.str());
    }
    
    if (strlen(found.cFileName) > 2) {
      DeleteFile(full_path.c_str());
    }
    valid = FindNextFile(dir_handle, &found);
  }

  // delete the directory handle
  if (dir_handle != INVALID_HANDLE_VALUE)
    FindClose(dir_handle);
}

