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

#include "glframe_state.hpp"

#include <string.h>

#include <map>
#include <sstream>
#include <string>

#include "GL/gl.h"
#include "GL/glext.h"
#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"
#include "glframe_retrace_interface.hpp"
#include "glframe_uniforms.hpp"
#include "glretrace.hpp"
#include "retrace.hpp"
#include "trace_model.hpp"

using glretrace::StateTrack;
using glretrace::WARN;
using glretrace::OutputPoller;
using glretrace::OnFrameRetrace;
using glretrace::ShaderAssembly;
using glretrace::ShaderType;
using glretrace::AssemblyType;
using glretrace::Uniforms;
using trace::Call;
using trace::Array;

static std::map<std::string, bool> ignore_strings;

StateTrack::TrackMap StateTrack::lookup;

StateTrack::StateTrack(OutputPoller *p)
    : m_poller(p),
      current_program(0),
      empty_shader() {
}

StateTrack::TrackMap::TrackMap() {
  lookup["glAttachShader"] = &StateTrack::trackAttachShader;
  lookup["glCreateShader"] = &StateTrack::trackCreateShader;
  lookup["glLinkProgram"] = &StateTrack::trackLinkProgram;
  lookup["glShaderSource"] = &StateTrack::trackShaderSource;
  lookup["glUseProgram"] = &StateTrack::trackUseProgram;
  lookup["glDeleteProgram"] = &StateTrack::trackDeleteProgram;
  lookup["glBindAttribLocation"] = &StateTrack::trackBindAttribLocation;
  lookup["glGetAttribLocation"] = &StateTrack::trackGetAttribLocation;
  lookup["glGetUniformLocation"] = &StateTrack::trackGetUniformLocation;
  lookup["glGetUniformBlockIndex"] = &StateTrack::trackGetUniformBlockIndex;
  lookup["glUniformBlockBinding"] = &StateTrack::trackUniformBlockBinding;
  lookup["glBindFragDataLocation"] = &StateTrack::trackBindFragDataLocation;
  lookup["glBindProgramPipeline"] = &StateTrack::trackBindProgramPipeline;
  lookup["glUseProgramStages"] = &StateTrack::trackUseProgramStages;
}

bool
StateTrack::TrackMap::track(StateTrack *tracker, const Call &call) {
  auto resolve = lookup.find(call.sig->name);
  if (resolve == lookup.end())
    return false;
  MemberFunType funptr = resolve->second;
  (tracker->*funptr)(call);
  return true;
}

// TODO(majanes): use a lookup table
void
StateTrack::track(const Call &call) {
  if (lookup.track(this, call)) {
    std::stringstream call_stream;
    trace::dump(const_cast<Call&>(call), call_stream,
                trace::DUMP_FLAG_NO_COLOR);
    GRLOG(glretrace::DEBUG, call_stream.str().c_str());
#ifdef WIN32
    // windows parsing of shader assemblies is extremely slow
    parse();
#endif
  }

#ifndef WIN32
  // on Linux we can parse for shader assembly data on every call.
  parse();
#endif
}

void
StateTrack::trackAttachShader(const Call &call) {
  const int call_program = call.args[0].value->toDouble();
  const int program = glretrace::getRetracedProgram(call_program);
  const int traced_shader = call.args[1].value->toDouble();
  const int shader = glretrace::getRetracedShader(traced_shader);
  if (shader_to_type[shader] == GL_FRAGMENT_SHADER) {
    program_to_fragment[program].shader = shader_to_source[shader];
    fragment_to_program[shader] = program;
  } else if (shader_to_type[shader] == GL_VERTEX_SHADER) {
    program_to_vertex[program].shader = shader_to_source[shader];
    vertex_to_program[shader] = program;
  } else if (shader_to_type[shader] == GL_TESS_CONTROL_SHADER) {
    program_to_tess_control[program].shader = shader_to_source[shader];
    tess_control_to_program[shader] = program;
  } else if (shader_to_type[shader] == GL_TESS_EVALUATION_SHADER) {
    program_to_tess_eval[program].shader = shader_to_source[shader];
    tess_eval_to_program[shader] = program;
  } else if (shader_to_type[shader] == GL_GEOMETRY_SHADER) {
    program_to_geom[program].shader = shader_to_source[shader];
    geom_to_program[shader] = program;
  } else if (shader_to_type[shader] == GL_COMPUTE_SHADER) {
    program_to_comp[program].shader = shader_to_source[shader];
    comp_to_program[shader] = program;
  }

  auto vs = program_to_vertex.find(program);
  auto fs = program_to_fragment.find(program);
  auto tess_control = program_to_tess_control.find(program);
  auto tess_eval = program_to_tess_eval.find(program);
  auto geom = program_to_geom.find(program);
  auto comp = program_to_comp.find(program);
  const ProgramKey k(program,
                     vs == program_to_vertex.end()
                     ? "" : vs->second.shader,
                     fs == program_to_fragment.end()
                     ? "" : fs->second.shader,
                     tess_control == program_to_tess_control.end()
                     ? "" : tess_control->second.shader,
                     tess_eval == program_to_tess_eval.end()
                     ? "" : tess_eval->second.shader,
                     geom == program_to_geom.end()
                     ? "" : geom->second.shader,
                     comp == program_to_comp.end()
                     ? "" : comp->second.shader);
  m_sources_to_program[k] = program;
}

