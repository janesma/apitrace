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
using glretrace::MetricSeries;
using glretrace::NoRedirect;
using glretrace::OnFrameRetrace;
using glretrace::OutputPoller;
using glretrace::RenderId;
using glretrace::RenderOptions;
using glretrace::RenderSelection;
using glretrace::RenderTargetType;
using glretrace::SelectionId;
using glretrace::ShaderAssembly;
using glretrace::StateTrack;
using glretrace::StdErrRedirect;
using glretrace::WARN;
using image::Image;
using retrace::parser;
using trace::Call;
using glretrace::WinShaders;

extern retrace::Retracer retracer;

#ifdef WIN32
static WinShaders assemblyOutput;
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
FrameRetrace::openFile(const std::string &filename,
                       const std::vector<unsigned char> &md5,
                       uint64_t fileSize,
                       uint32_t framenumber,
                       OnFrameRetrace *callback) {
  assemblyOutput.init();
  retrace::debug = 0;
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
    GRLOGF(glretrace::DEBUG, "CALL: %s", call_stream.str().c_str());

    // we re-use shaders for shader editing features, even if the
    // source program has deleted them.  To support this, we never
    // delete shaders.
    if (strcmp(call->sig->name, "glDeleteShader") != 0) {
      retracer.retrace(*call);
      m_tracker.track(*call);
    }
    const bool frame_boundary = call->flags & trace::CALL_FLAG_END_FRAME;
    delete call;
    if (frame_boundary) {
      ++current_frame;
      callback->onFileOpening(false, false, current_frame * 100 / framenumber);
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
  callback->onFileOpening(false, true, 100);
}

int
FrameRetrace::getRenderCount() const {
  return m_renders.size();
}

void
FrameRetrace::retraceRenderTarget(ExperimentId experimentCount,
                                  const RenderSelection &selection,
                                  RenderTargetType type,
                                  RenderOptions options,
                                  OnFrameRetrace *callback) const {
  // reset to beginning of frame
  parser->setBookmark(frame_start.start);

  bool clear_once = true;

  // play up to the beginning of the render
  RenderId current_render_id(0);
  for (const auto &sequence : selection.series) {
    while (current_render_id < sequence.begin) {
      m_renders[current_render_id.index()]->retraceRenderTarget(m_tracker,
                                                                NORMAL_RENDER);
      ++current_render_id;
    }

    while (current_render_id < sequence.end) {
      if ((options & glretrace::CLEAR_BEFORE_RENDER) && clear_once) {
        GlFunctions::Clear(GL_COLOR_BUFFER_BIT);
        GlFunctions::Clear(GL_DEPTH_BUFFER_BIT);
        GlFunctions::Clear(GL_STENCIL_BUFFER_BIT);
        GlFunctions::Clear(GL_ACCUM_BUFFER_BIT);
        // ignore errors from unsupported clears
        GlFunctions::GetError();

        // don't clear the frame buffer before every sequence.  We
        // want to clear everything before the first selection.
        clear_once = false;
      }

      // play up to the end of the render
      m_renders[current_render_id.index()]->retraceRenderTarget(m_tracker,
                                                                type);
      ++current_render_id;
    }

    if (!(options & glretrace::STOP_AT_RENDER)) {
      // play to the end of the render target

      const RenderId last_render = lastRenderForRTRegion(current_render_id);
      while (current_render_id < last_render) {
        const int i = current_render_id.index();
        m_renders[i]->retraceRenderTarget(m_tracker,
                                          NORMAL_RENDER);
        ++current_render_id;
      }
    }
  }

  Image *i = glstate::getDrawBufferImage(0);
  if (!i) {
    GRLOGF(WARN, "Failed to obtain draw buffer image for render id: %d",
           current_render_id());
    if (callback)
      callback->onError("Failed to obtain draw buffer image");
  } else {
    std::stringstream png;
    i->writePNG(png);

    std::vector<unsigned char> d;
    const int bytes = png.str().size();
    d.resize(bytes);
    memcpy(d.data(), png.str().c_str(), bytes);
    if (callback)
      callback->onRenderTarget(selection.id, experimentCount, d);
  }

  // play to the rest of the frame
  while (current_render_id.index() < m_renders.size()) {
    m_renders[current_render_id.index()]->retraceRenderTarget(m_tracker,
                                                              NORMAL_RENDER);
    ++current_render_id;
  }
}

void
FrameRetrace::retraceShaderAssembly(const RenderSelection &selection,
                                    OnFrameRetrace *callback) {
  // reset to beginning of frame
  parser->setBookmark(frame_start.start);
  StateTrack tmp_tracker = m_tracker;

  RenderId current_render_id(0);
  for (const auto &sequence : selection.series) {
    // play up to the end of the render
    while (current_render_id < sequence.begin) {
      m_renders[current_render_id.index()]->retrace(&tmp_tracker);
      ++current_render_id;
    }
    while (current_render_id < sequence.end) {
      m_renders[current_render_id.index()]->retrace(&tmp_tracker);
      callback->onShaderAssembly(current_render_id,
                                 selection.id,
                                 tmp_tracker.currentVertexShader(),
                                 tmp_tracker.currentFragmentShader(),
                                 tmp_tracker.currentTessControlShader(),
                                 tmp_tracker.currentTessEvalShader(),
                                 tmp_tracker.currentGeomShader(),
                                 tmp_tracker.currentCompShader());
      ++current_render_id;
    }
  }
  // play to the rest of the frame
  while (current_render_id.index() < m_renders.size()) {
    m_renders[current_render_id.index()]->retrace(&tmp_tracker);
    ++current_render_id;
  }
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
      if (current_frame == framenumber) {
        delete call;
        break;
      }
    }
    delete call;
  }

  while ((call = p->scan_call())) {
    if (call->flags & trace::CALL_FLAG_RENDER) {
      ++render_count;
    }

    if (call->flags & trace::CALL_FLAG_END_FRAME) {
      delete call;
      break;
    }
    delete call;
  }
}

