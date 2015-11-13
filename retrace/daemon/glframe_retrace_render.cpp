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

#include "glframe_retrace_render.hpp"

#include "glframe_state.hpp"
#include "retrace.hpp"
#include "trace_parser.hpp"

using glretrace::RetraceRender;
using glretrace::StateTrack;

bool changesContext(const trace::Call * const call) {
  if (strncmp(call->name(), "glXMakeCurrent", strlen("glXMakeCurrent")) == 0)
    return true;
  return false;
}

RetraceRender::RetraceRender(trace::AbstractParser *parser,
                             retrace::Retracer *retracer,
                             StateTrack *tracker) : m_parser(parser),
                                                    m_retracer(retracer) {
  m_parser->getBookmark(m_bookmark.start);
  trace::Call *call = NULL;
  while ((call = parser->parse_call())) {
    tracker->flush();
    m_retracer->retrace(*call);
    tracker->track(*call);
    m_end_of_frame = call->flags & trace::CALL_FLAG_END_FRAME;
    bool render = call->flags & trace::CALL_FLAG_RENDER;
    assert(!changesContext(call));
    delete call;

    ++(m_bookmark.numberOfCalls);
    if (render || m_end_of_frame)
      break;
  }
}

void
RetraceRender::retraceRenderTarget() const {
  // check that the parser is in correct state
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_bookmark.start.offset);

  // play up to the end of the render
  for (int calls = 0; calls < m_bookmark.numberOfCalls; ++calls) {
    trace::Call *call = m_parser->parse_call();
    assert(call);

    // TODO(majanes): select the simple shader program if necessary

    m_retracer->retrace(*call);
    delete(call);
  }
}


void
RetraceRender::retrace(StateTrack *tracker) const {
  // check that the parser is in correct state
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_bookmark.start.offset);

  // play up to the end of the render
  for (int calls = 0; calls < m_bookmark.numberOfCalls; ++calls) {
    if (tracker)
      tracker->flush();
    trace::Call *call = m_parser->parse_call();
    assert(call);

    // TODO(majanes): select the simple shader program if necessary

    m_retracer->retrace(*call);
    if (tracker)
      tracker->track(*call);
    delete(call);
  }
}
