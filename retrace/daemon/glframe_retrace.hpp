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

#ifndef _GLFRAME_RETRACE_HPP_
#define _GLFRAME_RETRACE_HPP_

#include <map>
#include <string>
#include <vector>

#include "image.hpp"
#include "trace_parser.hpp"
#include "retrace.hpp"
#include "glframe_retrace_interface.hpp"
#include "glframe_state.hpp"

namespace glretrace {

struct RenderBookmark {
  RenderBookmark()
      : numberOfCalls(0)
  {}
  explicit RenderBookmark(const trace::ParseBookmark &s)
      : start(s),
        numberOfCalls(0)
  {}
  trace::ParseBookmark start;
  unsigned numberOfCalls;
};

class PerfMetrics;
class RetraceRender;
class RetraceContext;
class FrameRetrace : public IFrameRetrace {
 public:
  FrameRetrace();
  ~FrameRetrace();
  void openFile(const std::string &filename,
                const std::vector<unsigned char> &md5,
                uint64_t fileSize,
                uint32_t frameNumber,
                OnFrameRetrace *callback);

  // TODO(majanes) move to frame state tracker
  int getRenderCount() const;
  // std::vector<int> renderTargets() const;
  void retraceRenderTarget(ExperimentId experimentCount,
                           const RenderSelection &selection,
                           RenderTargetType type,
                           RenderOptions options,
                           OnFrameRetrace *callback) const;
  void retraceShaderAssembly(const RenderSelection &selection,
                             OnFrameRetrace *callback);
  void retraceMetrics(const std::vector<MetricId> &ids,
                      ExperimentId experimentCount,
                      OnFrameRetrace *callback) const;
  void retraceAllMetrics(const RenderSelection &selection,
                         ExperimentId experimentCount,
                         OnFrameRetrace *callback) const;
  void replaceShaders(RenderId renderId,
                      ExperimentId experimentCount,
                      const std::string &vs,
                      const std::string &fs,
                      const std::string &tessControl,
                      const std::string &tessEval,
                      const std::string &geom,
                      const std::string &comp,
                      OnFrameRetrace *callback);
  // this is going to be ugly to serialize
  // void insertCall(const trace::Call &call,
  //                 uint32_t renderId,);
  // void setShaders(const std::string &vs,
  //                 const std::string &fs,
  //                 OnFrameRetrace *callback);
  // void revertModifications();
  void retraceApi(const RenderSelection &selection,
                  OnFrameRetrace *callback);

  void retraceBatch(const RenderSelection &selection,
                            OnFrameRetrace *callback);

 private:
  // these are global
  // trace::Parser parser;
  // retrace::Retracer retracer;

  RenderBookmark frame_start;
  std::vector<RetraceContext*> m_contexts;
  StateTrack m_tracker;
  PerfMetrics * m_metrics;

  // each entry is the last render in an RT region
  std::vector<RenderId> render_target_regions;
};

} /* namespace glretrace */


#endif /* _GLFRAME_RETRACE_HPP_ */
