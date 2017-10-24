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

using glretrace::DEBUG;
using glretrace::GlFunctions;
using glretrace::PerfMetrics;
using glretrace::RetraceRender;
using glretrace::SelectionId;
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

bool
isCompute(const trace::Call &call) {
  return ((strcmp("glDispatchCompute", call.name()) == 0) ||
          (strcmp("glDispatchComputeIndirect",
                  call.name()) == 0));
}

bool
isClear(const trace::Call &call) {
  return (strncmp("glClearBuffer", call.name(), strlen("glClearBuffer")) == 0);
}

bool
RetraceRender::changesContext(const trace::Call &call) {
  if (strncmp(call.name(), "glXMakeCurrent", strlen("glXMakeCurrent")) == 0)
    return true;
  if (strncmp(call.name(), "glXMakeContextCurrent",
              strlen("glXMakeContextCurrent")) == 0)
    return true;
  return false;
}

bool
RetraceRender::isRender(const trace::Call &call) {
  return ((call.flags & trace::CALL_FLAG_RENDER) || isCompute(call)
          || isClear(call));
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

class RetraceRender::StateOverride {
 public:
  StateOverride() {}
  void setState(const StateKey &item,
                const std::string &value) {
    m_overrides[item] = state_name_to_enum(value);
  }
  void saveState();
  void overrideState() const;
  void restoreState() const;

 private:
  struct Key {
    uint32_t item;
    uint32_t offset;
    Key(uint32_t i, uint32_t o) : item(i), offset(o) {}
    bool operator<(const Key &o) const {
      if (item < o.item)
        return true;
      if (item > o.item)
        return false;
      return offset < o.offset;
    }
  };
  typedef std::map<StateKey, uint32_t> KeyMap;
  void enact_state(const KeyMap &m) const;
  KeyMap m_overrides;
  KeyMap m_saved_state;
};

void
RetraceRender::StateOverride::saveState() {
  for (auto i : m_overrides) {
    if (m_saved_state.find(i.first) != m_saved_state.end())
      // we have already saved the state
      continue;
    switch (glretrace::state_name_to_enum(i.first.name)) {
      case GL_CULL_FACE:
        assert(GL::GetError() == GL_NO_ERROR);
        m_saved_state[i.first] = GlFunctions::IsEnabled(GL_CULL_FACE);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      case GL_CULL_FACE_MODE: {
        assert(GL::GetError() == GL_NO_ERROR);
        GLint cull;
        GlFunctions::GetIntegerv(GL_CULL_FACE_MODE, &cull);
        assert(GL::GetError() == GL_NO_ERROR);
        m_saved_state[i.first] = cull;
        break;
      }
      case GL_BLEND:
        m_saved_state[i.first] = GlFunctions::IsEnabled(GL_BLEND);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      case GL_BLEND_SRC: {
        GLint s;
        GlFunctions::GetIntegerv(GL_BLEND_SRC, &s);
        assert(GL::GetError() == GL_NO_ERROR);
        m_saved_state[i.first] = s;
        break;
      }
      case GL_BLEND_DST: {
        GLint s;
        GlFunctions::GetIntegerv(GL_BLEND_DST, &s);
        assert(GL::GetError() == GL_NO_ERROR);
        m_saved_state[i.first] = s;
        break;
      }
      case GL_INVALID_ENUM:
      default:
        assert(false);
        break;
    }
  }
}

void
RetraceRender::StateOverride::overrideState() const {
  enact_state(m_overrides);
}

void
RetraceRender::StateOverride::restoreState() const {
  enact_state(m_saved_state);
}

void
RetraceRender::StateOverride::enact_state(const KeyMap &m) const {
  GL::GetError();
  for (auto i : m) {
    switch (glretrace::state_name_to_enum(i.first.name)) {
      case GL_CULL_FACE: {
        if (i.second)
          GlFunctions::Enable(GL_CULL_FACE);
        else
          GlFunctions::Disable(GL_CULL_FACE);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_CULL_FACE_MODE: {
        GlFunctions::CullFace(i.second);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_BLEND: {
        if (i.second)
          GlFunctions::Enable(GL_BLEND);
        else
          GlFunctions::Disable(GL_BLEND);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_BLEND_SRC: {
        GLint dst;
        GlFunctions::GetIntegerv(GL_BLEND_DST, &dst);
        assert(GL::GetError() == GL_NO_ERROR);
        GlFunctions::BlendFunc(i.second, dst);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_BLEND_DST: {
        GLint src;
        GlFunctions::GetIntegerv(GL_BLEND_DST, &src);
        assert(GL::GetError() == GL_NO_ERROR);
        GlFunctions::BlendFunc(src, i.second);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_INVALID_ENUM:
      default:
        assert(false);
        break;
    }
  }
}

RetraceRender::RetraceRender(trace::AbstractParser *parser,
                             retrace::Retracer *retracer,
                             StateTrack *tracker)
    : m_parser(parser),
      m_retracer(retracer),
      m_rt_program(-1),
      m_retrace_program(-1),
      m_end_of_frame(false),
      m_highlight_rt(false),
      m_changes_context(false),
      m_disabled(false),
      m_simple_shader(false),
      m_state_override(new StateOverride()) {
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
    if (changesContext(*call)) {
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
  m_uniform_override = new UniformOverride();
}

RetraceRender::~RetraceRender() {
  delete m_uniform_override;
  delete m_state_override;
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

  bool blend_enabled = false;
  if ((m_simple_shader || (type == HIGHLIGHT_RENDER)) && (m_rt_program > -1)) {
    blend_enabled = GlFunctions::IsEnabled(GL_BLEND);
    GlFunctions::Disable(GL_BLEND);
    StateTrack::useProgramGL(m_rt_program);
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
    StateTrack::useProgramGL(m_retrace_program);
  }

  m_uniform_override->overrideUniforms();
  m_state_override->saveState();
  m_state_override->overrideState();

  // retrace the final render
  trace::Call *call = m_parser->parse_call();
  assert(call);
  if (!m_disabled)
    m_retracer->retrace(*call);
  delete(call);

  m_uniform_override->restoreUniforms();
  m_state_override->restoreState();

  if (blend_enabled)
    GlFunctions::Enable(GL_BLEND);

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
    assert((!changesContext(*call)) || calls == 0);
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
  m_state_override->saveState();
  m_state_override->overrideState();

  // retrace the final render
  trace::Call *call = m_parser->parse_call();
  assert(call);
  if (!m_disabled)
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
  {
    GL::GetError();
    GLboolean cull_enabled = GlFunctions::IsEnabled(GL_CULL_FACE);
    GLenum e = GL::GetError();
    if (e == GL_NO_ERROR) {
      callback->onState(selId, experimentCount, renderId,
                        StateKey("Rendering", "Cull State", "GL_CULL_FACE"),
                        cull_enabled ? "true" : "false");
    }
  }
  {
    GLint cull;
    GlFunctions::GetIntegerv(GL_CULL_FACE_MODE, &cull);
    GLenum e = GL::GetError();
    if (e == GL_NO_ERROR) {
      const std::string cull_str = state_enum_to_name(cull);
      if (cull_str.size() > 0) {
        callback->onState(selId, experimentCount, renderId,
                          StateKey("Rendering", "Cull State",
                                   "GL_CULL_FACE_MODE"), cull_str);
      }
    }
  }
  {
    GL::GetError();
    GLboolean enabled = GlFunctions::IsEnabled(GL_BLEND);
    GLenum e = GL::GetError();
    if (e == GL_NO_ERROR) {
      callback->onState(selId, experimentCount, renderId,
                        StateKey("Rendering", "Blend State", "GL_BLEND"),
                        enabled ? "true" : "false");
    }
  }
  {
    GLint s;
    GlFunctions::GetIntegerv(GL_BLEND_SRC, &s);
    GLenum e = GL::GetError();
    if (e == GL_NO_ERROR) {
      const std::string state_str = state_enum_to_name(s);
      if (state_str.size() > 0) {
        callback->onState(selId, experimentCount, renderId,
                          StateKey("Rendering", "Blend State",
                                   "GL_BLEND_SRC"), state_str);
      }
    }
  }
  {
    GLint s;
    GlFunctions::GetIntegerv(GL_BLEND_DST, &s);
    GLenum e = GL::GetError();
    if (e == GL_NO_ERROR) {
      const std::string state_str = state_enum_to_name(s);
      if (state_str.size() > 0) {
        callback->onState(selId, experimentCount, renderId,
                          StateKey("Rendering", "Blend State",
                                   "GL_BLEND_DST"), state_str);
      }
    }
  }
}

void
RetraceRender::setState(const StateKey &item,
                        const std::string &value) {
  m_state_override->setState(item, value);
}
