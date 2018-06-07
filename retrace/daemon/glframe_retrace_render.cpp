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

#include <map>
#include <string>
#include <sstream>
#include <vector>

#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"
#include "glframe_metrics.hpp"
#include "glframe_state.hpp"
#include "glframe_state_enums.hpp"
#include "glframe_uniforms.hpp"
#include "retrace.hpp"
#include "glstate_internal.hpp"
#include "trace_parser.hpp"
#include "glframe_state_override.hpp"
#include "glframe_texture_override.hpp"

using glretrace::DEBUG;
using glretrace::ExperimentId;
using glretrace::GlFunctions;
using glretrace::PerfMetrics;
using glretrace::RetraceRender;
using glretrace::SelectionId;
using glretrace::StateKey;
using glretrace::StateTrack;
using glretrace::RenderTargetType;
using glretrace::RenderId;
using glretrace::OnFrameRetrace;
using glretrace::state_name_to_enum;
using glretrace::state_enum_to_name;

static const std::string simple_fs =
    "void main(void) {\n"
    "  gl_FragColor = vec4(1,0,1,1);\n"
    "}";

static const std::string overdraw_fs =
    "void main(void) {\n"
    "  gl_FragColor = vec4(1,1,1,1);\n"
    "}";

bool
isCompute(const trace::Call &call) {
  return ((strcmp("glDispatchCompute", call.name()) == 0) ||
          (strcmp("glDispatchComputeIndirect",
                  call.name()) == 0));
}

bool
is_clear(const trace::Call &call) {
  if (strncmp("glClearBuffer", call.name(),
              strlen("glClearBuffer")) == 0)
    return true;
  if (strncmp("glClearNamedFramebuffer", call.name(),
              strlen("glClearNamedFramebuffer")) == 0)
    return true;
  if (strcmp("glClear", call.name()) == 0)
    return true;
  return false;
}

bool
RetraceRender::isRender(const trace::Call &call) {
  return ((call.flags & trace::CALL_FLAG_RENDER) || isCompute(call)
          || is_clear(call));
}

bool
RetraceRender::endsFrame(const trace::Call &c) {
  if (c.flags & trace::CALL_FLAG_END_FRAME) {
    // apitrace considers glFrameTerminatorGREMEDY to be a terminator,
    // but this always follows swapbuffers when it exists.
    if (strncmp("glFrameTerminatorGREMEDY", c.sig->name,
                strlen("glFrameTerminatorGREMEDY")) == 0)
      return false;
    return true;
  }
  return false;
}

int
RetraceRender::currentRenderBuffer() {
  glstate::Context context;
  const GLenum framebuffer_binding = context.ES ?
                                     GL_FRAMEBUFFER_BINDING :
                                     GL_DRAW_FRAMEBUFFER_BINDING;
  GLint draw_framebuffer = 0;
  GlFunctions::GetIntegerv(framebuffer_binding, &draw_framebuffer);
  return draw_framebuffer;
}

// saves, alters, and restores unform settings
class RetraceRender::UniformOverride {
 public:
  void setUniform(const std::string &name, int index,
                  const std::string &data) {
    m_uniform_overrides[UniformKey(name, index)] = data;
  }
  void overrideUniforms() {
    if (m_uniform_overrides.size() == 0)
      return;
    Uniforms modified;
    for (auto i : m_uniform_overrides) {
      modified.overrideUniform(i.first.name,
                               i.first.index,
                               i.second);
    }
    modified.set();
  }
  void restoreUniforms() {
    orig.set();
  }
  void revertExperiments() {
    m_uniform_overrides.clear();
  }

 private:
  struct UniformKey {
    std::string name;
    int index;
    UniformKey(const std::string &n, int i) : name(n), index(i) {}
    bool operator<(const UniformKey &o) const {
      if (name < o.name)
        return true;
      if (name > o.name)
        return false;
      return index < o.index;
    }
  };
  Uniforms orig;
  std::map<UniformKey, std::string> m_uniform_overrides;
};



