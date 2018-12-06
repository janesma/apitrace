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

#include <GL/gl.h>
#include <GL/glext.h>

#include <string>
#include <vector>
#include "glframe_state.hpp"

namespace glretrace {
class Context;
class StdErrRedirect : public OutputPoller {
 public:
  StdErrRedirect();
  void poll(int current_program, StateTrack *cb);
  void pollBatch(SelectionId selectionCount,
                 ExperimentId experimentCount,
                 RenderId id,
                 OnFrameRetrace *cb);
  void flush();
  ~StdErrRedirect();
  void init();

 private:
  int out_pipe[2];
  std::vector<char> buf;
};

class NoRedirect : public OutputPoller {
 public:
  NoRedirect() {}
  void poll(int, StateTrack *) {}
  void pollBatch(SelectionId,
                 ExperimentId,
                 RenderId,
                 OnFrameRetrace *) {}
  void flush() {}
  ~NoRedirect() {}
  void init() {}
};

class WinShaders : public OutputPoller {
 public:
  WinShaders() {}
  void poll(int current_program, StateTrack *cb);
  void pollBatch(SelectionId,
                 ExperimentId,
                 RenderId,
                 OnFrameRetrace *) {}
  void flush() {}
  ~WinShaders() {}
  void init();
 private:
  std::string m_dump_dir;
  std::string m_dump_pattern;
};

class ShaderCallback : public OutputPoller {
 public:
  explicit ShaderCallback(StateTrack *cb) : m_cb(cb) {}
  void poll(int current_program, StateTrack *cb) {}
  void pollBatch(SelectionId selectionCount,
                 ExperimentId experimentCount,
                 RenderId id,
                 OnFrameRetrace *cb) {}
  void flush() {}
  ~ShaderCallback();
  void init();
  void _callback(GLenum source, GLenum type, GLuint id,
                 GLenum severity, GLsizei length,
                 const GLchar *message) const;
 private:
  StateTrack *m_cb;
  std::vector<Context *> m_known_contexts;
};

}  // namespace glretrace
