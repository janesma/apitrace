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

#include "GLES2/gl2.h"
#include "trace_model.hpp"
#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"

using glretrace::StateTrack;
using glretrace::WARN;
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

void
StateTrack::TrackMap::track(StateTrack *tracker, const Call &call) {
  auto resolve = lookup.find(call.sig->name);
  if (resolve == lookup.end())
    return;

  MemberFunType funptr = resolve->second;
  (tracker->*funptr)(call);
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
  lookup.track(this, call);

  if (changesContext(call))
    current_context = getContext(call);

  parse();
}

void
StateTrack::trackAttachShader(const Call &call) {
  const int program = call.args[0].value->toDouble();
  const int shader = call.args[1].value->toDouble();
  if (shader_to_type[shader] == GL_FRAGMENT_SHADER)
    program_to_fragment_shader_source[program] = shader_to_source[shader];
  else if (shader_to_type[shader] == GL_VERTEX_SHADER)
    program_to_vertex_shader_source[program] = shader_to_source[shader];

  auto vs = program_to_vertex_shader_source.find(program);
  if (vs == program_to_vertex_shader_source.end())
    return;
  auto fs = program_to_fragment_shader_source.find(program);
  if (fs == program_to_fragment_shader_source.end())
    return;
  m_sources_to_program[ProgramKey(vs->second, fs->second)] = program;
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
  const std::string output = m_poller->poll();
  if (output.size() == 0)
    return;

  std::string fs_ir, fs_simd8, fs_simd16, vs_ir, vs_vec4, line,
      fs_nir_ssa, fs_nir_final,
      *current_target = NULL;
  std::stringstream line_split(output);
  int line_shader = -1;
  while (std::getline(line_split, line, '\n')) {
    int matches = sscanf(line.c_str(),
                         "GLSL IR for native vertex shader %d:", &line_shader);
    if (matches > 0)
      current_target = &vs_ir;

    if (0 == strcmp(line.c_str(), "NIR (SSA form) for fragment shader:"))
      current_target = &fs_nir_ssa;

    if (0 == strcmp(line.c_str(), "NIR (final form) for fragment shader:"))
      current_target = &fs_nir_final;

    if (matches <= 0) {
      matches = sscanf(line.c_str(),
                       "Native code for unnamed vertex shader %d:",
                       &line_shader);
      if (matches > 0)
        current_target = &vs_vec4;
    }
    if (matches <= 0) {
      matches = sscanf(line.c_str(),
                       "Native code for meta clear vertex shader %d:",
                       &line_shader);
      if (matches > 0)
        current_target = &vs_vec4;
    }
    if (matches <= 0) {
      matches = sscanf(line.c_str(),
                       "GLSL IR for native fragment shader %d:", &line_shader);
      if (matches > 0)
        current_target = &fs_ir;
    }
    if (matches <= 0) {
      int wide;
      matches = sscanf(line.c_str(),
                       "Native code for unnamed fragment shader %d",
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
    if (current_target) {
      *current_target += line + "\n";
    }
  }

  if (line_shader != current_program)
    // this is probably a shader that mesa uses
    return;

  if (fs_ir.length() > 0)
    program_to_fragment_shader_ir[current_program] = fs_ir;
  if (fs_simd8.length() > 0)
    program_to_fragment_shader_simd8[current_program] = fs_simd8;
  if (fs_simd16.length() > 0)
    program_to_fragment_shader_simd16[current_program] = fs_simd16;
  if (vs_ir.length() > 0)
    program_to_vertex_shader_ir[current_program] = vs_ir;
  if (vs_vec4.length() > 0)
    program_to_vertex_shader_vec4[current_program] = vs_vec4;
  if (fs_nir_final.length() > 0)
    program_to_fragment_shader_nir[current_program] = fs_nir_final;
  if (fs_nir_ssa.length() > 0)
    program_to_fragment_shader_ssa[current_program] = fs_nir_ssa;
}

std::string
StateTrack::currentVertexIr() const {
  auto sh = program_to_vertex_shader_ir.find(current_program);
  if (sh == program_to_vertex_shader_ir.end())
    return "";
  return sh->second;
}

std::string
StateTrack::currentFragmentIr() const {
  auto sh = program_to_fragment_shader_ir.find(current_program);
  if (sh == program_to_fragment_shader_ir.end())
    return "";
  return sh->second;
}

std::string
StateTrack::currentVertexShader() const {
  auto sh = program_to_vertex_shader_source.find(current_program);
  if (sh == program_to_vertex_shader_source.end())
    return "";
  return sh->second;
}

std::string
StateTrack::currentFragmentShader() const {
  auto sh = program_to_fragment_shader_source.find(current_program);
  if (sh == program_to_fragment_shader_source.end())
    return "";
  return sh->second;
}

std::string
StateTrack::currentVertexVec4() const {
  auto sh = program_to_vertex_shader_vec4.find(current_program);
  if (sh == program_to_vertex_shader_vec4.end())
    return "";
  return sh->second;
}

std::string
StateTrack::currentFragmentSimd8() const {
  auto sh = program_to_fragment_shader_simd8.find(current_program);
  if (sh == program_to_fragment_shader_simd8.end())
    return "";
  return sh->second;
}

std::string
StateTrack::currentFragmentSimd16() const {
  auto sh = program_to_fragment_shader_simd16.find(current_program);
  if (sh == program_to_fragment_shader_simd16.end())
    return "";
  return sh->second;
}

std::string
StateTrack::currentFragmentSSA() const {
  auto sh = program_to_fragment_shader_ssa.find(current_program);
  if (sh == program_to_fragment_shader_ssa.end())
    return "";
  return sh->second;
}

std::string
StateTrack::currentFragmentNIR() const {
  auto sh = program_to_fragment_shader_nir.find(current_program);
  if (sh == program_to_fragment_shader_nir.end())
    return "";
  return sh->second;
}

void
StateTrack::flush() { m_poller->poll(); }

StateTrack::ProgramKey::ProgramKey(const std::string &v,
                               const std::string &f)
    : vs(v), fs(f) {}

bool
StateTrack::ProgramKey::operator<(const ProgramKey &o) const {
  if (vs < o.vs)
    return true;
  if (vs > o.vs)
    return false;
  if (fs < o.fs)
    return true;
  return false;
}

int
StateTrack::useProgram(const std::string &vs,
                       const std::string &fs,
                       std::string *message) {
  const ProgramKey k(vs, fs);
  auto i = m_sources_to_program.find(k);
  if (i != m_sources_to_program.end())
    return i->second;

  // TODO(majanes) assert if ids are being reused
  const GLuint pid = GlFunctions::CreateProgram();
  GL_CHECK();
  assert(program_to_fragment_shader_source.find(pid)
         == program_to_fragment_shader_source.end());
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
      return -1;
    }
    if (GL_NO_ERROR != GlFunctions::GetError()) {
      return -1;
    }
    GL_CHECK();
    // TODO(majanes) check error and poll
    source_to_shader[vs] = vsid;
    vshader = source_to_shader.find(vs);
  }
  program_to_vertex_shader_source[pid] = vs;

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
      GRLOGF(WARN, "compile error: %s", message->c_str());
      return -1;
    }
    if (GL_NO_ERROR != GlFunctions::GetError()) {
      return -1;
    }
    GL_CHECK();
    // TODO(majanes) check error and poll
    source_to_shader[fs] = fsid;
    fshader = source_to_shader.find(fs);
  }
  program_to_fragment_shader_source[pid] = fs;

  GlFunctions::AttachShader(pid, fshader->second);
  GL_CHECK();
  GlFunctions::AttachShader(pid, vshader->second);
  GL_CHECK();
  GlFunctions::LinkProgram(pid);
  const int error = GlFunctions::GetError();
  if (GL_NO_ERROR != error) {
    GetLinkError(pid, message);
    return -1;
  }
  GL_CHECK();

  // TODO(majanes) check error
  parse();
  return pid;
}

void
StateTrack::useProgram(int program) {
  current_program = program;
  parse();
}
