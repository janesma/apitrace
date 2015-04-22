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

#include "glstate_images.hpp"
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

struct RenderTargetData {
    glstate::ImageDesc description;
    image::Image imageData;
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

class OnFrameRetrace {
public:
    virtual void onShaderAssembly(const RenderBookmark &render,
                                  const std::string &vertex_assembly,
                                  const std::string &shader_assembly) = 0;
    virtual void onRenderTarget(const RenderBookmark &render, RenderTargetType type,
                                const RenderTargetData &data) = 0;
    virtual void onShaderCompile(const RenderBookmark &render, int status,
                                 std::string errorString) = 0;
};

class FrameRetrace
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
    FrameRetrace(const std::string &filename, int framenumber);
    std::vector<RenderBookmark> getRenders() const;
    std::vector<int> renderTargets() const;
    void retraceRenderTarget(const RenderBookmark &render,
                             int render_target_number,
                             RenderTargetType type,
                             RenderOptions options,
                             OnFrameRetrace *callback) const;
    void retraceShaderAssembly(const RenderBookmark &render,
                               OnFrameRetrace *callback);
    void insertCall(const trace::Call &call,
                    const RenderBookmark &render);
    void setShaders(const std::string &vs,
                    const std::string &fs,
                    OnFrameRetrace *callback);
    void revertModifications();
};

} /* namespace glretrace */


#endif /* _GLFRAME_RETRACE_HPP_ */