RetraceRender::RetraceRender(unsigned int tex2x2,
                             trace::AbstractParser *parser,
                             retrace::Retracer *retracer,
                             StateTrack *tracker)
    : m_parser(parser),
      m_retracer(retracer),
      m_rt_program(-1),
      m_overdraw_program(-1),
      m_retrace_program(-1),
      m_end_of_frame(false),
      m_highlight_rt(false),
      m_changes_context(false),
      m_disabled(false),
      m_simple_shader(false),
      m_state_override(new StateOverride()),
      m_highlight_rt_override(new StateOverride()),
      m_geometry_rt_override(new StateOverride()),
      m_overdraw_rt_override(new StateOverride()),
      m_texture_override(new TextureOverride(tex2x2)) {
  m_parser->getBookmark(m_bookmark.start);
  trace::Call *call = NULL;
  std::stringstream call_stream;
  bool compute = false;
  trace::ParseBookmark call_start;
  while ((call = parser->parse_call())) {
    tracker->flush();
    m_retracer->retrace(*call);
    tracker->track(*call);
    m_end_of_frame = endsFrame(*call);
    const bool render = isRender(*call);
    compute = isCompute(*call);
    if (ThreadContext::changesContext(*call)) {
      m_changes_context = true;
      if (m_api_calls.size() > 0) {
        // this ought to be in the next context
        m_parser->setBookmark(call_start);
        delete call;
        break;
      }
    }
    trace::dump(*call, call_stream,
                trace::DUMP_FLAG_NO_COLOR);
    m_api_calls.push_back(call_stream.str());
    call_stream.str("");
    delete call;

    ++(m_bookmark.numberOfCalls);
    if (render || m_end_of_frame)
      break;
    m_parser->getBookmark(call_start);
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
    // generate the highlight and overdraw rt programs, for later use
    m_rt_program = tracker->useProgram(m_original_program,
                                       m_original_vs,
                                       simple_fs,
                                       m_original_tess_control,
                                       m_original_tess_eval,
                                       m_original_geom,
                                       m_original_comp);
    m_overdraw_program = tracker->useProgram(m_original_program,
                                             m_original_vs,
                                             overdraw_fs,
                                             m_original_tess_control,
                                             m_original_tess_eval,
                                             m_original_geom,
                                             m_original_comp);
    tracker->useProgram(m_original_program);
  }

  // GL state must be in-flight for uniforms to be correctly queried
  // in the constructor
  m_uniform_override = new UniformOverride();

  // configure highlight override for render targets
  m_highlight_rt_override->setState(StateKey("Fragment", "GL_BLEND"),
                                    0, "false");

  // configure wireframe override for render targets
  const StateKey wireframe_key("Primitive/Polygon", "GL_POLYGON_MODE");
  const StateKey width("Primitive/Line", "GL_LINE_WIDTH");
  const StateKey depth("Fragment/Depth", "GL_DEPTH_TEST");
  m_geometry_rt_override->setState(wireframe_key, 0, "GL_LINE");
  m_geometry_rt_override->setState(wireframe_key, 1, "GL_LINE");
  m_geometry_rt_override->setState(width, 0, "1.5");
  m_geometry_rt_override->setState(depth, 0, "false");
  m_geometry_rt_override->setState(StateKey("Fragment", "GL_BLEND"),
                                   0, "false");

  // configure the overdraw override for render targets
  const StateKey blend_color("Fragment", "GL_BLEND_COLOR");
  m_overdraw_rt_override->setState(blend_color, 0, "0.15");
  m_overdraw_rt_override->setState(blend_color, 1, "0.15");
  m_overdraw_rt_override->setState(blend_color, 2, "0.15");
  m_overdraw_rt_override->setState(blend_color, 3, "0.0");
  m_overdraw_rt_override->setState(StateKey("Fragment", "GL_BLEND"), 0, "true");
  m_overdraw_rt_override->setState(StateKey("Fragment", "GL_BLEND_DST"),
                                   0, "GL_ONE");
  m_overdraw_rt_override->setState(StateKey("Fragment",
                                            "GL_BLEND_EQUATION_ALPHA"),
                                   0, "GL_MAX");
  m_overdraw_rt_override->setState(StateKey("Fragment",
                                            "GL_BLEND_EQUATION_RGB"),
                                   0, "GL_FUNC_ADD");
  m_overdraw_rt_override->setState(StateKey("Fragment", "GL_BLEND_SRC_RGB"),
                                   0, "GL_CONSTANT_COLOR");
  m_overdraw_rt_override->setState(StateKey("Fragment", "GL_BLEND_SRC_ALPHA"),
                                   0, "GL_ONE");
}

