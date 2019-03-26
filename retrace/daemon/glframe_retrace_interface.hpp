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

#ifndef _GLFRAME_RETRACE_INTERFACE_HPP_
#define _GLFRAME_RETRACE_INTERFACE_HPP_

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>

namespace glretrace {

enum RenderTargetType {
  HIGHLIGHT_RENDER,
  NORMAL_RENDER,
  GEOMETRY_RENDER,
  OVERDRAW_RENDER,
  NULL_RENDER
};

enum RenderOptions {
  DEFAULT_RENDER = 0x0,
  STOP_AT_RENDER = 0x1,
  CLEAR_BEFORE_RENDER = 0x2,
};

enum ErrorSeverity {
  RETRACE_WARN,
  RETRACE_FATAL
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
    if (renderNumber == (uint32_t)-1)
      // -1 renderId means "no render".  This id addresses, for
      // -example, tracked state up to the point where the frame
      // -begins.
      renderNumber ^= ID_PREFIX_MASK;
    assert(((renderNumber & ID_PREFIX_MASK) == 0) ||
           ((renderNumber & ID_PREFIX_MASK) == RENDER_ID_PREFIX));
    value = RENDER_ID_PREFIX | renderNumber;
  }

  uint32_t operator()() const { return value; }
  RenderId &operator++() { ++value; return *this; }
  uint32_t index() const { return value & (~ID_PREFIX_MASK); }
  bool operator<(const RenderId &o) const { return value < o.value; }
  bool operator>(const RenderId &o) const { return value > o.value; }
  bool operator>=(const RenderId &o) const { return value >= o.value; }
  bool operator<=(const RenderId &o) const { return value <= o.value; }
  bool operator==(const RenderId &o) const { return value == o.value; }
  bool operator!=(const RenderId &o) const { return value != o.value; }
  static const uint32_t INVALID_RENDER = (-1 & ~ID_PREFIX_MASK);

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
  uint32_t group() const { return ((value >> 16) & 0x0FFF); }
  uint32_t counter() const { return (value & 0x0FFFF); }
  bool operator<(const MetricId &o) const { return value < o.value; }
  bool operator==(const MetricId &o) const { return value == o.value; }
  bool operator!=(const MetricId &o) const { return value != o.value; }

 private:
  // low 16 bits are the counter number
  // middle 32 bits are the group
  // high 4 bits are the mask
  uint64_t value;
};

// Decorates Selection numbers with a mask, so they can never be
// confused with any other Id.  A global SelectionId increments every
// time the users selects new renders, triggering retrace requests.
// Retrace requests carry the new id when they are enqueued.  Stale
// requests will be canceled when they are dequeued, by checking the
// global id that was subsequently incremented.  Asynchronous
// callbacks which contain a stale id will not be displayed to the
// user, because there is a retrace for a newer selection in the
// queue.
class SelectionId {
 public:
  explicit SelectionId(uint32_t selectionNumber) {
    assert(((selectionNumber & ID_PREFIX_MASK) == 0) ||
           ((selectionNumber & ID_PREFIX_MASK) == SELECTION_ID_PREFIX));
    value = SELECTION_ID_PREFIX | selectionNumber;
  }
  SelectionId() : value(0) {}

  uint32_t operator()() const { return value; }
  SelectionId &operator++() { ++value; return *this; }
  uint32_t count() const { return value & (~ID_PREFIX_MASK); }
  bool operator<=(const SelectionId &o) const { return value <= o.value; }
  bool operator>(const SelectionId &o) const { return value > o.value; }
  bool operator==(const SelectionId &o) const { return value == o.value; }
  bool operator!=(const SelectionId &o) const { return value != o.value; }
  static const uint32_t INVALID_SELECTION = (-1 & ~ID_PREFIX_MASK);

 private:
  uint32_t value;
};

// Decorates Experiment numbers with a mask, so they can never be
// confused with any other Id.  Retrace requests originating from an
// experiment will be enqueue with the value of a globally
// incrementing ExperimentId.  See SelectionId.
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
  ExperimentId &operator++() { ++value; return *this; }
  bool operator<(const ExperimentId &o) const { return value < o.value; }
  bool operator>(const ExperimentId &o) const { return value > o.value; }
  bool operator<=(const ExperimentId &o) const { return value <= o.value; }
  bool operator!=(const ExperimentId &o) const { return value != o.value; }
  bool operator==(const ExperimentId &o) const { return value == o.value; }
  static const uint32_t INVALID_EXPERIMENT = (-1 & ~ID_PREFIX_MASK);
 private:
  uint32_t value;
};