void
StateTrack::trackCreateShader(const Call &call) {
  const int shader_type = call.args[0].value->toDouble();
  const int shader = glretrace::getRetracedShader(call.ret->toDouble());
  shader_to_type[shader] = shader_type;
}

void
StateTrack::trackShaderSource(const Call &call) {
  const int traced_shader = call.args[0].value->toDouble();
  const int shader = glretrace::getRetracedShader(traced_shader);
  const Array * source = call.args[2].value->toArray();
  std::string text;
  for (auto line : source->values) {
    text += line->toString();
  }
  shader_to_source[shader] = text;
  source_to_shader[text] = shader;
  if (shader_to_type[shader] == GL_VERTEX_SHADER) {
    if (vertex_to_program.find(shader) != vertex_to_program.end())
      program_to_vertex[vertex_to_program[shader]].shader = text;
  } else if (shader_to_type[shader] == GL_FRAGMENT_SHADER) {
    if (fragment_to_program.find(shader) != fragment_to_program.end())
      program_to_fragment[fragment_to_program[shader]].shader = text;
  } else if (shader_to_type[shader] == GL_COMPUTE_SHADER) {
    if (comp_to_program.find(shader) != comp_to_program.end())
      program_to_comp[comp_to_program[shader]].shader = text;
  } else if (shader_to_type[shader] == GL_TESS_CONTROL_SHADER) {
    if (tess_control_to_program.find(shader) != tess_control_to_program.end())
      program_to_tess_control[tess_control_to_program[shader]].shader = text;
  } else if (shader_to_type[shader] == GL_TESS_EVALUATION_SHADER) {
    if (tess_eval_to_program.find(shader) != tess_eval_to_program.end())
      program_to_tess_eval[tess_eval_to_program[shader]].shader = text;
  } else if (shader_to_type[shader] == GL_GEOMETRY_SHADER) {
    if (geom_to_program.find(shader) != geom_to_program.end())
      program_to_geom[geom_to_program[shader]].shader = text;
  }
}

void
StateTrack::trackLinkProgram(const trace::Call &call) {
  current_program = getRetracedProgram(call.args[0].value->toDouble());
}

void
StateTrack::trackUseProgram(const trace::Call &call) {
  // the program in use will not be the id in the Call, but rather the
  // program id generated during retrace, which is mapped within
  // glretrace_gl.cpp.  To track the actual program, we must retrieve
  // it from apitrace.
  int call_program = call.args[0].value->toDouble();
  if (!call_program)
    current_program = 0;
  else
    current_program = getRetracedProgram(call_program);
}