void
FrameRetrace::retraceMetrics(const std::vector<MetricId> &ids,
                             ExperimentId experimentCount,
                             OnFrameRetrace *callback) const {
  // retrace the frame once to warm up the gpu, ensuring that the gpu
  // is not throttled
  parser->setBookmark(frame_start.start);
  for (int i = 0; i < m_renders.size(); ++i) {
    m_renders[i]->retrace(m_tracker);
  }

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
      callback->onMetrics(metricData, experimentCount,
                          // this use case is not based on render
                          // selection
                          SelectionId(0));
      continue;
    }

    m_metrics->selectMetric(id);
    for (int i = 0; i < m_renders.size(); ++i) {
      m_metrics->begin(RenderId(i));
      m_renders[i]->retrace(m_tracker);
      m_metrics->end();
    }
    m_metrics->publish(experimentCount,
                       SelectionId(0),  // this use case is not based
                                        // on render selection
                       callback);
  }
}

void
FrameRetrace::retraceAllMetrics(const RenderSelection &selection,
                                ExperimentId experimentCount,
                                OnFrameRetrace *callback) const {
  // retrace the frame once to warm up the gpu, ensuring that the gpu
  // is not throttled
  parser->setBookmark(frame_start.start);
  for (int i = 0; i < m_renders.size(); ++i) {
    m_renders[i]->retrace(m_tracker);
  }

  for (int i = 0; i < m_metrics->groupCount(); ++i) {
    bool query_active = false;
    m_metrics->selectGroup(i);
    parser->setBookmark(frame_start.start);
    // iterate through the RenderSelection, and insert begin/end
    // around each RenderSeries
    auto currentRenderSequence = selection.series.begin();
    for (int i = 0; i < m_renders.size(); ++i) {
      if (currentRenderSequence != selection.series.end()) {
        if (RenderId(i) == currentRenderSequence->end) {
          m_metrics->end();
          query_active = false;
          ++currentRenderSequence;
        }
        if (currentRenderSequence != selection.series.end() &&
            (RenderId(i) == currentRenderSequence->begin)) {
          m_metrics->begin(RenderId(i));
          query_active = true;
        }
      }
      m_renders[i]->retrace(m_tracker);
    }
    if (query_active)
      m_metrics->end();
    m_metrics->publish(experimentCount, selection.id, callback);
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
                             const std::string &tessControl,
                             const std::string &tessEval,
                             const std::string &geom,
                             const std::string &comp,
                             OnFrameRetrace *callback) {
  GRLOGF(DEBUG, "%s\n%s", vs.c_str(), fs.c_str());
  std::string message;
  const bool result = m_renders[renderId.index()]->replaceShaders(&m_tracker,
                                                                  vs, fs,
                                                                  tessControl,
                                                                  tessEval,
                                                                  geom,
                                                                  comp,
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
