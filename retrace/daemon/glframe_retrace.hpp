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

#ifndef _GLFRAME_RETRACE_HPP_
#define _GLFRAME_RETRACE_HPP_

#include <map>
#include <string>
#include <vector>

#include "image.hpp"
#include "trace_parser.hpp"
#include "retrace.hpp"
#include "glframe_state.hpp"

namespace glretrace {

enum RenderTargetType {
  HIGHLIGHT_RENDER,
  NORMAL_RENDER,
  DEPTH_RENDER,
  GEOMETRY_RENDER,
  OVERDRAW_RENDER,
  // etc
};

enum RenderOptions {
  STOP_AT_RENDER = 0x1,
  CLEAR_BEFORE_RENDER = 0x2,
};

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

// masked onto common integer Ids, to prevent any confusion between
// integer identifiers.
enum IdPrefix {
  CALL_ID_PREFIX = 0x1 << 28,
  RENDER_ID_PREFIX = 0x2  << 28,
  RENDER_TARGET_ID_PREFIX = 0x3  << 28,
  METRIC_ID_PREFIX = 0x4  << 28,
  SELECTION_ID_PREFIX = 0x5  << 28,
  EXPERIMENT_ID_PREFIX = 0x6  << 28,
  ID_PREFIX_MASK = 0xf << 28
};

// Decorates Render numbers with a mask, so they can never be confused
// with any other Id
class RenderId {
 public:
  RenderId() : value(0) {}
  explicit RenderId(uint32_t renderNumber) {
    assert(((renderNumber & ID_PREFIX_MASK) == 0) ||
           ((renderNumber & ID_PREFIX_MASK) == RENDER_ID_PREFIX));
    value = RENDER_ID_PREFIX | renderNumber;
  }

  uint32_t operator()() const { return value; }
  uint32_t index() const { return value & (~ID_PREFIX_MASK); }
  bool operator<(const RenderId &o) const { return value < o.value; }
 private:
  uint32_t value;
};

// Decorates Metric numbers with a mask, so they can never be confused
// with any other Id
class MetricId {
 public:
  explicit MetricId(uint64_t metricNumber) {
    assert(((metricNumber & ID_PREFIX_MASK) == 0) ||
           ((metricNumber & ID_PREFIX_MASK) == METRIC_ID_PREFIX));
    value = METRIC_ID_PREFIX | metricNumber;
  }
  MetricId(uint32_t group, uint16_t counter) {
    value = METRIC_ID_PREFIX | (group << 16) | counter;
  }
  MetricId() : value(0) {}

  uint64_t operator()() const { return value; }
  uint32_t group() const { return value >> 16; }
  uint32_t counter() const { return (value & 0x0FFFF); }
  bool operator<(const MetricId &o) const { return value < o.value; }
 private:
  // low 16 bits are the counter number
  // middle 32 bits are the group
  // high 4 bits are the mask
  uint64_t value;
};

// Decorates Selection numbers with a mask, so they can never be
// confused with any other Id
class SelectionId {
 public:
  explicit SelectionId(uint32_t selectionNumber) {
    assert(((selectionNumber & ID_PREFIX_MASK) == 0) ||
           ((selectionNumber & ID_PREFIX_MASK) == SELECTION_ID_PREFIX));
    value = SELECTION_ID_PREFIX | selectionNumber;
  }

  uint32_t operator()() const { return value; }
  uint32_t index() const { return value & (~ID_PREFIX_MASK); }
 private:
  uint32_t value;
};

// Decorates Experiment numbers with a mask, so they can never be
// confused with any other Id
class ExperimentId {
 public:
  ExperimentId() : value(0) {}
  explicit ExperimentId(uint32_t experimentNumber) {
    assert(((experimentNumber & ID_PREFIX_MASK) == 0) ||
           ((experimentNumber & ID_PREFIX_MASK) == EXPERIMENT_ID_PREFIX));
    value = EXPERIMENT_ID_PREFIX | experimentNumber;
  }