void
StateTrack::trackDeleteProgram(const trace::Call &call) {
  const int deleted_program =
      getRetracedProgram(call.args[0].value->toDouble());
  {
    auto i = program_to_vertex.find(deleted_program);
    if (i != program_to_vertex.end())
      program_to_vertex.erase(i);
    i = program_to_fragment.find(deleted_program);
    if (i != program_to_fragment.end())
      program_to_fragment.erase(i);
    i = program_to_tess_control.find(deleted_program);
    if (i != program_to_tess_control.end())
      program_to_tess_control.erase(i);
    i = program_to_tess_eval.find(deleted_program);
    if (i != program_to_tess_eval.end())
      program_to_tess_eval.erase(i);
    i = program_to_geom.find(deleted_program);
    if (i != program_to_geom.end())
      program_to_geom.erase(i);
    i = program_to_comp.find(deleted_program);
    if (i != program_to_comp.end())
      program_to_comp.erase(i);
  }
  {
    auto i = m_program_to_bound_attrib.find(deleted_program);
    if (i != m_program_to_bound_attrib.end())
      m_program_to_bound_attrib.erase(i);
    i = m_program_to_uniform_name.find(deleted_program);
    if (i != m_program_to_uniform_name.end())
      m_program_to_uniform_name.erase(i);
  }
  {
    auto i = m_program_to_uniform_block_index.find(deleted_program);
    if (i != m_program_to_uniform_block_index.end())
      m_program_to_uniform_block_index.erase(i);
  }
  {
    auto i = m_program_to_uniform_block_binding.find(deleted_program);
    if (i != m_program_to_uniform_block_binding.end())
      m_program_to_uniform_block_binding.erase(i);
  }
}

void
StateTrack::parse() {
  m_poller->poll(current_program, this);
}

const ShaderAssembly &
StateTrack::currentVertexShader() const {
  auto sh = program_to_vertex.find(current_program);
  return (sh == program_to_vertex.end() ?
          empty_shader : sh->second);
}

const ShaderAssembly &
StateTrack::currentFragmentShader() const {
  auto sh = program_to_fragment.find(current_program);
  return (sh == program_to_fragment.end() ? empty_shader : sh->second);
}

const ShaderAssembly &
StateTrack::currentTessControlShader() const {
  auto sh = program_to_tess_control.find(current_program);
  return (sh == program_to_tess_control.end() ? empty_shader : sh->second);
}

const ShaderAssembly &
StateTrack::currentTessEvalShader() const {
  auto sh = program_to_tess_eval.find(current_program);
  return (sh == program_to_tess_eval.end() ? empty_shader : sh->second);
}

const ShaderAssembly &
StateTrack::currentGeomShader() const {
  auto sh = program_to_geom.find(current_program);
  return (sh == program_to_geom.end() ? empty_shader : sh->second);
}

const ShaderAssembly &
StateTrack::currentCompShader() const {
  auto sh = program_to_comp.find(current_program);
  return (sh == program_to_comp.end() ? empty_shader : sh->second);
}

void
StateTrack::flush() { m_poller->poll(current_program, this); }

StateTrack::ProgramKey::ProgramKey(int orig_program,
                                   const std::string &v,
                                   const std::string &f,
                                   const std::string &t_c,
                                   const std::string &t_e,
                                   const std::string &g,
                                   const std::string &c)
    : orig(orig_program), vs(v), fs(f), tess_control(t_c),
      tess_eval(t_e), geom(g), comp(c) {}

bool
StateTrack::ProgramKey::operator<(const ProgramKey &o) const {
  if (orig < o.orig)
    return true;
  if (orig > o.orig)
    return false;
  if (vs < o.vs)
    return true;
  if (vs > o.vs)
    return false;
  if (fs < o.fs)
    return true;
  if (fs > o.fs)
    return false;
  if (tess_control < o.tess_control)
    return true;
  if (tess_control > o.tess_control)
    return false;
  if (tess_eval < o.tess_eval)
    return true;
  if (tess_eval > o.tess_eval)
    return false;
  if (geom < o.geom)
    return true;
  if (geom > o.geom)
    return false;
  if (comp < o.comp)
    return true;
  return false;
}

