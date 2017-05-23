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

#ifndef _GLFRAME_RETRACE_CONTEXT_HPP_
#define _GLFRAME_RETRACE_CONTEXT_HPP_

#include <map>
#include <string>
#include <vector>

#include "glframe_retrace_interface.hpp"
#include "glframe_retrace.hpp"
#include "trace_parser.hpp"

namespace trace {
class AbstractParser;
class Call;
}
namespace retrace {
class Retracer;
}

namespace glretrace {

struct Context;

class BatchControl;
class StateTrack;
class OnFrameRetrace;
class ExperimentId;
class MetricId;
class OutputPoller;
class PerfMetrics;
class RetraceRender;

class RetraceContext {
 public:
  RetraceContext(RenderId current_render,
                 trace::AbstractParser *parser,
                 retrace::Retracer *retracer,
                 StateTrack *tracker);
  void retraceRenderTarget(ExperimentId experimentCount,
                           const RenderSelection &selection,
                           RenderTargetType type,
                           RenderOptions options,
                           const StateTrack &tracker,
                           OnFrameRetrace *callback) const;
  void retraceMetrics(PerfMetrics *perf, const StateTrack &tracker) const;
  void retraceAllMetrics(const RenderSelection &selection,
                         PerfMetrics *perf,
                         const StateTrack &tracker) const;
  bool endsFrame() const;
  bool replaceShaders(RenderId renderId,
                      ExperimentId experimentCount,
                      StateTrack *tracker,
                      const std::string &vs,
                      const std::string &fs,
                      const std::string &tessControl,
                      const std::string &tessEval,
                      const std::string &geom,
                      const std::string &comp,
                      OnFrameRetrace *callback);
  void disableDraw(RenderId render, bool disable);
  void simpleShader(RenderId render, bool simple);
  void retraceApi(const RenderSelection &selection,
                  OnFrameRetrace *callback);
  void retraceShaderAssembly(const RenderSelection &selection,
                             ExperimentId experimentCount,
                             StateTrack *tracker,
                             OnFrameRetrace *callback);
  int getRenderCount() const;
  void retraceBatch(const RenderSelection &selection,
                    ExperimentId experimentCount,
                    const StateTrack &tracker,
                    BatchControl *control,
                    OutputPoller *poller,
                    OnFrameRetrace *callback);
  void retraceUniform(const RenderSelection &selection,
                      ExperimentId experimentCount,
                      const StateTrack &tracker,
                      OnFrameRetrace *callback);

 private:
  trace::AbstractParser *m_parser;
  retrace::Retracer *m_retracer;
  trace::ParseBookmark m_start_bookmark, m_end_bookmark;
  trace::Call *m_context_switch;
  RenderBookmark m_context_start;
  std::map<RenderId, RetraceRender*> m_renders;
  std::vector<RenderId> end_render_target_regions;
  bool m_ends_frame;

  RenderId lastRenderForRTRegion(RenderId render) const;
};

}  // namespace glretrace

#endif  // _GLFRAME_RETRACE_CONTEXT_HPP_

