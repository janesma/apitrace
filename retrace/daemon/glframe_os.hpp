// Copyright (C) Intel Corp.  2014.  All Rights Reserved.

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

#ifndef _GLFRAME_OS_H_
#define _GLFRAME_OS_H_

#include <mutex>
#include <condition_variable>
// #include <pthread.h>

#include "glframe_traits.hpp"

namespace glretrace {

typedef std::lock_guard<std::mutex> ScopedLock;

class Semaphore : NoCopy, NoAssign, NoMove {
 public:
  Semaphore() : m_count(0) {}

  void post() {
    std::lock_guard<std::mutex> lk(m_mutex);
    ++m_count;
    m_cv.notify_one();
  }
  void wait() {
    std::unique_lock<std::mutex> lk(m_mutex);
    while (!m_count)
      m_cv.wait(lk);
    --m_count;
  }
 private:
  unsigned m_count;
  unsigned m_max_count;
  std::mutex m_mutex;
  std::condition_variable m_cv;
};

int fork_execv(const char *path, char *const argv[]);

}  // namespace glretrace

#endif  // _GLFRAME_OS_H_
