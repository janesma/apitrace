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

#include <string>

#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"
#include "glframe_state.hpp"
#include "retrace.hpp"
#include "trace_parser.hpp"

using glretrace::DEBUG;
using glretrace::GlFunctions;
using glretrace::RetraceRender;
using glretrace::StateTrack;
using glretrace::RenderTargetType;

bool changesContext(const trace::Call * const call) {
  if (strncmp(call->name(), "glXMakeCurrent", strlen("glXMakeCurrent")) == 0)
    return true;
  return false;
}

static const std::string simple_fs =
    "void main(void) {\n"
    "  gl_FragColor = vec4(1,0,1,1);\n"
    "}";

RetraceRender::RetraceRender(trace::AbstractParser *parser,
                             retrace::Retracer *retracer,
                             StateTrack *tracker) : m_parser(parser),
                                                    m_retracer(retracer),
                                                    m_rt_program(0),
                                                    m_retrace_program(0),
                                                    m_end_of_frame(false),
                                                    m_highlight_rt(false) {
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
  const int p = tracker->CurrentProgram();
  m_original_vs = tracker->currentVertexShader();
  m_modified_vs = m_original_vs;
  m_original_fs = tracker->currentFragmentShader();
  m_modified_fs = m_original_fs;

  // generate the highlight rt program, for later use
  m_rt_program = tracker->useProgram(m_modified_vs,
                                     simple_fs);
  tracker->useProgram(p);
}

void
RetraceRender::retraceRenderTarget(RenderTargetType type) const {
  // check that the parser is in correct state
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_bookmark.start.offset);

  // play up to but not past the end of the render
  for (int calls = 0; calls < m_bookmark.numberOfCalls - 1; ++calls) {
    trace::Call *call = m_parser->parse_call();
    assert(call);
    m_retracer->retrace(*call);
    delete(call);
  }

  if (type == HIGHLIGHT_RENDER)
    GlFunctions::UseProgram(m_rt_program);
  else if (m_retrace_program)
    GlFunctions::UseProgram(m_retrace_program);

  // retrace the final render
  trace::Call *call = m_parser->parse_call();
  assert(call);
  m_retracer->retrace(*call);
  delete(call);
}


void
RetraceRender::retrace(StateTrack *tracker) const {
  // check that the parser is in correct state
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_bookmark.start.offset);

  if (tracker)
    tracker->flush();

  // play up to but not past the end of the render
  for (int calls = 0; calls < m_bookmark.numberOfCalls - 1; ++calls) {
    trace::Call *call = m_parser->parse_call();
    assert(call);
    m_retracer->retrace(*call);
    if (tracker)
      tracker->track(*call);
    delete(call);
  }

  // select the shader override if necessary
  if (m_retrace_program) {
    GlFunctions::UseProgram(m_retrace_program);
    if (tracker) {
      tracker->useProgram(m_retrace_program);
    }
  }

  // retrace the final render
  trace::Call *call = m_parser->parse_call();
  assert(call);
  m_retracer->retrace(*call);
  if (tracker)
    tracker->track(*call);
  delete(call);
}


bool
RetraceRender::replaceShaders(StateTrack *tracker,
                              const std::string &vs,
                              const std::string &fs,
                              std::string *message) {
  GRLOGF(DEBUG, "RetraceRender: %s \n %s", vs.c_str(), fs.c_str());
  const int result = tracker->useProgram(vs, fs, message);
  if (result == -1)
    return false;

  // else
  m_modified_vs = vs;
  m_modified_fs = fs;
  m_retrace_program = result;
  *message = "";
  m_rt_program = tracker->useProgram(vs, simple_fs, message);
  tracker->useProgram(result);
  return true;
}
