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

#include "glframe_batch.hpp"
#include "glframe_glhelper.hpp"
#include "glframe_gpu_speed.hpp"
#include "glframe_logger.hpp"
#include "glframe_metrics.hpp"
#include "glframe_perf_enabled.hpp"
#include "glframe_retrace_context.hpp"
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
using glretrace::MesaBatch;
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
using glretrace::WinBatch;
using image::Image;
using retrace::parser;
using trace::Call;
using glretrace::WinShaders;

extern retrace::Retracer retracer;

#ifdef WIN32
static WinShaders assemblyOutput;
static WinBatch batchControl;
#else
static StdErrRedirect assemblyOutput;
static MesaBatch batchControl;
#endif

FrameRetrace::FrameRetrace()
    : m_tracker(&assemblyOutput),
      m_metrics(NULL) {
}

FrameRetrace::~FrameRetrace() {
  if (m_metrics)
    delete m_metrics;
  parser->close();
  retrace::cleanUp();
}

void
FrameRetrace::openFile(const std::string &filename,
                       const std::vector<unsigned char> &,
                       uint64_t,
                       uint32_t framenumber,
                       OnFrameRetrace *callback) {
  check_gpu_speed(callback);

  if (perf_enabled() == false) {
    std::stringstream msg;
    msg << "Performance counters not enabled.\n"
        "To enable counters, execute as root: "
        "`/sbin/sysctl dev.i915.perf_stream_paranoid=0`";
    callback->onError(RETRACE_FATAL, msg.str());
    return;
  }

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
  unsigned int current_frame = 0;
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
      callback->onFileOpening(false, false, current_frame);
      if (current_frame == framenumber)
        break;
    }
  }

  if (current_frame < framenumber) {
    std::stringstream msg;
    msg << "Trace contains " << current_frame <<
        " frames.  Please choose an earlier frame for analysis.";
    callback->onError(RETRACE_FATAL, msg.str());
    return;
  }

  // sends list of available metrics to ui
  m_metrics = new PerfMetrics(callback);
  parser->getBookmark(frame_start.start);

  // play through the frame, recording each context
  RenderId current_render(0);
  while (true) {
    auto c = new RetraceContext(current_render, parser, &retracer, &m_tracker);
    current_render = RenderId(current_render.index() + c->getRenderCount());
    m_contexts.push_back(c);
    if (c->endsFrame()) {
      break;
    }
  }

  callback->onFileOpening(false, true, current_frame);
}

int
FrameRetrace::getRenderCount() const {
  int count = 0;
  for (auto i : m_contexts) {
    count += i->getRenderCount();
  }
  return count;
}

void
FrameRetrace::retraceRenderTarget(ExperimentId experimentCount,
                                  const RenderSelection &selection,
                                  RenderTargetType type,
                                  RenderOptions options,
                                  OnFrameRetrace *callback) const {
  // reset to beginning of frame
  parser->setBookmark(frame_start.start);
  for (auto i : m_contexts)
    i->retraceRenderTarget(experimentCount, selection, type, options,
                           m_tracker, callback);
}

void
FrameRetrace::retraceShaderAssembly(const RenderSelection &selection,
                                    ExperimentId experimentCount,
                                    OnFrameRetrace *callback) {
  // reset to beginning of frame
  parser->setBookmark(frame_start.start);
  StateTrack tmp_tracker = m_tracker;

  for (auto i : m_contexts)
    i->retraceShaderAssembly(selection, experimentCount, &m_tracker, callback);
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
    if (RetraceRender::isRender(*call)) {
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

  for (auto i : m_contexts)
    i->retraceMetrics(NULL, m_tracker);

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
    for (auto i : m_contexts)
      i->retraceMetrics(m_metrics, m_tracker);
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
  for (auto i : m_contexts)
    i->retraceMetrics(NULL, m_tracker);

  for (int i = 0; i < m_metrics->groupCount(); ++i) {
    m_metrics->selectGroup(i);
    parser->setBookmark(frame_start.start);

    for (auto i : m_contexts)
      i->retraceAllMetrics(selection, m_metrics, m_tracker);
  }
  m_metrics->publish(experimentCount,
                     selection.id,
                     callback);
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
  for (auto i : m_contexts)
    if (i->replaceShaders(renderId, experimentCount, &m_tracker,
                          vs, fs, tessControl, tessEval,
                          geom, comp, callback))
      return;
}

void
FrameRetrace::disableDraw(const RenderSelection &selection, bool disable) {
  for (auto sequence : selection.series) {
    for (auto render = sequence.begin; render < sequence.end; ++render) {
      for (auto context : m_contexts)
        context->disableDraw(render, disable);
    }
  }
}

void
FrameRetrace::simpleShader(const RenderSelection &selection, bool simple) {
  for (auto sequence : selection.series) {
    for (auto render = sequence.begin; render < sequence.end; ++render) {
      for (auto context : m_contexts)
        context->simpleShader(render, simple);
    }
  }
}

void
FrameRetrace::retraceApi(const RenderSelection &selection,
                         OnFrameRetrace *callback) {
  // reset to beginning of frame
  parser->setBookmark(frame_start.start);
  for (auto i : m_contexts)
    i->retraceApi(selection, callback);
}

void
FrameRetrace::retraceBatch(const RenderSelection &selection,
                           ExperimentId experimentCount,
                           OnFrameRetrace *callback) {
  // reset to beginning of frame
  parser->setBookmark(frame_start.start);
  if (!batchControl.batchSupported())
    return;
  for (auto i : m_contexts)
    i->retraceBatch(selection, experimentCount, m_tracker, &batchControl,
                    &assemblyOutput, callback);
  batchControl.batchOff();
}