int
StateTrack::useProgram(int orig_retraced_program,
                       const std::string &vs,
                       const std::string &fs,
                       const std::string &tessControl,
                       const std::string &tessEval,
                       const std::string &geom,
                       const std::string &comp,
                       std::string *message) {
  if (vs.empty() && comp.empty())
    return -1;

  const ProgramKey k(orig_retraced_program,
                     vs, fs, tessControl, tessEval,
                     geom, comp);
  auto i = m_sources_to_program.find(k);
  if (i != m_sources_to_program.end())
    return i->second;

  // TODO(majanes) assert if ids are being reused
  const GLuint pid = GlFunctions::CreateProgram();
  GL_CHECK();
  assert(program_to_fragment.find(pid)
         == program_to_fragment.end());
  current_program = pid;
  flush();
  if (vs.size()) {
    auto vshader = source_to_shader.find(vs);
    if (vshader == source_to_shader.end()) {
      // have to compile the vs
      const GLint len = vs.size();
      const GLchar *vsstr = vs.c_str();
      const GLuint vsid = GlFunctions::CreateShader(GL_VERTEX_SHADER);
      GlFunctions::ShaderSource(vsid, 1, &vsstr, &len);
      GL_CHECK();
      GlFunctions::CompileShader(vsid);
      if (message) {
        GetCompileError(vsid, message);
        if (message->size()) {
          GRLOGF(WARN, "compile error: %s", message->c_str());
          return -1;
        }
      }
      if (GL_NO_ERROR != GlFunctions::GetError()) {
        return -1;
      }
      GL_CHECK();
      // TODO(majanes) check error and poll
      source_to_shader[vs] = vsid;
      vshader = source_to_shader.find(vs);
    }
    program_to_vertex[pid].shader = vs;
    GlFunctions::AttachShader(pid, vshader->second);
    GL_CHECK();
  }

  // compile/attach fragment shader if not empty
  if (fs.size()) {
    auto fshader = source_to_shader.find(fs);
    if (fshader == source_to_shader.end()) {
      // have to compile the fs
      const GLint len = fs.size();
      const GLchar *fsstr = fs.c_str();
      const GLuint fsid = GlFunctions::CreateShader(GL_FRAGMENT_SHADER);
      GlFunctions::ShaderSource(fsid, 1, &fsstr, &len);
      GL_CHECK();
      GlFunctions::CompileShader(fsid);
      if (message) {
        GetCompileError(fsid, message);
        if (message->size()) {
          GRLOGF(WARN, "compile error: %s", message->c_str());
          return -1;
        }
      }
      if (GL_NO_ERROR != GlFunctions::GetError()) {
        return -1;
      }
      GL_CHECK();
      // TODO(majanes) check error and poll
      source_to_shader[fs] = fsid;
      fshader = source_to_shader.find(fs);
    }
    program_to_fragment[pid].shader = fs;
    GlFunctions::AttachShader(pid, fshader->second);
    GL_CHECK();
  }

  // compile/attach tess shaders if not empty
  if (tessControl.size()) {
    auto shader = source_to_shader.find(tessControl);
    if (shader == source_to_shader.end()) {
      // have to compile the fs
      const GLint len = tessControl.size();
      const GLchar *cstr = tessControl.c_str();
      const GLuint id = GlFunctions::CreateShader(GL_TESS_CONTROL_SHADER);
      GlFunctions::ShaderSource(id, 1, &cstr, &len);
      GL_CHECK();
      GlFunctions::CompileShader(id);
      if (message) {
        GetCompileError(id, message);
        if (message->size()) {
          GRLOGF(WARN, "compile error: %s", message->c_str());
          return -1;
        }
      }
      if (GL_NO_ERROR != GlFunctions::GetError()) {
        return -1;
      }
      GL_CHECK();
      // TODO(majanes) check error and poll
      source_to_shader[tessControl] = id;
    }
    shader = source_to_shader.find(tessControl);
    GlFunctions::AttachShader(pid, shader->second);
    GL_CHECK();
    program_to_tess_control[pid].shader = tessControl;
  }

  // compile/attach tess shaders if not empty
  if (tessEval.size()) {
    auto shader = source_to_shader.find(tessEval);
    if (shader == source_to_shader.end()) {
      // have to compile the fs
      const GLint len = tessEval.size();
      const GLchar *cstr = tessEval.c_str();
      const GLuint id = GlFunctions::CreateShader(GL_TESS_EVALUATION_SHADER);
      GlFunctions::ShaderSource(id, 1, &cstr, &len);
      GL_CHECK();
      GlFunctions::CompileShader(id);
      if (message) {
        GetCompileError(id, message);
        if (message->size()) {
          GRLOGF(WARN, "compile error: %s", message->c_str());
          return -1;
        }
      }
      if (GL_NO_ERROR != GlFunctions::GetError()) {
        return -1;
      }
      GL_CHECK();
      // TODO(majanes) check error and poll
      source_to_shader[tessEval] = id;
    }
    shader = source_to_shader.find(tessEval);
    GlFunctions::AttachShader(pid, shader->second);
    program_to_tess_eval[pid].shader = tessEval;
  }

  // compile/attach geometry shader if not empty
  if (geom.size()) {
    auto shader = source_to_shader.find(geom);
    if (shader == source_to_shader.end()) {
      // have to compile the fs
      const GLint len = geom.size();
      const GLchar *cstr = geom.c_str();
      const GLuint id = GlFunctions::CreateShader(GL_GEOMETRY_SHADER);
      GlFunctions::ShaderSource(id, 1, &cstr, &len);
      GL_CHECK();
      GlFunctions::CompileShader(id);
      if (message) {
        GetCompileError(id, message);
        if (message->size()) {
          GRLOGF(WARN, "compile error: %s", message->c_str());
          return -1;
        }
      }
      if (GL_NO_ERROR != GlFunctions::GetError()) {
        return -1;
      }
      GL_CHECK();
      // TODO(majanes) check error and poll
      source_to_shader[geom] = id;
    }
    shader = source_to_shader.find(geom);
    GlFunctions::AttachShader(pid, shader->second);
    GL_CHECK();
    program_to_geom[pid].shader = geom;
  }

  // compile/attach compute shader if not empty
  if (comp.size()) {
    auto shader = source_to_shader.find(comp);
    if (shader == source_to_shader.end()) {
      // have to compile the fs
      const GLint len = comp.size();
      const GLchar *cstr = comp.c_str();
      const GLuint id = GlFunctions::CreateShader(GL_COMPUTE_SHADER);
      GlFunctions::ShaderSource(id, 1, &cstr, &len);
      GL_CHECK();
      GlFunctions::CompileShader(id);
      if (message) {
        GetCompileError(id, message);
        if (message->size()) {
          GRLOGF(WARN, "compile error: %s", message->c_str());
          return -1;
        }
      }
      if (GL_NO_ERROR != GlFunctions::GetError()) {
        return -1;
      }
      GL_CHECK();
      // TODO(majanes) check error and poll
      source_to_shader[comp] = id;
    }
    shader = source_to_shader.find(comp);
    GlFunctions::AttachShader(pid, shader->second);
    GL_CHECK();
    program_to_comp[pid].shader = comp;
  }

  for (auto &binding : m_program_to_bound_attrib[orig_retraced_program]) {
    if (binding.first == -1)
      continue;
    GlFunctions::BindAttribLocation(pid, binding.first, binding.second.c_str());
  }

  for (auto &binding : m_program_to_frag_data_location[orig_retraced_program]) {
    GlFunctions::BindFragDataLocation(pid, binding.second,
                                      binding.first.c_str());
  }

  GlFunctions::LinkProgram(pid);
  GL_CHECK();
  if (message) {
    GetLinkError(pid, message);
    if (message->size()) {
      return -1;
    }
  } else {
    std::string _message;
    GetLinkError(pid, &_message);
    if (_message.size() > 0) {
      GRLOGF(WARN, "link error for custom program: %s", _message.c_str());
      return -1;
    }
  }

  // TODO(majanes) check error
  parse();

  m_sources_to_program[k] = pid;
  program_to_replacements[orig_retraced_program].push_back(pid);

  for (const auto &name_to_index :
           m_program_to_uniform_block_index[orig_retraced_program]) {
    const char *name = name_to_index.first.c_str();
    const int index = GlFunctions::GetUniformBlockIndex(pid, name);
    if (index == -1)
      continue;
    const int orig_index = name_to_index.second;
    auto &block_binding =
        m_program_to_uniform_block_binding[orig_retraced_program];
    const int binding = block_binding[orig_index];
    GlFunctions::UniformBlockBinding(pid, index,
                                     binding);
  }

  // set initial uniform state for program, based on the original.
  // Some game titles partially initialize uniforms at link time.
  int cur_prog;
  GlFunctions::GetIntegerv(GL_CURRENT_PROGRAM, &cur_prog);
  GlFunctions::UseProgram(orig_retraced_program);
  Uniforms orig;
  GlFunctions::UseProgram(pid);
  orig.set();
  GlFunctions::UseProgram(cur_prog);

  return pid;
}

