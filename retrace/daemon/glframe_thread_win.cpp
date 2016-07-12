// Copyright (C) Intel Corp.  2016.  All Rights Reserved.

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

#include "glframe_thread.hpp"

#include <assert.h>
#include <string>

#include "glframe_logger.hpp"

using glretrace::Thread;
using glretrace::INFO;

Thread::Thread(const std::string &name) : m_name(name) {}

DWORD WINAPI start_thread(LPVOID ctx);
DWORD WINAPI start_thread(LPVOID ctx) {
  reinterpret_cast<Thread*>(ctx)->Run();
  return 0;
}

void
Thread::Start() {
  thread_handle = CreateThread( 
      NULL,                             // default security attributes
      0,                                // use default stack size  
      start_thread,                     // thread function name
      this,                             // argument to thread function 
      0,                                // use default creation flags 
      NULL);                            // returns the thread identifier
  
  GRLOGF(INFO, "thread started: %s", m_name.c_str());
}

void
Thread::Join() {
  GRLOGF(INFO, "joining thread: %s", m_name.c_str());
  WaitForSingleObject(thread_handle, INFINITE);
  CloseHandle(thread_handle);
  GRLOGF(INFO, "thread joined: %s", m_name.c_str());
}


