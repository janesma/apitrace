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

#include "glframe_thread.hpp"

#include <assert.h>
#include <string>

using glretrace::Thread;

Thread::Thread(const std::string &name) : m_name(name) {}

void *start_thread(void*ctx);
void *start_thread(void*ctx) {
  reinterpret_cast<Thread*>(ctx)->Run();
  return NULL;
}

void
Thread::start_routine(Thread *context) {
  context->Run();
}

void
Thread::Start() {
  m_thread = std::thread(start_routine, this);
  // GFLOGF("thread started: %s", m_name.c_str());
}

void
Thread::Join() {
  // GFLOGF("joining thread: %s", m_name.c_str());
  assert(m_thread.joinable());
  m_thread.join();
  // GFLOGF("thread joined: %s", m_name.c_str());
}


