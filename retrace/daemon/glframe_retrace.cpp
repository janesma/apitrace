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

#include <sstream>
#include <string>
#include <vector>

#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"
#include "glframe_metrics.hpp"
#include "glframe_retrace_render.hpp"
#include "glframe_stderr.hpp"
#include "glretrace.hpp"
// #include "glws.hpp"
// #include "trace_dump.hpp"
// #include "trace_parser.hpp"

// #include "glstate_images.hpp"
#include "glstate.hpp"
#include "glstate_internal.hpp"
#include "trace_dump.hpp"

using glretrace::ExperimentId;
using glretrace::FrameRetrace;
using glretrace::FrameState;
using glretrace::GlFunctions;
using glretrace::MetricId;
using glretrace::OnFrameRetrace;
using glretrace::OutputPoller;
using glretrace::RenderId;
using glretrace::StateTrack;
using glretrace::WARN;
using glretrace::NoRedirect;
using glretrace::StdErrRedirect;
using image::Image;
using retrace::parser;
using trace::Call;
using glretrace::RenderTargetType;
using glretrace::RenderOptions;

extern retrace::Retracer retracer;

#ifdef WIN32
static NoRedirect assemblyOutput;
#else
static StdErrRedirect assemblyOutput;
#endif

FrameRetrace::FrameRetrace()
    : m_tracker(&assemblyOutput) {
}

FrameRetrace::~FrameRetrace() {
  delete m_metrics;
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
  assemblyOutput.init();
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
    std::stringstream call_stream;
    trace::dump(*call, call_stream,
                trace::DUMP_FLAG_NO_COLOR);
    GRLOGF(glretrace::INFO, "CALL: %s", call_stream.str().c_str());
    retracer.retrace(*call);
    m_tracker.track(*call);
    const bool frame_boundary = call->flags & trace::CALL_FLAG_END_FRAME;
    delete call;
    if (frame_boundary) {
      ++current_frame;
      callback->onFileOpening(false, current_frame * 100 / framenumber);
      if (current_frame == framenumber)
        break;
    }
  }

  m_metrics = new PerfMetrics(callback);
  parser->getBookmark(frame_start.start);
  int current_render_buffer = currentRenderBuffer();

  // play through the frame, recording renders and call counts
  while (true) {
    auto r = new RetraceRender(parser, &retracer, &m_tracker);
    if (r->endsFrame()) {
      delete r;
      break;
    }
    m_renders.push_back(r);

    const int new_render_buffer = currentRenderBuffer();
    if (new_render_buffer != current_render_buffer) {
      if (m_renders.size() > 1)  // don't record the very first draw as
                               // ending a render target
        render_target_regions.push_back(RenderId(m_renders.size() - 1));
      current_render_buffer = new_render_buffer;
    }
  }

  // record the final render as ending a render target region
  render_target_regions.push_back(RenderId(m_renders.size() - 1));
  callback->onFileOpening(true, 100);
}

int
FrameRetrace::getRenderCount() const {
  return m_renders.size();
}

void
FrameRetrace::retraceRenderTarget(RenderId renderId,
                                  int render_target_number,
                                  RenderTargetType type,
                                  RenderOptions options,
                                  OnFrameRetrace *callback) const {
  // reset to beginning of frame
  parser->setBookmark(frame_start.start);

  // play up to the beginning of the render
  for (int i = 0; i < renderId.index(); ++i)
    m_renders[i]->retraceRenderTarget(NORMAL_RENDER);

  if (options & glretrace::CLEAR_BEFORE_RENDER)
    GlFunctions::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                       GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  // play up to the end of the render
  m_renders[renderId.index()]->retraceRenderTarget(type);

  if (!(options & glretrace::STOP_AT_RENDER)) {
    // play to the end of the render target

    const RenderId last_render = lastRenderForRTRegion(renderId);
    for (int i = renderId.index() + 1; i <= last_render.index(); ++i)
      m_renders[i]->retraceRenderTarget(NORMAL_RENDER);
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
  // reset to beginning of frame
  parser->setBookmark(frame_start.start);
  StateTrack tmp_tracker = m_tracker;
  tmp_tracker.reset();

  // play up to the end of the render
  for (int i = 0; i <= renderId.index(); ++i)
    m_renders[i]->retrace(&tmp_tracker);

  callback->onShaderAssembly(renderId,
                             tmp_tracker.currentVertexShader(),
                             tmp_tracker.currentVertexIr(),
                             tmp_tracker.currentVertexNIR(),
                             tmp_tracker.currentVertexSSA(),
                             tmp_tracker.currentVertexVec4(),
                             tmp_tracker.currentFragmentShader(),
                             tmp_tracker.currentFragmentIr(),
                             tmp_tracker.currentFragmentSimd8(),
                             tmp_tracker.currentFragmentSimd16(),
                             tmp_tracker.currentFragmentSSA(),
                             tmp_tracker.currentFragmentNIR(),
                             tmp_tracker.currentTessControlShader(),
                             tmp_tracker.currentTessEvalShader());
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
  const int render_count = getRenderCount();
  for (const auto &id : ids) {
    // reset to beginning of frame
    parser->setBookmark(frame_start.start);
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

    m_metrics->selectMetric(id);
    for (int i = 0; i < m_renders.size(); ++i) {
      m_metrics->begin(RenderId(i));
      m_renders[i]->retrace();
      m_metrics->end();
    }
    m_metrics->publish(experimentCount, callback);
  }
}



RenderId
FrameRetrace::lastRenderForRTRegion(RenderId render) const {
  for (auto rt_render : render_target_regions)
    if (rt_render > render)
      return rt_render;
  return render_target_regions.back();
}

void
FrameRetrace::replaceShaders(RenderId renderId,
                             ExperimentId experimentCount,
                             const std::string &vs,
                             const std::string &fs,
                             OnFrameRetrace *callback) {
  GRLOGF(DEBUG, "%s\n%s", vs.c_str(), fs.c_str());
  std::string message;
  const bool result = m_renders[renderId.index()]->replaceShaders(&m_tracker,
                                                                  vs, fs,
                                                                  &message);
  if (!result)
    GRLOGF(WARN, "compile failed: %s", message.c_str());
  callback->onShaderCompile(renderId, experimentCount,
                            result, message);
}

void
FrameRetrace::retraceApi(RenderId renderId,
                         OnFrameRetrace *callback) {
  if (renderId.index() == RenderId(-1).index())
    return m_tracker.onApi(callback);
  m_renders[renderId.index()]->onApi(renderId, callback);
}
