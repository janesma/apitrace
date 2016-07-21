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
  cmdLine << path;
  const char *i = argv[0];
  while (i != NULL) {
    cmdLine << i << " ";
  }
  return CreateProcess(NULL,
                       (char*) (cmdLine.str().c_str()),
                       NULL,
                       NULL,
                       false,
                       0,
                       NULL,  // _In_opt_    LPVOID                lpEnvironment,
                       NULL,  // _In_opt_    LPCTSTR               lpCurrentDirectory,
                       NULL,  // _In_        LPSTARTUPINFO         lpStartupInfo,
                       NULL); // _Out_       LPPROCESS_INFORMATION lpProcessInformation
		
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

void
shutdown() {
  shutdown(m_socket_fd, SHUT_RDWR);
}

}  // namespace glretrace