RetraceRender::~RetraceRender() {
  delete m_uniform_override;
  delete m_state_override;
  delete m_highlight_rt_override;
  delete m_geometry_rt_override;
  delete m_overdraw_rt_override;
  delete m_texture_override;
}

void
RetraceRender::retraceRenderTarget(const StateTrack &tracker,
                                   RenderTargetType type) const {
  // check that the parser is in correct state
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_bookmark.start.offset);

  // play up to but not past the end of the render
  for (unsigned int calls = 0; calls < m_bookmark.numberOfCalls - 1; ++calls) {
    trace::Call *call = m_parser->parse_call();
    assert(call);
    tracker.retraceProgramSideEffects(m_original_program, call, m_retracer);
    m_retracer->retrace(*call);
    delete(call);
  }

  if ((m_simple_shader ||
       type == HIGHLIGHT_RENDER ||
       type == GEOMETRY_RENDER) &&
      (m_rt_program > -1)) {
    StateTrack::useProgramGL(m_rt_program);
  } else if ((type == OVERDRAW_RENDER) &&
             m_overdraw_program > -1) {
    StateTrack::useProgramGL(m_overdraw_program);
  } else if (m_retrace_program > -1) {
    StateTrack::useProgramGL(m_retrace_program);
  }

  m_uniform_override->overrideUniforms();
  m_state_override->overrideState();
  m_texture_override->overrideTexture();

  if (m_simple_shader ||
      type == HIGHLIGHT_RENDER)
    m_highlight_rt_override->overrideState();
  else if (type == GEOMETRY_RENDER)
    m_geometry_rt_override->overrideState();
  else if (type == OVERDRAW_RENDER)
    m_overdraw_rt_override->overrideState();

  // retrace the final render
  trace::Call *call = m_parser->parse_call();
  assert(call);
  if ((type != NULL_RENDER) &&
      (!m_disabled) &&
      // do not retrace swap buffers: the gpu cost is variable
      (!endsFrame(*call)))
    m_retracer->retrace(*call);
  delete(call);

  m_highlight_rt_override->restoreState();
  m_geometry_rt_override->restoreState();
  m_overdraw_rt_override->restoreState();

  m_uniform_override->restoreUniforms();
  m_state_override->restoreState();
  m_texture_override->restoreTexture();

  StateTrack::useProgramGL(m_original_program);
}


void
RetraceRender::retrace(StateTrack *tracker) const {
  // check that the parser is in correct state
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_bookmark.start.offset);

  tracker->flush();

  // play up to but not past the end of the render
  for (unsigned int calls = 0; calls < m_bookmark.numberOfCalls - 1; ++calls) {
    trace::Call *call = m_parser->parse_call();
    assert(call);

    tracker->retraceProgramSideEffects(m_original_program, call, m_retracer);

    m_retracer->retrace(*call);
    tracker->track(*call);
    delete(call);
  }

  // select the shader override if necessary
  if (m_simple_shader) {
    StateTrack::useProgramGL(m_rt_program);
    tracker->useProgram(m_rt_program);
  } else if (m_retrace_program > -1) {
    StateTrack::useProgramGL(m_retrace_program);
    if (tracker) {
      tracker->useProgram(m_retrace_program);
    }
  }

  // retrace the final render
  trace::Call *call = m_parser->parse_call();
  assert(call);
  if (!m_disabled)
    m_retracer->retrace(*call);
  if (tracker)
    tracker->track(*call);
  delete(call);
  StateTrack::useProgramGL(m_original_program);
}