struct MetricSeries {
  MetricId metric;
  std::vector<float> data;
};

struct RenderSequence {
  RenderSequence(RenderId b, RenderId e) : begin(b), end(e) {}
  RenderSequence(const RenderSequence &o) : begin(o.begin), end(o.end) {}
  RenderSequence() {}
  RenderId begin;
  RenderId end;
};

typedef std::vector<RenderSequence> RenderSeries;

struct RenderSelection {
  SelectionId id;
  RenderSeries series;
  RenderSelection() {}
  RenderSelection(const RenderSelection &o)
      : id(o.id), series(o.series) {}
  void clear() { series.clear(); }
  void push_back(int begin, int end) {
    series.push_back(RenderSequence(RenderId(begin), RenderId(end)));
  }
  void push_back(int render) {
    series.push_back(RenderSequence(RenderId(render), RenderId(render+1)));
  }
};

struct ShaderAssembly {
  std::string shader;
  std::string ir;
  std::string ssa;
  std::string nir;
  std::string simd;
  std::string simd8;
  std::string simd16;
  std::string simd32;
  std::string beforeUnification;
  std::string afterUnification;
  std::string beforeOptimization;
  std::string constCoalescing;
  std::string genIrLowering;
  std::string layout;
  std::string optimized;
  std::string pushAnalysis;
  std::string codeHoisting;
  std::string codeSinking;
};

enum UniformType {
  kFloatUniform,
  kIntUniform,
  kUIntUniform,
  kBoolUniform
};

enum UniformDimension {
  k1x1,
  k2x1,
  k3x1,
  k4x1,
  k2x2,
  k3x2,
  k4x2,
  k2x3,
  k3x3,
  k4x3,
  k2x4,
  k3x4,
  k4x4
};

struct StateKey {
  std::string path;
  std::string name;

  StateKey() {}
  StateKey(const std::string _path,
           const std::string _name)
      : path(_path), name(_name) {}
  bool operator<(const StateKey &o) const {
    if (path < o.path)
      return true;
    if (path > o.path)
      return false;
    return (name < o.name);
  }
};

enum TextureTarget {
  kTexture2d,
  kTextureCube,
  kTexture3d,
  kTexture2dArray
};
struct TextureKey {
  int unit;  // from zero up to  GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS minus one.
  int target;  // GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, etc
  int offset;  // for cube maps, 0-5.  For 3d/2dArray, the slice of the image

  TextureKey() : unit(-1), target(-1), offset(-1) {}
  TextureKey(int u, int t, int o) : unit(u), target(t), offset(o) {}
};

struct TextureData {
  // from https://www.khronos.org/registry/OpenGL-Refpages/es3.0/html/glTexImage2D.xhtml
  int level;
  std::string internalFormat;
  int width;
  int height;
  std::string format;
  std::string type;
  std::vector<unsigned char> image_data;
  TextureData() {}
  TextureData(int l,
              const std::string &iformat,
              int w,
              int h,
              const std::string &fmt,
              const std::string &t,
              const std::string &data) : level(l),
                                         internalFormat(iformat),
                                         width(w),
                                         height(h),
                                         format(fmt),
                                         type(t),
                                         image_data(data.size()) {
    memcpy(image_data.data(), data.c_str(), data.size());
  }
};

