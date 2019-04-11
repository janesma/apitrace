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

#include <string>
#include <sstream>
#include <vector>

#include "glframe_batch.hpp"
#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"
#include "glframe_metrics.hpp"
#include "glframe_retrace_render.hpp"
#include "glframe_retrace_texture.hpp"
#include "glframe_state_override.hpp"
#include "glframe_uniforms.hpp"
#include "glretrace.hpp"
#include "glstate.hpp"
#include "glstate_internal.hpp"
#include "image.hpp"
#include "trace_model.hpp"

using image::Image;
using glretrace::BatchControl;
using glretrace::ExperimentId;
using glretrace::OutputPoller;
using glretrace::OnFrameRetrace;
using glretrace::PerfMetrics;
using glretrace::RenderId;
using glretrace::RetraceRender;
using glretrace::RenderOptions;
using glretrace::RenderSelection;
using glretrace::RenderTargetType;
using glretrace::RetraceContext;
using glretrace::StateKey;
using glretrace::StateOverride;
using glretrace::SelectionId;
using glretrace::StateTrack;
using glretrace::Textures;
using glretrace::WARN;

static int geometry_render_supported = -1;

RetraceContext::RetraceContext(RenderId current_render,
                               unsigned int tex2x2,
                               trace::AbstractParser *parser,
                               retrace::Retracer *retracer,
                               StateTrack *tracker,
                               const CancellationPolicy &cancel)
    : m_parser(parser), m_retracer(retracer),
      m_context_switch(NULL), m_ends_frame(false),
      m_textures(new Textures),
      m_cancelPolicy(cancel) {

  if (geometry_render_supported == -1) {
    GLint pgm[2];
    GlFunctions::GetIntegerv(GL_POLYGON_MODE, pgm);
    if (GL_NO_ERROR == GL::GetError())
      geometry_render_supported = 1;
    else
      geometry_render_supported = 0;
  }

  m_parser->getBookmark(m_start_bookmark);
  trace::Call *call = parser->parse_call();
  if (ThreadContext::changesContext(*call))
    m_context_switch = call;
  else
    delete call;

  m_parser->setBookmark(m_start_bookmark);

  int current_render_buffer = RetraceRender::currentRenderBuffer();
  // play through the frame, generating renders
  while (true) {
    auto r = new RetraceRender(tex2x2, parser, retracer, tracker);
    m_renders[current_render] = r;
    ++current_render;
    if (r->endsFrame()) {
      m_ends_frame = true;
      break;
    }

    // peek ahead to see if next call is in the following context
    m_parser->getBookmark(m_end_bookmark);
    call = parser->parse_call();
    m_parser->setBookmark(m_end_bookmark);
    if (ThreadContext::changesContext(*call)) {
      m_parser->setBookmark(m_end_bookmark);
      delete call;
      break;
    }

    const int new_render_buffer = RetraceRender::currentRenderBuffer();

    // consider refactoring context to be a container of rt regions
    if (new_render_buffer != current_render_buffer) {
      if (m_renders.size() > 1)  // don't record the very first draw as
                               // ending a render target
        m_end_render_target_regions.push_back(RenderId(m_renders.size() - 1));
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
  for (auto rt_render : m_end_render_target_regions)
    if (rt_render > render)
      return RenderId(rt_render() - 1);
  // last render for the context is the final render for the last
  // region.
  return m_renders.rbegin()->first;
}

// see glstate_images.cpp:973
enum RtImage {
  kOverDraw = -3,
  kStencil = -2,
  kDepth = -1
};

void
normalize_image(Image *image, int rt_num) {
  float * const pixels = reinterpret_cast<float*>(image->pixels);
  const int pixel_count = image->width * image->height;
  switch (rt_num) {
    case kDepth: {
      // normalize the values in the depth image, so the minimum value
      // is black and max is white.  Failure to do this will render a
      // mostly-white depth buffer.  Normalizing on the client (UI)
      // side results in banding, because Apitrace's PNG support
      // converts grayscale images to 8bit depth.  PNG is the format
      // used to send image data from the server to the client.

      // get the global max/min for pixel greyscale in the image
      float min = 1.0, max = 0.0;
      for (int i = 0; i < pixel_count; ++i) {
        if (pixels[i] > max)
          max = pixels[i];
        if (pixels[i] < min)
          min = pixels[i];
      }
      const float range = max - min;
      // correct each pixel so the greyscale coves [0.0,1.0] instead of
      // the narrower range in the original image.
      for (int i = 0; i < pixel_count; ++i) {
        pixels[i] = (pixels[i] - min) / range;
      }
      break;
    }
    case kStencil: {
      // make pixels in stencil image either white or black
      if (image->channelType != image::TYPE_FLOAT)
        return;
      for (int i = 0; i < pixel_count; ++i) {
        if (pixels[i])
          pixels[i] = 1.0f;
      }
      break;
    }
    default:
      return;
  }
}

void clear_all() {
  // clear each attachment individually, in case the current context
  // lacks support for one.  disregard errors for unsupported
  // attachments.
  glretrace::GlFunctions::Clear(GL_COLOR_BUFFER_BIT);
  glretrace::GlFunctions::Clear(GL_DEPTH_BUFFER_BIT);
  glretrace::GlFunctions::Clear(GL_STENCIL_BUFFER_BIT);
  glretrace::GlFunctions::Clear(GL_ACCUM_BUFFER_BIT);

  GLint count;
  glretrace::GlFunctions::GetIntegerv(GL_MAX_DRAW_BUFFERS, &count);
  const float zero[] = {0, 0, 0, 0};
  for (int i = 0; i < count; ++i) {
    glretrace::GlFunctions::ClearBufferfv(GL_COLOR, i, zero);
  }
  float default_depth;
  glretrace::GlFunctions::GetFloatv(GL_DEPTH_CLEAR_VALUE, &default_depth);
  glretrace::GlFunctions::ClearBufferfv(GL_DEPTH, 0, &default_depth);
  GLint default_stencil;
  glretrace::GlFunctions::GetIntegerv(GL_STENCIL_CLEAR_VALUE, &default_stencil);
  glretrace::GlFunctions::ClearBufferiv(GL_STENCIL, 0, &default_stencil);
  // ignore errors from unsupported clears
  glretrace::GlFunctions::GetError();
}

void
RetraceContext::retraceRenderTarget(ExperimentId experimentCount,
                                    const RenderSelection &selection,
                                    RenderTargetType type,
                                    RenderOptions options,
                                    const StateTrack &tracker,
                                    OnFrameRetrace *callback) const {
  if (m_renders.empty())
    return;
  if ((type == GEOMETRY_RENDER) && (!geometry_render_supported))
    return;

  auto current_render = m_renders.begin();

  // for highlight renders, we only want to highlight what is
  // selected.
  RenderTargetType unselected_type = type;
  if (type == HIGHLIGHT_RENDER)
    unselected_type = NORMAL_RENDER;
  else if (type == GEOMETRY_RENDER)
    unselected_type = NULL_RENDER;

  // play up to the beginning of the first render
  while (current_render->first < selection.series.front().begin) {
    current_render->second->retraceRenderTarget(tracker, unselected_type);
    ++current_render;
    if (current_render == m_renders.end())
      // played through the context
      return;
  }

  if (selection.series.front().begin == current_render->first) {
    // this is the first render of the selection
    if ((options & glretrace::CLEAR_BEFORE_RENDER)) {
      clear_all();
    }
  }

  // play through the selected renders
  const RenderId last_render = lastRender(selection);
  while (current_render->first < selection.series.back().end) {
    RenderTargetType current_type = type;
    if (!isSelected(current_render->first, selection))
      // unselected renders don't get highlighting
      current_type = unselected_type;
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
                                                  unselected_type);
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

    // re-order the images to put depth/stencil at the bottom
    std::vector<int> rt_indices;
    if (type == GEOMETRY_RENDER || type == OVERDRAW_RENDER) {
      rt_indices.push_back(0);
    } else {
      for (int rt_num = 0; rt_num < image_count; ++rt_num)
        rt_indices.push_back(rt_num);
      // request depth and stencil images
      rt_indices.push_back(kDepth);
      rt_indices.push_back(kStencil);
    }

    if (callback) {
      for (auto rt_num : rt_indices) {
        Image *i = glstate::getDrawBufferImage(rt_num);
        if (!i) {
          // it is typical for some render targets to be inaccessible
          continue;
        }
        // else construct the appropriate label for the image
        std::string label;
        switch (rt_num) {
          case kDepth:
            label = "depth";
            break;
          case kStencil:
            label = "stencil";
            break;
          default: {
            std::stringstream slabel;
            slabel <<  "attachment " << rt_num;
            label = slabel.str();
            break;
          }
        }
        if (type == GEOMETRY_RENDER)
          label = "geometry";
        if (type == OVERDRAW_RENDER) {
          label = "overdraw";
          rt_num = kOverDraw;
        }

        normalize_image(i, rt_num);
        std::stringstream png;
        i->writePNG(png);

        std::vector<unsigned char> d;
        const int bytes = png.str().size();
        d.resize(bytes);
        memcpy(d.data(), png.str().c_str(), bytes);
        callback->onRenderTarget(selection.id, experimentCount, label, d);
        delete i;
      }

      // after reporting the RT image, clear all attachments to
      // prevent artifacts from appearing in subsequent retraces.
      clear_all();
    }
  }

  // play to the rest of the context
  while (current_render != m_renders.end()) {
    current_render->second->retraceRenderTarget(tracker,
                                                unselected_type);
    ++current_render;
  }
  clear_all();
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
  if (m_renders.empty())
    return;
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

  // play to end of context
  while (current_render != m_renders.end()) {
    current_render->second->retrace(tracker);
    ++current_render;
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
    if (isSelected(r.first, selection) &&
        (!m_cancelPolicy.isCancelled(selection.id, experimentCount))) {
      GlFunctions::Finish();
      control->batchOn();
    } else {
      control->batchOff();
    }
    poller->flush();
    r.second->retrace(tracker);
    if (isSelected(r.first, selection)) {
      GlFunctions::Finish();
      poller->pollBatch(selection.id, experimentCount, r.first, callback);
    }
  }
  control->batchOff();
}

class UniformHook : public RetraceRender::CallbackHook {
 public:
  UniformHook(SelectionId s, ExperimentId e,
              RenderId r, OnFrameRetrace *c)
      : m_s(s), m_e(e), m_r(r), m_c(c) {}
  void onCallbackReady() const {
    glretrace::Uniforms u;
    u.onUniform(m_s, m_e, m_r, m_c);
  }
 private:
  SelectionId m_s;
  ExperimentId m_e;
  RenderId m_r;
  OnFrameRetrace *m_c;
};

void
RetraceContext::retraceUniform(const RenderSelection &selection,
                               ExperimentId experimentCount,
                               const StateTrack &tracker,
                               OnFrameRetrace *callback) {
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_start_bookmark.offset);

  for (auto r : m_renders) {
    if (isSelected(r.first, selection)) {
      // pass down the context that is needed to make the uniform callback
      const UniformHook c(selection.id,
                             experimentCount,
                             r.first, callback);
      r.second->retrace(tracker, &c);
    } else {
      r.second->retrace(tracker);
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


class StateHook : public RetraceRender::CallbackHook {
 public:
  StateHook(SelectionId s, ExperimentId e,
            RenderId r, OnFrameRetrace *c)
      : m_s(s), m_e(e), m_r(r), m_c(c) {}
  void onCallbackReady() const {
    StateOverride::onState(m_s, m_e, m_r, m_c);
  }
 private:
  SelectionId m_s;
  ExperimentId m_e;
  RenderId m_r;
  OnFrameRetrace *m_c;
};

void
RetraceContext::retraceState(const RenderSelection &selection,
                             ExperimentId experimentCount,
                             const StateTrack &tracker,
                             OnFrameRetrace *callback) {
  for (auto r : m_renders) {
    if (isSelected(r.first, selection)) {
      const StateHook c(selection.id, experimentCount, r.first, callback);
      r.second->retrace(tracker, &c);
    } else {
      r.second->retrace(tracker);
    }
  }
}

void
RetraceContext::setState(const RenderSelection &selection,
                         const StateKey &item,
                         int offset,
                         const std::string &value,
                         const StateTrack &tracker) {
  for (auto r : m_renders) {
    r.second->retrace(tracker);
    if (isSelected(r.first, selection))
      r.second->setState(item, offset, value);
  }
}

void
RetraceContext::revertState(const RenderSelection &selection,
                            const StateKey &item) {
  for (auto r : m_renders) {
    if (isSelected(r.first, selection))
      r.second->revertState(item);
  }
}

void
RetraceContext::revertExperiments(StateTrack *tracker) {
  for (auto r : m_renders)
    r.second->revertExperiments(tracker);
}

void
RetraceContext::texture2x2(const RenderSelection &selection,
                           bool enable) {
  for (auto r : m_renders)
    if (isSelected(r.first, selection))
      r.second->texture2x2(enable);
}


void
RetraceContext::retraceTextures(const RenderSelection &selection,
                                ExperimentId experimentCount,
                                const StateTrack &tracker,
                                OnFrameRetrace *callback) {
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_start_bookmark.offset);
  glstate::Context c;
  for (auto r : m_renders) {
    r.second->retrace(tracker);
    if (m_cancelPolicy.isCancelled(selection.id, experimentCount))
        continue;
    if (isSelected(r.first, selection)) {
      m_textures->retraceTextures(r.first, selection.id,
                                  experimentCount, callback);
    }
  }
}
