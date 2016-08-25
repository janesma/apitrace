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

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "glframe_thread.hpp"

using glretrace::Thread;
class TestThread : public Thread {
 public:
  TestThread() : Thread("gtest thread") {}
  ~TestThread() {}
  void Run() {
#ifdef WIN32
    Sleep(20);
#else
    usleep(20 * 1000);
#endif
  }
};

TEST(Thread, StartStop) {
  std::vector<Thread*> threads;
  for (int i = 0; i < 10; ++i) {
    threads.push_back(new TestThread());
  }
  for (auto i : threads) {
    i->Start();
  }
  for (auto i : threads) {
    i->Join();
  }
  for (auto i : threads) {
    delete i;
  }
}

