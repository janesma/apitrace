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

// #include "glstate_images.hpp"
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
  RenderBookmark(const trace::ParseBookmark &s)
        : start(s),
          numberOfCalls(0)
    {}
  trace::ParseBookmark start;
    unsigned numberOfCalls;
};    

enum IdPrefix {
  CALL_ID_PREFIX = 0x1 << 28,
  RENDER_ID_PREFIX = 0x2  << 28,
  RENDER_TARGET_ID_PREFIX = 0x3  << 28,
  ID_PREFIX_MASK = 0xf << 28
};

class RenderId {
 public:
  RenderId(uint32_t renderNumber) {
    assert(((renderNumber & ID_PREFIX_MASK) == 0) ||
           ((renderNumber & ID_PREFIX_MASK) == RENDER_ID_PREFIX));
    value = RENDER_ID_PREFIX | renderNumber;
  }

  uint32_t operator()() const { return value; }
  uint32_t index() const { return value & (~ID_PREFIX_MASK); }
 private:
  uint32_t value;
};

class OnFrameRetrace {
 public:
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
                              const std::vector<unsigned char> &pngImageData) = 0;
  virtual void onShaderCompile(RenderId renderId, int status,
                               std::string errorString) = 0;
};

class IFrameRetrace
{
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


class FrameRetrace : public IFrameRetrace
{
 private:
  // these are global
  //trace::Parser parser;
  //retrace::Retracer retracer;

  RenderBookmark frame_start;
  std::vector<RenderBookmark> renders;

  StateTrack tracker;
 public:
  // 
  FrameRetrace();
  void openFile(const std::string &filename,
                uint32_t frameNumber,
                OnFrameRetrace *callback);

  // TODO move to frame state tracker
  int getRenderCount() const;
  //std::vector<int> renderTargets() const;
  void retraceRenderTarget(RenderId renderId,
                           int render_target_number,
                           RenderTargetType type,
                           RenderOptions options,
                           OnFrameRetrace *callback) const;
  void retraceShaderAssembly(RenderId renderId,
                             OnFrameRetrace *callback);
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
