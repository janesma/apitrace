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
#include <stdlib.h>

#include <string>

#include "glframe_logger.hpp"
#include "glframe_os.hpp"

using glretrace::Logger;
using glretrace::ERR;
using glretrace::WARN;
using glretrace::glretrace_rand;


class LogTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
  Logger::Create();
  Logger::Begin();
  }
  virtual void TearDown() {
    Logger::Destroy();
  }
};

TEST_F(LogTest, ReadWrite) {
  std::string s("This is a test message");
  GRLOG(ERR, s.c_str());
  Logger::Flush();
  std::string m;
  Logger::GetLog(&m);
  size_t p = m.find(s);
  EXPECT_NE(p, std::string::npos);
}

TEST_F(LogTest, RepeatLog) {
  std::string m;
  size_t p;
  unsigned int seed = 1;
  // make a long string
  for (int j = 0; j < 100; ++j) {
    std::stringstream ss;
    for (int i = 0; i < 10; ++i)
      ss << glretrace_rand(&seed);
    GRLOG(WARN, ss.str().c_str());
    Logger::Flush();
    Logger::GetLog(&m);
    p = m.find(ss.str());
    EXPECT_NE(p, std::string::npos);

    m.clear();
    Logger::GetLog(&m);
    EXPECT_EQ(m.size(), 0);
  }
}
