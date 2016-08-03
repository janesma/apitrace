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

#include "glframe_state.hpp"

#include <string.h>

#include <map>
#include <sstream>
#include <string>

#include "GL/gl.h"
#include "GL/glext.h"
#include "trace_model.hpp"
#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"
#include "retrace.hpp"
#include "glframe_retrace_interface.hpp"

using glretrace::StateTrack;
using glretrace::WARN;
using glretrace::OutputPoller;
using glretrace::OnFrameRetrace;
using glretrace::ShaderAssembly;
using trace::Call;
using trace::Array;

static std::map<std::string, bool> ignore_strings;

StateTrack::TrackMap StateTrack::lookup;

StateTrack::StateTrack(OutputPoller *p)
    : m_poller(p),
      current_program(0),
      current_context(0) {
}

StateTrack::TrackMap::TrackMap() {
  lookup["glAttachShader"] = &StateTrack::trackAttachShader;
  lookup["glCreateShader"] = &StateTrack::trackCreateShader;
  lookup["glLinkProgram"] = &StateTrack::trackLinkProgram;
  lookup["glShaderSource"] = &StateTrack::trackShaderSource;
  lookup["glUseProgram"] = &StateTrack::trackUseProgram;
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

bool
changesContext(const trace::Call &call) {
  if (strncmp(call.name(), "glXMakeCurrent", strlen("glXMakeCurrent")) == 0)
    return true;
  return false;
}

uint64_t
getContext(const trace::Call &call) {
  assert(changesContext(call));
  // there ought to be a const variant for this
  return const_cast<trace::Call &>(call).arg(2).toUIntPtr();
}

// TODO(majanes): use a lookup table
void
StateTrack::track(const Call &call) {
  if (lookup.track(this, call)) {
    std::stringstream call_stream;
    trace::dump(const_cast<Call&>(call), call_stream,
                trace::DUMP_FLAG_NO_COLOR);
    tracked_calls.push_back(call_stream.str());
    GRLOG(glretrace::WARN, call_stream.str().c_str());
  }

  if (changesContext(call))
    current_context = getContext(call);

  parse();
}

void
StateTrack::trackAttachShader(const Call &call) {
  const int program = call.args[0].value->toDouble();
  const int shader = call.args[1].value->toDouble();
  if (shader_to_type[shader] == GL_FRAGMENT_SHADER)
    program_to_fragment[program].shader = shader_to_source[shader];
  else if (shader_to_type[shader] == GL_VERTEX_SHADER)
    program_to_vertex[program].shader = shader_to_source[shader];
  else if (shader_to_type[shader] == GL_TESS_CONTROL_SHADER)
    program_to_tess_control[program].shader = shader_to_source[shader];
  else if (shader_to_type[shader] == GL_TESS_EVALUATION_SHADER)
    program_to_tess_eval[program].shader = shader_to_source[shader];
  else if (shader_to_type[shader] == GL_GEOMETRY_SHADER)
    program_to_geom[program].shader = shader_to_source[shader];

  auto vs = program_to_vertex.find(program);
  auto fs = program_to_fragment.find(program);
  auto tess_control = program_to_tess_control.find(program);
  auto tess_eval = program_to_tess_eval.find(program);
  auto geom = program_to_geom.find(program);
  const ProgramKey k(vs == program_to_vertex.end()
                     ? "" : vs->second.shader,
                     fs == program_to_fragment.end()
                     ? "" : fs->second.shader,
                     tess_control == program_to_tess_control.end()
                     ? "" : tess_control->second.shader,
                     tess_eval == program_to_tess_eval.end()
                     ? "" : tess_eval->second.shader,
                     geom == program_to_geom.end()
                     ? "" : geom->second.shader);
  m_sources_to_program[k] = program;
}

void
StateTrack::trackCreateShader(const Call &call) {
  const int shader_type = call.args[0].value->toDouble();
  const int shader = call.ret->toDouble();
  shader_to_type[shader] = shader_type;
}

void
StateTrack::trackShaderSource(const Call &call) {
  const int shader = call.args[0].value->toDouble();
  const Array * source = call.args[2].value->toArray();
  std::string text;
  for (auto line : source->values) {
    text += line->toString();
  }
  shader_to_source[shader] = text;
  source_to_shader[text] = shader;
}

void
StateTrack::trackLinkProgram(const trace::Call &call) {
  current_program = call.args[0].value->toDouble();
}

void
StateTrack::trackUseProgram(const trace::Call &call) {
  current_program = call.args[0].value->toDouble();
}

void
StateTrack::parse() {
  std::string fs_ir, fs_simd8, fs_simd16, vs_ir, vs_simd8, line,
      fs_nir_ssa, fs_nir_final, vs_nir_ssa, vs_nir_final,
      tess_eval_ir, tess_eval_ssa, tess_eval_final, tess_eval_simd8,
      tess_control_ir, tess_control_ssa, tess_control_final, tess_control_simd8,
      geom_ir, geom_ssa, geom_final, geom_simd8,
      *current_target = NULL;
  int line_shader = -1;
  while (true) {
    const std::string output = m_poller->poll();
    if (output.size() == 0)
      break;

    std::stringstream line_split(output);
    while (std::getline(line_split, line, '\n')) {
      int matches = sscanf(line.c_str(),
                           "GLSL IR for native vertex shader %d:",
                           &line_shader);
      if (matches > 0)
        current_target = &vs_ir;

      if (0 == strcmp(line.c_str(), "NIR (SSA form) for vertex shader:"))
        current_target = &vs_nir_ssa;

      if (0 == strcmp(line.c_str(), "NIR (final form) for vertex shader:"))
        current_target = &vs_nir_final;

      if (0 == strcmp(line.c_str(), "NIR (SSA form) for fragment shader:"))
        current_target = &fs_nir_ssa;

      if (0 == strcmp(line.c_str(), "NIR (final form) for fragment shader:"))
        current_target = &fs_nir_final;

      if (matches <= 0) {
        matches = sscanf(line.c_str(),
                         "Native code for unnamed vertex shader GLSL%d:",
                         &line_shader);
        if (matches > 0)
          current_target = &vs_simd8;
      }
      if (matches <= 0) {
        matches = sscanf(line.c_str(),
                         "Native code for meta clear vertex shader %d:",
                         &line_shader);
        if (matches > 0)
          current_target = &vs_simd8;
      }
      if (matches <= 0) {
        matches = sscanf(line.c_str(),
                         "GLSL IR for native fragment shader %d:",
                         &line_shader);
        if (matches > 0)
          current_target = &fs_ir;
      }
      if (matches <= 0) {
        int wide;
        matches = sscanf(line.c_str(),
                         "Native code for unnamed fragment shader GLSL%d",
                         &line_shader);
        if (matches > 0) {
          if (line_shader != current_program) {
            current_target = NULL;
            continue;
          }
          // for Native code, need the second line to get the
          // target.
          const std::string line_copy = line;
          std::getline(line_split, line, '\n');
          matches = sscanf(line.c_str(),
                           "SIMD%d", &wide);
          assert(matches > 0);
          current_target = ((wide == 16) ? &fs_simd16 : &fs_simd8);

          *current_target += line_copy + "\n";
        }
      }

      if (matches <= 0) {
        matches = sscanf(line.c_str(),
                         "GLSL IR for native tessellation "
                         "evaluation shader %d:",
                         &line_shader);
        if (matches > 0)
          current_target = &tess_eval_ir;
      }
      if (0 == strcmp(line.c_str(),
                      "NIR (SSA form) for tessellation evaluation shader:"))
        current_target = &tess_eval_ssa;

      if (0 == strcmp(line.c_str(),
                      "NIR (final form) for tessellation evaluation shader:"))
        current_target = &tess_eval_final;

      if (matches <= 0) {
        matches = sscanf(line.c_str(),
                         "Native code for unnamed tessellation "
                         "evaluation shader GLSL%d:",
                         &line_shader);
        if (matches > 0)
          current_target = &tess_eval_simd8;
      }

      if (matches <= 0) {
        matches = sscanf(line.c_str(),
                         "GLSL IR for native tessellation control shader %d:",
                         &line_shader);
        if (matches > 0)
          current_target = &tess_control_ir;
      }
      if (0 == strcmp(line.c_str(),
                      "NIR (SSA form) for tessellation control shader:"))
        current_target = &tess_control_ssa;

      if (0 == strcmp(line.c_str(),
                      "NIR (final form) for tessellation control shader:"))
        current_target = &tess_control_final;

      if (matches <= 0) {
        matches = sscanf(line.c_str(),
                         "Native code for unnamed tessellation "
                         "control shader GLSL%d:",
                         &line_shader);
        if (matches > 0)
          current_target = &tess_control_simd8;
      }

      if (matches <= 0) {
        matches = sscanf(line.c_str(),
                         "GLSL IR for native geometry shader %d:",
                         &line_shader);
        if (matches > 0)
          current_target = &geom_ir;
      }
      if (0 == strcmp(line.c_str(),
                      "NIR (SSA form) for geometry shader:"))
        current_target = &geom_ssa;

      if (0 == strcmp(line.c_str(),
                      "NIR (final form) for geometry shader:"))
        current_target = &geom_final;

      if (matches <= 0) {
        matches = sscanf(line.c_str(),
                         "Native code for unnamed geometry "
                         "shader GLSL%d:",
                         &line_shader);
        if (matches > 0)
          current_target = &geom_simd8;
      }

      if (current_target) {
        *current_target += line + "\n";
      }
    }

    if (line_shader != current_program) {
      // this is probably a shader that mesa uses, flush output
      while (m_poller->poll().size() > 0) {}
      return;
    }
  }
  if (fs_ir.length() > 0)
    program_to_fragment[current_program].ir = fs_ir;
  if (fs_nir_ssa.length() > 0)
    program_to_fragment[current_program].ssa = fs_nir_ssa;
  if (fs_nir_final.length() > 0)
    program_to_fragment[current_program].nir = fs_nir_final;
  if (fs_simd8.length() > 0)
    program_to_fragment[current_program].simd8 = fs_simd8;
  if (fs_simd16.length() > 0)
    program_to_fragment[current_program].simd16 = fs_simd16;

  if (vs_ir.length() > 0)
    program_to_vertex[current_program].ir = vs_ir;
  if (vs_nir_ssa.length() > 0)
    program_to_vertex[current_program].ssa = vs_nir_ssa;
  if (vs_nir_final.length() > 0)
    program_to_vertex[current_program].nir = vs_nir_final;
  if (vs_simd8.length() > 0)
    program_to_vertex[current_program].simd8 = vs_simd8;

  if (tess_eval_ir.length() > 0)
    program_to_tess_eval[current_program].ir = tess_eval_ir;
  if (tess_eval_ssa.length() > 0)
    program_to_tess_eval[current_program].ssa = tess_eval_ssa;
  if (tess_eval_final.length() > 0)
    program_to_tess_eval[current_program].nir = tess_eval_final;
  if (tess_eval_simd8.length() > 0)
    program_to_tess_eval[current_program].simd8 = tess_eval_simd8;

  if (tess_control_ir.length() > 0)
    program_to_tess_control[current_program].ir = tess_control_ir;
  if (tess_control_ssa.length() > 0)
    program_to_tess_control[current_program].ssa = tess_control_ssa;
  if (tess_control_final.length() > 0)
    program_to_tess_control[current_program].nir = tess_control_final;
  if (tess_control_simd8.length() > 0)
    program_to_tess_control[current_program].simd8 = tess_control_simd8;

  if (geom_ir.length() > 0)
    program_to_geom[current_program].ir = geom_ir;
  if (geom_ssa.length() > 0)
    program_to_geom[current_program].ssa = geom_ssa;
  if (geom_final.length() > 0)
    program_to_geom[current_program].nir = geom_final;
  if (geom_simd8.length() > 0)
    program_to_geom[current_program].simd8 = geom_simd8;
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

void
StateTrack::flush() { m_poller->poll(); }

StateTrack::ProgramKey::ProgramKey(const std::string &v,
                                   const std::string &f,
                                   const std::string &t_c,
                                   const std::string &t_e,
                                   const std::string &g)
    : vs(v), fs(f), tess_control(t_c),
      tess_eval(t_e), geom(g) {}

bool
StateTrack::ProgramKey::operator<(const ProgramKey &o) const {
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
  return false;
}

int
StateTrack::useProgram(const std::string &vs,
                       const std::string &fs,
                       const std::string &tessControl,
                       const std::string &tessEval,
                       const std::string &geom,
                       std::string *message) {
  const ProgramKey k(vs, fs, tessControl, tessEval, geom);
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

  GlFunctions::LinkProgram(pid);
  if (message) {
    GetLinkError(pid, message);
    if (message->size()) {
      return -1;
    }
  }
  GL_CHECK();

  // TODO(majanes) check error
  parse();
  m_sources_to_program[k] = pid;
  return pid;
}

void
StateTrack::useProgram(int program) {
  current_program = program;
  parse();
}

void
StateTrack::onApi(OnFrameRetrace *callback) {
  callback->onApi(RenderId(-1), tracked_calls);
}
