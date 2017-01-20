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

#include "glframe_retrace_render.hpp"

#include <string>
#include <sstream>
#include <vector>

#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"
#include "glframe_state.hpp"
#include "retrace.hpp"
#include "trace_parser.hpp"

using glretrace::DEBUG;
using glretrace::GlFunctions;
using glretrace::RetraceRender;
using glretrace::SelectionId;
using glretrace::StateTrack;
using glretrace::RenderTargetType;
using glretrace::RenderId;
using glretrace::OnFrameRetrace;

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
                                                    m_rt_program(-1),
                                                    m_retrace_program(-1),
                                                    m_end_of_frame(false),
                                                    m_highlight_rt(false) {
  m_parser->getBookmark(m_bookmark.start);
  trace::Call *call = NULL;
  std::stringstream call_stream;
  bool compute = false;
  while ((call = parser->parse_call())) {
    trace::dump(*call, call_stream,
                trace::DUMP_FLAG_NO_COLOR);
    m_api_calls.push_back(call_stream.str());
    call_stream.str("");

    tracker->flush();
    m_retracer->retrace(*call);
    tracker->track(*call);
    m_end_of_frame = call->flags & trace::CALL_FLAG_END_FRAME;
    const bool render = (call->flags & trace::CALL_FLAG_RENDER) ||
                        (strcmp("glDispatchCompute", call->name()) == 0) ||
                        (strcmp("glDispatchComputeIndirect",
                                call->name()) == 0);
    compute = render && (!(call->flags & trace::CALL_FLAG_RENDER));
    assert(!changesContext(call));
    delete call;

    ++(m_bookmark.numberOfCalls);
    if (render || m_end_of_frame)
      break;
  }
  m_original_program = tracker->CurrentProgram();
  m_original_vs = tracker->currentVertexShader().shader;
  m_modified_vs = m_original_vs;
  m_original_fs = tracker->currentFragmentShader().shader;
  m_modified_fs = m_original_fs;
  m_original_tess_control = tracker->currentTessControlShader().shader;
  m_modified_tess_control = m_original_tess_control;
  m_original_tess_eval = tracker->currentTessEvalShader().shader;
  m_modified_tess_eval = m_original_tess_eval;
  m_original_geom = tracker->currentGeomShader().shader;
  m_modified_geom = m_original_geom;
  m_original_comp = tracker->currentCompShader().shader;
  m_modified_comp = m_original_comp;

  if (!compute) {
    // generate the highlight rt program, for later use
    m_rt_program = tracker->useProgram(m_original_program,
                                       m_modified_vs,
                                       simple_fs,
                                       m_modified_tess_eval,
                                       m_modified_tess_control,
                                       m_original_geom,
                                       m_original_comp);
    tracker->useProgram(m_original_program);
  }
}

void
RetraceRender::retraceRenderTarget(const StateTrack &tracker,
                                   RenderTargetType type) const {
  // check that the parser is in correct state
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_bookmark.start.offset);

  // play up to but not past the end of the render
  for (int calls = 0; calls < m_bookmark.numberOfCalls - 1; ++calls) {
    trace::Call *call = m_parser->parse_call();
    assert(call);
    tracker.retraceProgramSideEffects(m_original_program, call, m_retracer);
    m_retracer->retrace(*call);
    delete(call);
  }

  bool blend_enabled = false;
  if ((type == HIGHLIGHT_RENDER) && (m_rt_program > -1)) {
    blend_enabled = GlFunctions::IsEnabled(GL_BLEND);
    GlFunctions::Disable(GL_BLEND);
    GlFunctions::UseProgram(m_rt_program);
    GlFunctions::ValidateProgram(m_rt_program);
    GLint result;
    GlFunctions::GetProgramiv(m_rt_program, GL_VALIDATE_STATUS, &result);
    if (result == GL_FALSE) {
      std::vector<char> buf(1024);
      GLsizei s;
      GlFunctions::GetProgramInfoLog(m_rt_program, 1024, &s, buf.data());
      GRLOGF(ERR, "Highlight program not validated: %s", buf.data());
    }
  } else if (m_retrace_program > -1) {
    GlFunctions::UseProgram(m_retrace_program);
  }

  // retrace the final render
  trace::Call *call = m_parser->parse_call();
  assert(call);
  m_retracer->retrace(*call);
  delete(call);
  if (type == HIGHLIGHT_RENDER || m_retrace_program > -1) {
    if (blend_enabled)
      GlFunctions::Enable(GL_BLEND);
    GlFunctions::UseProgram(m_original_program);
  }
}


void
RetraceRender::retrace(StateTrack *tracker) const {
  // check that the parser is in correct state
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_bookmark.start.offset);

  tracker->flush();

  // play up to but not past the end of the render
  for (int calls = 0; calls < m_bookmark.numberOfCalls - 1; ++calls) {
    trace::Call *call = m_parser->parse_call();
    assert(call);

    tracker->retraceProgramSideEffects(m_original_program, call, m_retracer);

    m_retracer->retrace(*call);
    tracker->track(*call);
    delete(call);
  }

  // select the shader override if necessary
  if (m_retrace_program > -1) {
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
  if (m_retrace_program)
    GlFunctions::UseProgram(m_original_program);
}

void
RetraceRender::retrace(const StateTrack &tracker) const {
  // check that the parser is in correct state
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_bookmark.start.offset);

  // play up to but not past the end of the render
  for (int calls = 0; calls < m_bookmark.numberOfCalls - 1; ++calls) {
    trace::Call *call = m_parser->parse_call();
    assert(call);

    tracker.retraceProgramSideEffects(m_original_program, call, m_retracer);

    m_retracer->retrace(*call);

    delete(call);
  }

  // select the shader override if necessary
  if (m_retrace_program > -1) {
    GlFunctions::UseProgram(m_retrace_program);
  }

  // retrace the final render
  trace::Call *call = m_parser->parse_call();
  assert(call);
  m_retracer->retrace(*call);
  delete(call);
  if (m_retrace_program)
    GlFunctions::UseProgram(m_original_program);
}


bool
RetraceRender::replaceShaders(StateTrack *tracker,
                              const std::string &vs,
                              const std::string &fs,
                              const std::string &tessControl,
                              const std::string &tessEval,
                              const std::string &geom,
                              const std::string &comp,
                              std::string *message) {
  GRLOGF(DEBUG, "RetraceRender: %s \n %s \n %s \n %s", vs.c_str(), fs.c_str(),
         tessControl.c_str(), tessEval.c_str());
  const int result = tracker->useProgram(m_original_program,
                                         vs, fs,
                                         tessControl, tessEval,
                                         geom, comp, message);
  if (result == -1)
    return false;

  // else
  m_modified_vs = vs;
  m_modified_fs = fs;
  m_modified_tess_control = tessControl;
  m_modified_tess_eval = tessEval;
  m_modified_geom = geom;
  m_modified_comp = comp;
  m_retrace_program = result;
  *message = "";
  m_rt_program = tracker->useProgram(m_original_program,
                                     vs, simple_fs,
                                     tessControl, tessEval,
                                     geom, comp, message);
  tracker->useProgram(result);
  return true;
}

void
RetraceRender::onApi(SelectionId selId,
                     RenderId renderId,
                     OnFrameRetrace *callback) {
  callback->onApi(selId, renderId, m_api_calls);
}
