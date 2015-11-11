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

#include "glframe_retrace.hpp"

#include <GLES2/gl2.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sstream>
#include <string>
#include <vector>

#include "glframe_glhelper.hpp"
#include "glframe_metrics.hpp"
#include "glretrace.hpp"
#include "glws.hpp"
#include "trace_dump.hpp"
#include "trace_parser.hpp"

#include "glstate_images.hpp"
#include "glstate.hpp"
#include "glstate_internal.hpp"

using glretrace::ExperimentId;
using glretrace::FrameRetrace;
using glretrace::FrameState;
using glretrace::GlFunctions;
using glretrace::MetricId;
using glretrace::OnFrameRetrace;
using glretrace::RenderId;
using glretrace::StateTrack;
using trace::Call;
using retrace::parser;
using image::Image;
using retrace::parser;
using trace::Call;
extern retrace::Retracer retracer;


class StdErrRedirect {
 public:
  StdErrRedirect() {
    pipe2(out_pipe, O_NONBLOCK);
    dup2(out_pipe[1], STDERR_FILENO);
    close(out_pipe[1]);
    buf.resize(1024);
  }
  std::string poll() {
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
 private:
  int out_pipe[2];
  std::vector<char> buf;
};

static StdErrRedirect assemblyOutput;

class PlayAndCleanUpCall {
 public:
  PlayAndCleanUpCall(Call *c, StateTrack *tracker, bool dump = true)
      : call(c) {
    assemblyOutput.poll();

    retracer.retrace(*call);

    if (tracker) {
      tracker->track(*c);
      tracker->parse(assemblyOutput.poll());
    }
  }
  ~PlayAndCleanUpCall() {
    delete call;
  }
 private:
  Call *call;
};

FrameRetrace::FrameRetrace() {}
FrameRetrace::~FrameRetrace() {
  for (auto i : metrics)
    delete(i.second);
  metrics.clear();
  parser->close();
  retrace::cleanUp();
}

int currentRenderBuffer() {
  glstate::Context context;
  const GLenum framebuffer_binding = context.ES ?
                                     GL_FRAMEBUFFER_BINDING :
                                     GL_DRAW_FRAMEBUFFER_BINDING;
  GLint draw_framebuffer = 0;
  GlFunctions::GetIntegerv(framebuffer_binding, &draw_framebuffer);
  return draw_framebuffer;
}

void
FrameRetrace::openFile(const std::string &filename, uint32_t framenumber,
                       OnFrameRetrace *callback) {
  setenv("INTEL_DEBUG", "vs,fs", 1);
  setenv("vblank_mode", "0", 1);

  retracer.addCallbacks(glretrace::gl_callbacks);
  retracer.addCallbacks(glretrace::glx_callbacks);
  retracer.addCallbacks(glretrace::wgl_callbacks);
  retracer.addCallbacks(glretrace::cgl_callbacks);
  retracer.addCallbacks(glretrace::egl_callbacks);

  retrace::setUp();
  parser->open(filename.c_str());

  // play up to the requested frame
  trace::Call *call;
  int current_frame = 0;
  while ((call = parser->parse_call()) && current_frame < framenumber) {
    PlayAndCleanUpCall c(call, &tracker, false);
    handleContext(call, callback);
    if (call->flags & trace::CALL_FLAG_END_FRAME) {
      ++current_frame;
      callback->onFileOpening(false, current_frame * 100 / framenumber);
      if (current_frame == framenumber)
        break;
    }
  }

  parser->getBookmark(frame_start.start);
  renders.push_back(RenderBookmark());
  parser->getBookmark(renders.back().start);

  int current_render_buffer = currentRenderBuffer();
  // play through the frame, recording renders and call counts
  while ((call = parser->parse_call())) {
    PlayAndCleanUpCall c(call, &tracker, false);
    assert(!changesContext(call));
    ++frame_start.numberOfCalls;
    ++(renders.back().numberOfCalls);

    const int new_render_buffer = currentRenderBuffer();
    if (new_render_buffer != current_render_buffer) {
      if (renders.size() > 1)  // don't record the very first draw as
                               // ending a render target
        render_target_regions.push_back(RenderId(renders.size() - 1));
      current_render_buffer = new_render_buffer;
    }

    if (current_frame > framenumber) {
      return;
    }
    if (call->flags & trace::CALL_FLAG_RENDER) {
      renders.push_back(RenderBookmark());
      parser->getBookmark(renders.back().start);
    }

    if (call->flags & trace::CALL_FLAG_END_FRAME) {
      renders.pop_back();
      break;
    }
  }
  callback->onFileOpening(true, 100);
}

void
FrameRetrace::handleContext(Call *call, OnFrameRetrace *cb) {
  if (!changesContext(call))
    return;

  initial_frame_context = getContext(call);
  if (metrics.find(initial_frame_context) == metrics.end())
    metrics[initial_frame_context] = new PerfMetrics(cb);
}

int
FrameRetrace::getRenderCount() const {
  return renders.size();
}


void
FrameRetrace::retraceRenderTarget(RenderId renderId,
                                  int render_target_number,
                                  RenderTargetType type,
                                  RenderOptions options,
                                  OnFrameRetrace *callback) const {
  trace::Call *call;
  const RenderBookmark &render = renders[renderId.index()];
  // reset to beginning of frame
  parser->setBookmark(frame_start.start);
  int played_calls = 0;

  // play up to the beginning of the render
  int current_call = frame_start.start.next_call_no;
  while ((current_call < render.start.next_call_no) &&
         (call = parser->parse_call())) {
    ++played_calls;
    ++current_call;
    PlayAndCleanUpCall c(call, NULL);
  }

  if (options & glretrace::CLEAR_BEFORE_RENDER)
    GlFunctions::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                       GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  // play up to the end of the render
  for (int render_calls = 0; render_calls < render.numberOfCalls;
       ++render_calls) {
    call = parser->parse_call();
    if (!call)
      break;

    ++played_calls;
    ++current_call;
    PlayAndCleanUpCall c(call, NULL);
  }

  if (!(options & glretrace::STOP_AT_RENDER)) {
    // play to the end of the render target

    const RenderId last_render = lastRenderForRTRegion(renderId);
    const RenderBookmark &last_render_bm = renders[last_render.index()];
    const int next_render_call = last_render_bm.start.next_call_no +
                                 last_render_bm.numberOfCalls;
    while ((current_call < next_render_call) &&
           (call = parser->parse_call())) {
      ++played_calls;
      ++current_call;
      PlayAndCleanUpCall c(call, NULL);
    }
  }

  Image *i = glstate::getDrawBufferImage(0);
  std::stringstream png;
  i->writePNG(png);

  std::vector<unsigned char> d;
  const int bytes = png.str().size();
  d.resize(bytes);
  memcpy(d.data(), png.str().c_str(), bytes);

  if (callback)
    callback->onRenderTarget(renderId, type, d);
}


void
FrameRetrace::retraceShaderAssembly(RenderId renderId,
                                    OnFrameRetrace *callback) {
  const RenderBookmark &render = renders[renderId.index()];
  trace::Call *call;
  StateTrack tmp_tracker = tracker;
  tmp_tracker.reset();

  // reset to beginning of frame
  parser->setBookmark(frame_start.start);
  int played_calls = 0;

  // play up to the beginning of the render
  int current_call = frame_start.start.next_call_no;
  while ((current_call < render.start.next_call_no) &&
         (call = parser->parse_call())) {
    ++played_calls;
    ++current_call;
    PlayAndCleanUpCall c(call, &tmp_tracker, true);
  }

  // play up to the end of the render
  for (int render_calls = 0; render_calls < render.numberOfCalls;
       ++render_calls) {
    call = parser->parse_call();
    if (!call)
      break;

    ++played_calls;
    PlayAndCleanUpCall c(call, &tmp_tracker, true);
  }

  callback->onShaderAssembly(renderId,
                             tmp_tracker.currentVertexShader(),
                             tmp_tracker.currentVertexIr(),
                             tmp_tracker.currentVertexVec4(),
                             tmp_tracker.currentFragmentShader(),
                             tmp_tracker.currentFragmentIr(),
                             tmp_tracker.currentFragmentSimd8(),
                             tmp_tracker.currentFragmentSimd16(),
                             tmp_tracker.currentFragmentSSA(),
                             tmp_tracker.currentFragmentNIR());
}

FrameState::FrameState(const std::string &filename,
                       int framenumber) : render_count(0) {
  trace::Parser *p = reinterpret_cast<trace::Parser*>(parser);
  p->open(filename.c_str());
  trace::Call *call;
  int current_frame = 0;
  while ((call = p->scan_call()) && current_frame < framenumber) {
    if (call->flags & trace::CALL_FLAG_END_FRAME) {
      ++current_frame;
      if (current_frame == framenumber)
        break;
    }
  }

  while ((call = p->scan_call())) {
    if (call->flags & trace::CALL_FLAG_RENDER) {
      ++render_count;
    }

    if (call->flags & trace::CALL_FLAG_END_FRAME) {
      break;
    }
  }
}

void
FrameRetrace::retraceMetrics(const std::vector<MetricId> &ids,
                             ExperimentId experimentCount,
                             OnFrameRetrace *callback) const {
  auto i = metrics.find(initial_frame_context);
  assert(i != metrics.end());
  PerfMetrics *m = i->second;
  const int render_count = getRenderCount();
  for (const auto &id : ids) {
    const MetricId nullMetric(0);
    if (id == nullMetric) {
      // no metrics selected
      MetricSeries metricData;
      metricData.metric = nullMetric;
      for (int i = 0; i < render_count; ++i)
        metricData.data.push_back(1.0);
      callback->onMetrics(metricData, experimentCount);
      continue;
    }
    m->selectMetric(id);
    parser->setBookmark(frame_start.start);

    int current_render = 0;
    m->begin(RenderId(current_render));
    trace::Call *call;
    while (current_render < render_count) {
      call = parser->parse_call();
      assert(!changesContext(call));
      PlayAndCleanUpCall c(call, NULL, false);
      if (call->flags & trace::CALL_FLAG_RENDER) {
        m->end();
        ++current_render;
        if (current_render == render_count)
          break;
        m->begin(RenderId(current_render));
      }
      assert(!(call->flags & trace::CALL_FLAG_END_FRAME));
    }
    m->publish(experimentCount, callback);
  }
}


bool
FrameRetrace::changesContext(trace::Call *call) const {
  if (strncmp(call->name(), "glXMakeCurrent", strlen("glXMakeCurrent")) == 0)
    return true;
  return false;
}

uint64_t
FrameRetrace::getContext(trace::Call *call) {
  assert(changesContext(call));
  return call->arg(2).toUIntPtr();
}

RenderId
FrameRetrace::lastRenderForRTRegion(RenderId render) const {
  for (auto rt_render : render_target_regions)
    if (rt_render > render)
      return rt_render;
  return render_target_regions.back();
}
