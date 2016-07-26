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
 **************************************************************************/

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <string>

#include "glframe_stderr.hpp"
#include "glframe_logger.hpp"

using glretrace::StdErrRedirect;

StdErrRedirect::StdErrRedirect() {
  pipe2(out_pipe, O_NONBLOCK);
  dup2(out_pipe[1], STDERR_FILENO);
  close(out_pipe[1]);
  buf.resize(1024);
}

std::string
StdErrRedirect::poll() {
  fflush(stdout);
  std::string ret;
  int bytes = read(out_pipe[0], buf.data(), buf.size() - 1);
  while (0 < bytes) {
    buf[bytes] = '\0';
    ret.append(buf.data());
    bytes = read(out_pipe[0], buf.data(), buf.size() - 1);
  }
  return ret;
}

StdErrRedirect::~StdErrRedirect() {
  close(out_pipe[2]);
}

void
StdErrRedirect::init() {
  setenv("INTEL_DEBUG", "vs,fs", 1);
  setenv("vblank_mode", "0", 1);
}
