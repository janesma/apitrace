/**************************************************************************
 *
 * Copyright 2016 Intel Corporation
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

#include "glframe_loop.hpp"

#include <GLES2/gl2.h>

#include <sstream>
#include <string>
#include <vector>

#include "glframe_glhelper.hpp"
#include "glretrace.hpp"
#include "retrace.hpp"

using glretrace::FrameLoop;

using retrace::parser;
using trace::Call;

extern retrace::Retracer retracer;

FrameLoop::FrameLoop(const std::string filepath,
                     const std::string out_path,
                     int loop_count)
    : m_of(), m_out(NULL),
      m_current_frame(1),
      m_loop_count(loop_count) {
  if (out_path.size()) {
    m_of.open(out_path);
    m_out = new std::ostream(m_of.rdbuf());
  } else {
    m_out = new std::ostream(std::cout.rdbuf());
  }
  *m_out << filepath << std::endl;
  *m_out << "frame";
  for (int i = 0; i < loop_count; ++i)
    *m_out << "\t" << i;
  *m_out << std::endl;

  retrace::debug = 0;
  retracer.addCallbacks(glretrace::gl_callbacks);
  retracer.addCallbacks(glretrace::glx_callbacks);
  retracer.addCallbacks(glretrace::wgl_callbacks);
  retracer.addCallbacks(glretrace::cgl_callbacks);
  retracer.addCallbacks(glretrace::egl_callbacks);
  retrace::setUp();
  parser->open(filepath.c_str());
}

FrameLoop::~FrameLoop() {
  if (m_of.is_open())
    m_of.close();
  delete m_out;
}

void
FrameLoop::advanceToFrame(int f) {
  *m_out << std::endl << f;
  for (auto c : m_calls)
    delete c;
  m_calls.clear();

  trace::Call *call;
  while ((call = parser->parse_call()) && m_current_frame < f) {
    retracer.retrace(*call);
    const bool frame_boundary = call->flags & trace::CALL_FLAG_END_FRAME;
    delete call;
    if (frame_boundary) {
      ++m_current_frame;
      if (m_current_frame == f)
        break;
    }
  }

  while ((call = parser->parse_call())) {
    m_calls.push_back(call);
    retracer.retrace(*call);
    const bool frame_boundary = call->flags & trace::CALL_FLAG_END_FRAME;
    if (frame_boundary) {
      ++m_current_frame;
      break;
    }
  }
}
#ifdef _MSC_VER
#include "Windows.h"
inline unsigned int
get_ms_time() {
  LARGE_INTEGER frequency;
  ::QueryPerformanceFrequency(&frequency);

  LARGE_INTEGER start;
  ::QueryPerformanceCounter(&start);

  return start.QuadPart / (frequency.QuadPart / 1000);
}
#else
inline unsigned int
get_ms_time() {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t);
  unsigned int ms = t.tv_sec * 1000;
  ms += (t.tv_nsec / 1000000);
  return ms;
}
#endif

void
FrameLoop::loop() {
  // warm up with 5 frames
  // retrace count frames and output frame time
  for (int i = 0; i < 5; ++i) {
    for (auto c : m_calls) {
      retracer.retrace(*c);
    }
  }
  GlFunctions::Finish();
  unsigned int begin = get_ms_time();
  for (int i = 0; i < m_loop_count; ++i) {
    for (auto c : m_calls) {
      retracer.retrace(*c);
    }
    GlFunctions::Finish();
    const unsigned int end = get_ms_time();
    *m_out << "\t" << end - begin;
    begin = end;
  }
}