  uint32_t operator()() const { return value; }
  uint32_t count() const { return value & (~ID_PREFIX_MASK); }
 private:
  uint32_t value;
};


struct MetricSeries {
  MetricId metric;
  std::vector<float> data;
};

class OnFrameRetrace {
 public:
  typedef std::vector<unsigned char> uvec;
  virtual void onFileOpening(bool finished,
                             uint32_t percent_complete) = 0;
  virtual void onShaderAssembly(RenderId renderId,
                                const std::string &vertex_shader,
                                const std::string &vertex_ir,
                                const std::string &vertex_vec4,
                                const std::string &fragemnt_shader,
                                const std::string &fragemnt_ir,
                                const std::string &fragemnt_simd8,
                                const std::string &fragemnt_simd16) = 0;
  virtual void onRenderTarget(RenderId renderId, RenderTargetType type,
                              const uvec & pngImageData) = 0;
  virtual void onShaderCompile(RenderId renderId, int status,
                               std::string errorString) = 0;
  virtual void onMetricList(const std::vector<MetricId> &ids,
                            const std::vector<std::string> &names) = 0;
  virtual void onMetrics(const MetricSeries &metricData,
                         ExperimentId experimentCount) = 0;
};

class IFrameRetrace {
 public:
  virtual ~IFrameRetrace() {}
  virtual void openFile(const std::string &filename,
                        uint32_t frameNumber,
                        OnFrameRetrace *callback) = 0;
  virtual void retraceRenderTarget(RenderId renderId,
                                   int render_target_number,
                                   RenderTargetType type,
                                   RenderOptions options,
                                   OnFrameRetrace *callback) const = 0;
  virtual void retraceShaderAssembly(RenderId renderId,
                                     OnFrameRetrace *callback) = 0;
  virtual void retraceMetrics(const std::vector<MetricId> &ids,
                              ExperimentId experimentCount,
                              OnFrameRetrace *callback) const = 0;
};

class FrameState {
 private:
  // not needed yet
  // StateTrack tracker;
  // trace::RenderBookmark frame_start;
  // std::vector<trace::RenderBookmark> renders;
  int render_count;
 public:
  FrameState(const std::string &filename,
             int framenumber);
  int getRenderCount() const { return render_count; }
};

class PerfMetrics;
class FrameRetrace : public IFrameRetrace {
 private:
  // these are global
  // trace::Parser parser;
  // retrace::Retracer retracer;

  RenderBookmark frame_start;
  std::vector<RenderBookmark> renders;

  StateTrack tracker;

  // key is context
  std::map<uint64_t, PerfMetrics *> metrics;
  uint64_t initial_frame_context;

  void handleContext(trace::Call *call, OnFrameRetrace* cb);
  bool changesContext(trace::Call *call) const;
  uint64_t getContext(trace::Call *call);

 public:
  FrameRetrace();
  ~FrameRetrace();
  void openFile(const std::string &filename,
                uint32_t frameNumber,
                OnFrameRetrace *callback);

  // TODO(majanes) move to frame state tracker
  int getRenderCount() const;
  // std::vector<int> renderTargets() const;
  void retraceRenderTarget(RenderId renderId,
                           int render_target_number,
                           RenderTargetType type,
                           RenderOptions options,
                           OnFrameRetrace *callback) const;
  void retraceShaderAssembly(RenderId renderId,
                             OnFrameRetrace *callback);
  void retraceMetrics(const std::vector<MetricId> &ids,
                              ExperimentId experimentCount,
                              OnFrameRetrace *callback) const;
  // this is going to be ugly to serialize
  // void insertCall(const trace::Call &call,
  //                 uint32_t renderId,);
  // void setShaders(const std::string &vs,
  //                 const std::string &fs,
  //                 OnFrameRetrace *callback);
  // void revertModifications();
};

} /* namespace glretrace */


#endif /* _GLFRAME_RETRACE_HPP_ */
