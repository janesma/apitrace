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

#include "glframe_retrace.hpp"

#include <stdlib.h>

#include "glretrace.hpp"
#include "trace_dump.hpp"

using glretrace::FrameRetrace;
using glretrace::StateTrack;
using trace::RenderBookmark;
using trace::Call;
using retrace::parser;

extern retrace::Retracer retracer;

class PlayAndCleanUpCall
{
public:
    PlayAndCleanUpCall(Call *c, StateTrack *tracker)
        : call(c) {
        trace::dump(*c, std::cout, 0);
        if (tracker)
            tracker->track(*c);
        retracer.retrace(*call);
    }
    ~PlayAndCleanUpCall() {
        delete call;
    }
private:
    Call *call;
};

FrameRetrace::FrameRetrace(const std::string &filename, int framenumber)
{
    setenv("INTEL_DEBUG", "vs,fs", 1);

    // TODO reroute stdout, to get the shader contents
    
    retracer.addCallbacks(glretrace::gl_callbacks);
    retracer.addCallbacks(glretrace::glx_callbacks);
    retracer.addCallbacks(glretrace::wgl_callbacks);
    retracer.addCallbacks(glretrace::cgl_callbacks);
    retracer.addCallbacks(glretrace::egl_callbacks);

    retrace::setUp();
    parser.open(filename.c_str());

    // play up to the requested frame
    trace::Call *call;
    int current_frame = 0;
    while ((call = parser.parse_call()) && current_frame < framenumber)
    {
        PlayAndCleanUpCall c(call, &tracker);
        if (call->flags & trace::CALL_FLAG_END_FRAME)
        {
            ++current_frame;
            if (current_frame == framenumber)
                break;
        }
    }
    
    parser.getBookmark(frame_start.start);
    renders.push_back(RenderBookmark());
    parser.getBookmark(renders.back().start);

    // play through the frame, recording renders and call counts
    while ((call = parser.parse_call()))
    {
        PlayAndCleanUpCall c(call, &tracker);

        ++frame_start.numberOfCalls;
        ++(renders.back().numberOfCalls);
        
        if (current_frame > framenumber) {
            return;
        }
        if (call->flags & trace::CALL_FLAG_RENDER) {
            renders.push_back(RenderBookmark());
            parser.getBookmark(renders.back().start);
        }
            
        if (call->flags & trace::CALL_FLAG_END_FRAME) {
            renders.pop_back();
            break;
        }
    }
}

std::vector<trace::RenderBookmark>
FrameRetrace::getRenders() const {
    return renders;
}


void
FrameRetrace::retraceRenderTarget(const trace::RenderBookmark &render,
                                  int render_target_number,
                                  RenderTargetType type,
                                  RenderOptions options,
                                  OnFrameRetrace *callback) const
{
    trace::Call *call;

    // reset to beginning of frame
    parser.setBookmark(frame_start.start);
    int played_calls = 0;

    // play up to the beginning of the render
    int current_call = frame_start.start.next_call_no;
    while ((current_call < render.start.next_call_no) &&
           (call = parser.parse_call()))
    {
        ++played_calls;
        ++current_call;
        PlayAndCleanUpCall c(call, NULL);
    }

    if (options & glretrace::CLEAR_BEFORE_RENDER)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // play up to the end of the render
    for (int render_calls = 0; render_calls < render.numberOfCalls; ++render_calls)
    {
        call = parser.parse_call();
        if (!call)
            break;
        
        ++played_calls;
        PlayAndCleanUpCall c(call, NULL);
    }

    // play remainder of frame
    while ((played_calls < frame_start.numberOfCalls - 1) &&
           (call = parser.parse_call()))
    {
        ++played_calls;
        if (! (options & glretrace::STOP_AT_RENDER))
        {
            PlayAndCleanUpCall c(call, NULL);
        }
    }

    // play the swapBuffers
    if ((call = parser.parse_call()))
        PlayAndCleanUpCall c(call, NULL);
        
}
