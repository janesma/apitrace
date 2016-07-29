// Copyright (C) Intel Corp.  2015.  All Rights Reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice (including the
// next paragraph) shall be included in all copies or substantial
// portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  **********************************************************************/
//  * Authors:
//  *   Mark Janes <mark.a.janes@intel.com>
//  **********************************************************************/

#include <windows.h>
#include <time.h>
#include <stdlib.h>
#include <sstream>

namespace glretrace {

int fork_execv(const char *path, const char *const argv[]) {
  std::stringstream cmdLine;
  cmdLine << path << " ";
  int i = 1;
  const char *arg = argv[i];
  while (arg != NULL) {
    cmdLine << arg << " ";
    arg = argv[++i];
  }
  std::string cmdStr = cmdLine.str();
  char* cmd_c = (char*) cmdStr.c_str();
  STARTUPINFO startupInfo;
  PROCESS_INFORMATION processInfo;
  memset(&startupInfo, 0, sizeof(startupInfo));
  memset(&processInfo, 0, sizeof(processInfo));
  return CreateProcess(path,
                       cmd_c,
                       NULL,
                       NULL,
                       false,
					   NORMAL_PRIORITY_CLASS,
                       NULL,  // _In_opt_    LPVOID                lpEnvironment,
                       NULL,  // _In_opt_    LPCTSTR               lpCurrentDirectory,
                       &startupInfo,  // _In_        LPSTARTUPINFO         lpStartupInfo,
                       &processInfo); // _Out_       LPPROCESS_INFORMATION lpProcessInformation
		
}

struct tm *glretrace_localtime(const time_t *timep, struct tm *result) {
  localtime_s(result, timep);
  return result;
}

int
glretrace_rand(unsigned int *seedp) {
  return rand();
}

void
glretrace_delay(unsigned int ms) {
  Sleep(ms);
}

}  // namespace glretrace