// Serializable asynchronous callbacks made from remote
// implementations of IFrameRetrace.
class OnFrameRetrace {
 public:
  typedef std::vector<unsigned char> uvec;
  virtual void onFileOpening(bool needUpload,
                             bool finished,
                             uint32_t frame_count) = 0;
  virtual void onShaderAssembly(RenderId renderId,
                                SelectionId selectionCount,
                                ExperimentId experimentCount,
                                const ShaderAssembly &vertex,
                                const ShaderAssembly &fragment,
                                const ShaderAssembly &tess_control,
                                const ShaderAssembly &tess_eval,
                                const ShaderAssembly &geom,
                                const ShaderAssembly &comp) = 0;
  virtual void onRenderTarget(SelectionId selectionCount,
                              ExperimentId experimentCount,
                              const std::string &label,
                              const uvec & pngImageData) = 0;
  virtual void onMetricList(const std::vector<MetricId> &ids,
                            const std::vector<std::string> &names,
                            const std::vector<std::string> &descriptions) = 0;
  virtual void onMetrics(const MetricSeries &metricData,
                         ExperimentId experimentCount,
                         SelectionId selectionCount) = 0;
  virtual void onShaderCompile(RenderId renderId,
                               ExperimentId experimentCount,
                               bool status,
                               const std::string &errorString) = 0;
  virtual void onApi(SelectionId selectionCount,
                     RenderId renderId,
                     const std::vector<std::string> &api_calls) = 0;
  virtual void onError(ErrorSeverity s, const std::string &message) = 0;
  virtual void onBatch(SelectionId selectionCount,
                       ExperimentId experimentCount,
                       RenderId renderId,
                       const std::string &batch) = 0;
  virtual void onUniform(SelectionId selectionCount,
                         ExperimentId experimentCount,
                         RenderId renderId,
                         const std::string &name,
                         UniformType type,
                         UniformDimension dimension,
                         const std::vector<unsigned char> &data) = 0;
  virtual void onState(SelectionId selectionCount,
                       ExperimentId experimentCount,
                       RenderId renderId,
                       StateKey item,
                       const std::vector<std::string> &value) = 0;
  virtual void onTexture(SelectionId selectionCount,
                         ExperimentId experimentCount,
                         RenderId renderId,
                         TextureKey binding,
                         const std::vector<TextureData> &images) = 0;
};

// Serializable asynchronous retrace requests.
class IFrameRetrace {
 public:
  virtual ~IFrameRetrace() {}
  // server responds with onFileOpening until finished.  For remote
  // connections, client will immediately send file data if server
  // responds that md5 is not located in the cache.  Sending file data
  // is not expressed in this interface or in the protobuf interface,
  // because data will be too large to format into a message.
  virtual void openFile(const std::string &filename,
                        const std::vector<unsigned char> &md5,
                        uint64_t fileSize,
                        uint32_t frameNumber,
                        uint32_t frameCount,
                        OnFrameRetrace *callback) = 0;
  virtual void retraceRenderTarget(ExperimentId experimentCount,
                                   const RenderSelection &selection,
                                   RenderTargetType type,
                                   RenderOptions options,
                                   OnFrameRetrace *callback) const = 0;
  virtual void retraceShaderAssembly(const RenderSelection &selection,
                                     ExperimentId experimentCount,
                                     OnFrameRetrace *callback) = 0;
  virtual void retraceMetrics(const std::vector<MetricId> &ids,
                              ExperimentId experimentCount,
                              OnFrameRetrace *callback) const = 0;
  virtual void retraceAllMetrics(const RenderSelection &selection,
                                 ExperimentId experimentCount,
                                 OnFrameRetrace *callback) const = 0;
  virtual void replaceShaders(RenderId renderId,
                              ExperimentId experimentCount,
                              const std::string &vs,
                              const std::string &fs,
                              const std::string &tessControl,
                              const std::string &tessEval,
                              const std::string &geom,
                              const std::string &comp,
                              OnFrameRetrace *callback) = 0;
  virtual void disableDraw(const RenderSelection &selection,
                           bool disable) = 0;
  virtual void simpleShader(const RenderSelection &selection,
                            bool simple) = 0;
  virtual void oneByOneScissor(const RenderSelection &selection,
                               bool scissor) = 0;
  virtual void wireframe(const RenderSelection &selection,
                         bool wireframe) = 0;
  virtual void texture2x2(const RenderSelection &selection,
                          bool texture_2x2) = 0;
  virtual void retraceApi(const RenderSelection &selection,
                          OnFrameRetrace *callback) = 0;
  virtual void retraceBatch(const RenderSelection &selection,
                            ExperimentId experimentCount,
                            OnFrameRetrace *callback) = 0;
  virtual void retraceUniform(const RenderSelection &selection,
                              ExperimentId experimentCount,
                              OnFrameRetrace *callback) = 0;
  virtual void setUniform(const RenderSelection &selection,
                          const std::string &name,
                          int index,
                          const std::string &data) = 0;
  virtual void retraceState(const RenderSelection &selection,
                            ExperimentId experimentCount,
                            OnFrameRetrace *callback) = 0;
  virtual void setState(const RenderSelection &selection,
                        const StateKey &item,
                        int offset,
                        const std::string &value) = 0;
  virtual void retraceTextures(const RenderSelection &selection,
                               ExperimentId experimentCount,
                               OnFrameRetrace *callback) = 0;
  virtual void revertExperiments() = 0;
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
             int framenumber, int framecount);
  int getRenderCount() const { return render_count; }
};

} /* namespace glretrace */


#endif /* _GLFRAME_RETRACE_INTERFACE_HPP_ */