void
RetraceRender::retrace(const StateTrack &tracker,
                       const CallbackContext *uniform_context,
                       const CallbackContext *state_context) const {
  // check that the parser is in correct state
  trace::ParseBookmark bm;
  m_parser->getBookmark(bm);
  assert(bm.offset == m_bookmark.start.offset);

  // play up to but not past the end of the render
  for (unsigned int calls = 0; calls < m_bookmark.numberOfCalls - 1; ++calls) {
    trace::Call *call = m_parser->parse_call();
    assert(call);

    tracker.retraceProgramSideEffects(m_original_program, call, m_retracer);

    // context change must be on the first call of the render
    assert((!ThreadContext::changesContext(*call)) || calls == 0);
    m_retracer->retrace(*call);

    delete(call);
  }

  // select the shader override if necessary
  if (m_simple_shader) {
    StateTrack::useProgramGL(m_rt_program);
  } else if (m_retrace_program > -1) {
    StateTrack::useProgramGL(m_retrace_program);
  }

  m_uniform_override->overrideUniforms();
  m_state_override->overrideState();
  m_texture_override->overrideTexture();

  // retrace the final render
  trace::Call *call = m_parser->parse_call();
  assert(call);
  if ((!m_disabled) &&
      // do not retrace swap buffers: the gpu cost is variable
      (!endsFrame(*call)))
    m_retracer->retrace(*call);
  delete(call);

  if (uniform_context) {
    Uniforms u;
    u.onUniform(uniform_context->selection, uniform_context->experiment,
                uniform_context->render, uniform_context->callback);
  }

  if (state_context) {
    onState(state_context->selection, state_context->experiment,
            state_context->render, state_context->callback);
  }

  m_state_override->restoreState();
  m_uniform_override->restoreUniforms();
  m_texture_override->restoreTexture();

  StateTrack::useProgramGL(m_original_program);
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
  m_overdraw_program = tracker->useProgram(m_original_program,
                                     vs, overdraw_fs,
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

void
RetraceRender::disableDraw(bool disable) {
  m_disabled = disable;
}

void
RetraceRender::simpleShader(bool simple) {
  m_simple_shader = simple;
}

void
RetraceRender::setUniform(const std::string &name, int index,
                          const std::string &data) {
  m_uniform_override->setUniform(name, index, data);
}

void
RetraceRender::onState(SelectionId selId,
                       ExperimentId experimentCount,
                       RenderId renderId,
                       OnFrameRetrace *callback) const {
  m_state_override->onState(selId, experimentCount, renderId, callback);
}

void
RetraceRender::setState(const StateKey &item,
                        int offset,
                        const std::string &value) {
  m_state_override->setState(item, offset, value);
}

void
RetraceRender::revertState(const StateKey &item) {
  m_state_override->revertState(item);
}

void
RetraceRender::revertExperiments(StateTrack *tracker) {
  m_modified_fs = "";
  m_modified_vs = "";
  m_modified_tess_eval = "";
  m_modified_tess_control = "";
  m_modified_geom = "";
  m_modified_comp = "";
  m_retrace_program = -1;
  m_disabled = false;
  m_simple_shader = false;
  m_uniform_override->revertExperiments();
  m_state_override->revertExperiments();
  m_texture_override->revertExperiments();
  if (m_rt_program > -1)
    // set render target program back to default
    m_rt_program = tracker->useProgram(m_original_program,
                                       m_original_vs,
                                       simple_fs,
                                       m_original_tess_control,
                                       m_original_tess_eval,
                                       m_original_geom,
                                       m_original_comp);
  if (m_overdraw_program > -1)
    // set render target program back to default
    m_overdraw_program = tracker->useProgram(m_original_program,
                                             m_original_vs,
                                             overdraw_fs,
                                             m_original_tess_control,
                                             m_original_tess_eval,
                                             m_original_geom,
                                             m_original_comp);
}

void
RetraceRender::texture2x2(bool enable) {
  m_texture_override->set2x2(enable);
}