void
StateTrack::useProgram(int program) {
  current_program = program;
  parse();
}

void
StateTrack::useProgramGL(int program) {
  // glretrace keeps tabs on the current program and asserts if you
  // don't correct its accounting.
  glretrace::Context *currentContext = glretrace::getCurrentContext();
  GlFunctions::UseProgram(program);
  currentContext->currentProgram = program;
}

void
StateTrack::retraceProgramSideEffects(int orig_program, trace::Call *c,
                                      retrace::Retracer *retracer) const {
  if (strncmp("glProgramUniform", c->sig->name,
              strlen("glProgramUniform")) == 0) {
    const int program = c->arg(0).toUInt();
    const int retraced_program = getRetracedProgram(program);
    auto replacements = program_to_replacements.find(retraced_program);
    if (replacements != program_to_replacements.end()) {
      trace::Value * call_program = c->args[0].value;
      trace::Value * call_loc = c->args[1].value;
      const int call_loc_val = call_loc->toSInt();
      for (auto replacement : replacements->second) {
        trace::UInt replace_program(replacement);
        c->args[0].value = &replace_program;
        auto name_map = m_program_to_uniform_name.find(retraced_program);
        assert(name_map != m_program_to_uniform_name.end());
        auto name_it = name_map->second.find(call_loc->toSInt());
        trace::SInt replace_loc(call_loc_val);
        if (name_it != name_map->second.end()) {
          const std::string &name = name_it->second;
          GLint loc = GlFunctions::GetUniformLocation(replacement,
                                                      name.c_str());
          replace_loc.value = loc;
          c->args[1].value = &replace_loc;
        }
        retracer->retrace(*c);
      }
      c->args[0].value = call_program;
      c->args[1].value = call_loc;
    }
    return;
  }
  if (strncmp("glUniform", c->sig->name, strlen("glUniform")) == 0) {
    const int retraced_program = orig_program;
    assert(retraced_program == orig_program);
    auto replacements = program_to_replacements.find(retraced_program);
    if (replacements != program_to_replacements.end()) {
      trace::Value * call_loc = c->args[0].value;
      const int call_loc_val = call_loc->toSInt();
      for (auto replacement : replacements->second) {
        useProgramGL(replacement);
        auto name_map = m_program_to_uniform_name.find(retraced_program);
        assert(name_map != m_program_to_uniform_name.end());
        auto name_it = name_map->second.find(call_loc_val);
        trace::SInt replace_loc(call_loc_val);
        if (name_it != name_map->second.end()) {
          const std::string &name = name_it->second;
          GLint loc = GlFunctions::GetUniformLocation(replacement,
                                                      name.c_str());
          replace_loc.value = loc;
          c->args[0].value = &replace_loc;
        }
        retracer->retrace(*c);
      }
      c->args[0].value = call_loc;
      useProgramGL(orig_program);
    }
    return;
  }
}

