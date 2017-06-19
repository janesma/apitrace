/**************************************************************************
 *
 * Copyright 2017 Intel Corporation
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

#include "glframe_retrace_context.hpp"

#include <sstream>
#include <string>
#include <vector>

#include "trace_model.hpp"
#include "glframe_batch.hpp"
#include "glframe_glhelper.hpp"
#include "glretrace.hpp"
#include "glframe_logger.hpp"
#include "glframe_metrics.hpp"
#include "glframe_retrace_render.hpp"
#include "glframe_uniforms.hpp"
#include "glstate.hpp"

using glretrace::BatchControl;
using glretrace::ExperimentId;
using glretrace::OutputPoller;
using glretrace::OnFrameRetrace;
using glretrace::PerfMetrics;
using glretrace::RenderId;
using glretrace::RenderOptions;
using glretrace::RenderSelection;
using glretrace::RenderTargetType;
using glretrace::RetraceContext;
using glretrace::StateTrack;
using glretrace::WARN;
using image::Image;

RetraceContext::RetraceContext(RenderId current_render,
                               trace::AbstractParser *parser,
                               retrace::Retracer *retracer,
                               StateTrack *tracker)
    : m_parser(parser), m_retracer(retracer),
      m_context_switch(NULL), m_ends_frame(false) {
  m_parser->getBookmark(m_start_bookmark);
  trace::Call *call = parser->parse_call();
  if (RetraceRender::changesContext(*call))
    m_context_switch = call;
  else
    delete call;

  m_parser->setBookmark(m_start_bookmark);

  int current_render_buffer = RetraceRender::currentRenderBuffer();
  // play through the frame, generating renders
  while (true) {
    auto r = new RetraceRender(parser, retracer, tracker);
    if (r->endsFrame()) {
      delete r;
      m_ends_frame = true;
      break;
    }
    m_renders[current_render] = r;
    ++current_render;

    // peek ahead to see if next call is in the following context
    m_parser->getBookmark(m_end_bookmark);
    call = parser->parse_call();
    m_parser->setBookmark(m_end_bookmark);
    if (RetraceRender::changesContext(*call)) {
      m_parser->setBookmark(m_end_bookmark);
      delete call;
      break;
    }

    const int new_render_buffer = RetraceRender::currentRenderBuffer();

    // consider refactoring context to be a container of rt regions
    if (new_render_buffer != current_render_buffer) {
      if (m_renders.size() > 1)  // don't record the very first draw as
                               // ending a render target
        end_render_target_regions.push_back(RenderId(m_renders.size() - 1));
      current_render_buffer = new_render_buffer;
    }
  }
}

int
RetraceContext::getRenderCount() const {
  return m_renders.size();
}

bool isFirstRender(RenderId id, const RenderSelection &s) {
  return id == s.series.front().begin;
}

RenderId lastRender(const RenderSelection &s) {
  return RenderId(s.series.back().end() - 1);
}


bool isSelected(RenderId id, const RenderSelection &s) {
  for (auto seq : s.series) {
    if (id >= seq.end)
      continue;
    if (id < seq.begin)
      return false;
    return true;
  }
  return false;
}

RenderId
RetraceContext::lastRenderForRTRegion(RenderId render) const {
  for (auto rt_render : end_render_target_regions)
    if (rt_render > render)
      return RenderId(rt_render() - 1);
  // last render for the context is the final render for the last
  // region.
  return m_renders.rbegin()->first;
}

void
RetraceContext::retraceRenderTarget(ExperimentId experimentCount,
                                    const RenderSelection &selection,
                                    RenderTargetType type,
                                    RenderOptions options,
                                    const StateTrack &tracker,
                                    OnFrameRetrace *callback) const {
  auto current_render = m_renders.begin();
  // play up to the beginning of the first render
  while (current_render->first < selection.series.front().begin) {
    current_render->second->retraceRenderTarget(tracker, NORMAL_RENDER);
    ++current_render;
    if (current_render == m_renders.end())
      // played through the context
      return;
  }

  // TODO(majanes) possibly should be clearing on the first selected
  // render of the last selected framebuffer
  if (selection.series.front().begin == current_render->first) {
    // this is the first render of the selection
    if ((options & glretrace::CLEAR_BEFORE_RENDER)) {
      GlFunctions::Clear(GL_COLOR_BUFFER_BIT);
      GlFunctions::Clear(GL_DEPTH_BUFFER_BIT);
      GlFunctions::Clear(GL_STENCIL_BUFFER_BIT);
      GlFunctions::Clear(GL_ACCUM_BUFFER_BIT);
      // ignore errors from unsupported clears
      GlFunctions::GetError();
    }
  }

  // play through the selected renders
  const RenderId last_render = lastRender(selection);
  while (current_render->first < selection.series.back().end) {
    RenderTargetType current_type = type;
    if (!isSelected(current_render->first, selection))
      // unselected renders don't get highlighting
      type = NORMAL_RENDER;
    const bool is_last_render = (current_render->first == last_render);
    current_render->second->retraceRenderTarget(tracker,
                                                current_type);
    ++current_render;

    if (is_last_render)
      // reached end of selection
      break;
    if (current_render == m_renders.end())
      // played through the context
      break;
  }

  if ((!(options & glretrace::STOP_AT_RENDER)) &&
      (current_render != m_renders.end())) {
    // play through to the end of the currently attached framebuffer
    RenderId last_render_in_rt_region =
        lastRenderForRTRegion(last_render);
    while (current_render->first <= last_render_in_rt_region) {
      current_render->second->retraceRenderTarget(tracker,
                                                  NORMAL_RENDER);
      ++current_render;
      if (current_render == m_renders.end())
        // played through the context
        break;
    }
  }

  // report an image if the last selected render is in this context
  const bool contains_last_render =
      ((last_render >= m_renders.begin()->first) &&
       (last_render <= m_renders.rbegin()->first));
  if (contains_last_render) {
    const int image_count = glstate::getDrawBufferImageCount();
    for (int rt_num = 0; rt_num < image_count; ++rt_num) {
      Image *i = glstate::getDrawBufferImage(rt_num);
      if (!i) {
        GRLOG(WARN, "Failed to obtain draw buffer image for render id");
        if (callback)
          callback->onError(RETRACE_WARN, "Failed to obtain draw buffer image");
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
    }
  }

  // play to the rest of the context
  while (current_render != m_renders.end()) {
    current_render->second->retraceRenderTarget(tracker,
                                                NORMAL_RENDER);
    ++current_render;
  }
}

void
RetraceContext::retraceMetrics(PerfMetrics *perf,
                               const StateTrack &tracker) const {
  // NULL perf is passed for a warm-up round
  if (m_context_switch)
    m_retracer->retrace(*m_context_switch);
  if (perf)
    perf->beginContext();
  for (auto i : m_renders) {
    if (perf)
      perf->begin(i.first);
    i.second->retrace(tracker);
    if (perf)
      perf->end();
  }
  if (perf)
    perf->endContext();
}


class CleanPerf {
 public:
  CleanPerf(glretrace::PerfMetrics *perf,
            bool *active) : m_perf(perf),
                            m_active(active) {}
  ~CleanPerf() {
    if (*m_active) {
      m_perf->end();
    }
    m_perf->endContext();
  }
 private:
  glretrace::PerfMetrics *m_perf;
  bool *m_active;
};

void
RetraceContext::retraceAllMetrics(const RenderSelection &selection,
                                  PerfMetrics *perf,
                                  const StateTrack &tracker) const {
  if (m_context_switch)
    m_retracer->retrace(*m_context_switch);
  perf->beginContext();

  // iterate through the RenderSelection, and insert begin/end
  // around each RenderSeries
  // auto currentRenderSequence = selection.series.begin();
  auto current_render = m_renders.begin();
  bool metrics_active = false;
  CleanPerf cleanup(perf, &metrics_active);
  if (isSelected(current_render->first, selection)) {
    // continues a selection from the previous context
    perf->begin(current_render->first);
    metrics_active = true;
  }

  for (auto sequence : selection.series) {
    if (current_render->first >= sequence.end)
      // this sequence ended before our context begins
      continue;

    while (current_render->first < sequence.begin) {
      current_render->second->retrace(tracker);
      ++current_render;
      if (current_render == m_renders.end())
        return;
    }

    if ((current_render->first == sequence.begin) && (!metrics_active)) {
      perf->begin(current_render->first);
      metrics_active = true;
    }

    while (current_render->first < sequence.end) {
      current_render->second->retrace(tracker);
      ++current_render;
      if (current_render == m_renders.end())
        return;
    }

    if (sequence.end == current_render->first) {
      assert(metrics_active);
      metrics_active = false;
      perf->end();
    }
  }

  // CleanPerf destructor will end context
}

bool
RetraceContext::endsFrame() const {
  return m_ends_frame;
}

bool
RetraceContext::replaceShaders(RenderId renderId,
                               ExperimentId experimentCount,
                               StateTrack *tracker,
                               const std::string &vs,
                               const std::string &fs,
                               const std::string &tessControl,
                               const std::string &tessEval,
                               const std::string &geom,
                               const std::string &comp,
                               OnFrameRetrace *callback) {
  auto r = m_renders.find(renderId);
  if (r == m_renders.end())
    return true;
  std::string message;
  const bool result = r->second->replaceShaders(tracker,
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
  return result;
}

void
RetraceContext::disableDraw(RenderId render, bool disable) {
  auto render_iterator = m_renders.find(render);
  if (render_iterator == m_renders.end())
    return;
  render_iterator->second->disableDraw(disable);
}

void
RetraceContext::simpleShader(RenderId render, bool simple) {
  auto render_iterator = m_renders.find(render);
  if (render_iterator == m_renders.end())
    return;
  render_iterator->second->simpleShader(simple);
}

void
RetraceContext::retraceApi(const RenderSelection &selection,
                           OnFrameRetrace *callback) {
  if (selection.series.empty()) {
    // empty selection: display the full api log
    for (auto r : m_renders)
      r.second->onApi(selection.id, r.first, callback);
    return;
  }
  for (auto sequence : selection.series) {
    auto currentRender = sequence.begin;
    while (currentRender < sequence.end) {
      auto r = m_renders.find(currentRender);
      if (r != m_renders.end())
        r->second->onApi(selection.id,
                         currentRender,
                         callback);
      ++currentRender;
    }
  }
}


void
RetraceContext::retraceShaderAssembly(const RenderSelection &selection,
                                      ExperimentId experimentCount,
                                      StateTrack *tracker,
                                      OnFrameRetrace *callback) {
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_start_bookmark.offset);

  for (auto r : m_renders) {
    r.second->retrace(tracker);
    if (isSelected(r.first, selection)) {
      callback->onShaderAssembly(r.first,
                                 selection.id,
                                 experimentCount,
                                 tracker->currentVertexShader(),
                                 tracker->currentFragmentShader(),
                                 tracker->currentTessControlShader(),
                                 tracker->currentTessEvalShader(),
                                 tracker->currentGeomShader(),
                                 tracker->currentCompShader());
    }
  }
}

void
RetraceContext::retraceBatch(const RenderSelection &selection,
                             ExperimentId experimentCount,
                             const StateTrack &tracker,
                             BatchControl *control,
                             OutputPoller *poller,
                             OnFrameRetrace *callback) {
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_start_bookmark.offset);

  for (auto r : m_renders) {
    if (isSelected(r.first, selection)) {
      GlFunctions::Finish();
      control->batchOn();
    } else {
      control->batchOff();
    }
    r.second->retrace(tracker);
    if (isSelected(r.first, selection)) {
      GlFunctions::Finish();
      poller->pollBatch(selection.id, experimentCount, r.first, callback);
    }
  }
  control->batchOff();
}

void
RetraceContext::retraceUniform(const RenderSelection &selection,
                               ExperimentId experimentCount,
                               const StateTrack &tracker,
                               OnFrameRetrace *callback) {
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_start_bookmark.offset);

  for (auto r : m_renders) {
    r.second->retrace(tracker);
    if (isSelected(r.first, selection)) {
      Uniforms u;
      u.onUniform(selection.id,
                  experimentCount,
                  r.first, callback);
    }
  }
}

void
RetraceContext::setUniform(const RenderSelection &selection,
                           const std::string &name, int index,
                           const std::string &data) {
  for (auto r : m_renders)
    if (isSelected(r.first, selection))
      r.second->setUniform(name, index, data);
}