void
StateTrack::trackBindAttribLocation(const Call &call) {
  const int call_program = call.args[0].value->toDouble();
  const int program = glretrace::getRetracedProgram(call_program);
  const int location = call.args[1].value->toDouble();
  const std::string name(call.args[2].value->toString());
  m_program_to_bound_attrib[program][location] = name;
}

void
StateTrack::trackGetAttribLocation(const trace::Call &call) {
  const int call_program = call.args[0].value->toDouble();
  const int program = glretrace::getRetracedProgram(call_program);
  const std::string name(call.args[1].value->toString());
  const int location = GlFunctions::GetAttribLocation(program,
                                                      name.c_str());
  m_program_to_bound_attrib[program][location] = name;
}

void
StateTrack::trackGetUniformLocation(const Call &call) {
  const int call_program = call.args[0].value->toDouble();
  const int program = glretrace::getRetracedProgram(call_program);
  const std::string name(call.args[1].value->toString());
  const int location = GlFunctions::GetUniformLocation(program,
                                                       name.c_str());
  m_program_to_uniform_name[program][location] = name;
}

void
StateTrack::trackGetUniformBlockIndex(const trace::Call &call) {
  const int call_index = call.ret->toDouble();
  if (call_index == -1)
    return;
  const int call_program = call.args[0].value->toDouble();

  const int retraced_index =
      glretrace::getRetracedUniformBlockIndex(call_program, call_index);

  const int program = glretrace::getRetracedProgram(call_program);
  const std::string name(call.args[1].value->toString());
  m_program_to_uniform_block_index[program][name] = retraced_index;
}

void
StateTrack::trackUniformBlockBinding(const trace::Call &call) {
  const int call_program = call.args[0].value->toDouble();
  const int program = glretrace::getRetracedProgram(call_program);
  const int call_index = call.args[1].value->toDouble();

  const int retraced_index =
      glretrace::getRetracedUniformBlockIndex(call_program, call_index);

  const int binding = call.args[2].value->toDouble();
  m_program_to_uniform_block_binding[program][retraced_index] = binding;
}

void
StateTrack::trackBindFragDataLocation(const trace::Call &call) {
  const int call_program = call.args[0].value->toDouble();
  const int program = glretrace::getRetracedProgram(call_program);
  const int call_location = call.args[1].value->toDouble();
  const std::string name(call.args[2].value->toString());
  m_program_to_frag_data_location[program][name] = call_location;
}

void
StateTrack::trackBindProgramPipeline(const trace::Call &call) {
  int call_pipeline = call.args[0].value->toDouble();
  if (!call_pipeline)
    current_pipeline = 0;
  else
    current_pipeline = getRetracedPipeline(call_pipeline);
}

void
StateTrack::trackUseProgramStages(const trace::Call &call) {
  const int call_pipeline = call.args[0].value->toDouble();
  const int pipeline = getRetracedPipeline(call_pipeline);
  const unsigned int stages = call.args[1].value->toDouble();
  const int call_program = call.args[2].value->toDouble();
  const int program = getRetracedProgram(call_program);
  if (stages & GL_VERTEX_SHADER_BIT)
    pipeline_to_vertex_program[pipeline] = program;
  if (stages & GL_FRAGMENT_SHADER_BIT)
    pipeline_to_fragment_program[pipeline] = program;
  if (stages & GL_TESS_CONTROL_SHADER_BIT)
    pipeline_to_tess_control_program[pipeline] = program;
  if (stages & GL_TESS_EVALUATION_SHADER_BIT)
    pipeline_to_tess_eval_program[pipeline] = program;
  if (stages & GL_GEOMETRY_SHADER_BIT)
    pipeline_to_geom_program[pipeline] = program;
  if (stages & GL_COMPUTE_SHADER_BIT)
    pipeline_to_comp_program[pipeline] = program;
}

void
StateTrack::onAssembly(ShaderType st, AssemblyType at,
                       const std::string &assembly) {
  if (!current_program)
    return;
  ShaderAssembly *sa;
  switch (st) {
    case kVertex:
      sa = &program_to_vertex[current_program];
      break;
    case kFragment:
      sa = &program_to_fragment[current_program];
      break;
    case kTessEval:
      sa = &program_to_tess_eval[current_program];
      break;
    case kTessControl:
      sa = &program_to_tess_control[current_program];
      break;
    case kGeometry:
      sa = &program_to_geom[current_program];
      break;
    case kCompute:
      sa = &program_to_comp[current_program];
      break;
    case kShaderTypeUnknown:
      return;
  }

  switch (at) {
    case kSimd8:
      sa->simd8 = assembly;
      break;
    case kSimd16:
      sa->simd16 = assembly;
      break;
    case kSimd32:
      sa->simd32 = assembly;
      break;
    case kOriginal:
      sa->ir = assembly;
      break;
    case kBeforeUnification:
      sa->beforeUnification = assembly;
      break;
    case kAfterUnification:
      sa->afterUnification = assembly;
      break;
    case kBeforeOptimization:
      sa->beforeOptimization = assembly;
      break;
    case kConstCoalescing:
      sa->constCoalescing = assembly;
      break;
    case kGenIrLowering:
      sa->genIrLowering = assembly;
      break;
    case kLayout:
      sa->layout = assembly;
      break;
    case kOptimized:
      sa->optimized = assembly;
      break;
    case kPushAnalysis:
      sa->pushAnalysis = assembly;
      break;
    case kCodeHoisting:
      sa->codeHoisting = assembly;
      break;
    case kCodeSinking:
      sa->codeSinking = assembly;
      break;
    case kIr:
      sa->ir = assembly;
      break;
    case kNirSsa:
      sa->ssa = assembly;
      break;
    case kNirFinal:
      sa->nir = assembly;
      break;
    case kAssemblyTypeUnknown:
      return;
  }
}
