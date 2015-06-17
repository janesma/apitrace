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

#include "trace_model.hpp"
#include "GLES2/gl2.h"

using glretrace::StateTrack;
using trace::Call;
using trace::Array;

static std::map<std::string, bool> ignore_strings;

StateTrack::TrackMap StateTrack::lookup;

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

// TODO(majanes): use a lookup table
void
StateTrack::track(const Call &call) {
  lookup.track(this, call);
}

void
StateTrack::trackAttachShader(const Call &call) {
  const int program = call.args[0].value->toDouble();
  const int shader = call.args[1].value->toDouble();
  if (shader_to_type[shader] == GL_FRAGMENT_SHADER)
    program_to_fragment_shader_source[program] = shader_to_source[shader];
  else if (shader_to_type[shader] == GL_VERTEX_SHADER)
    program_to_vertex_shader_source[program] = shader_to_source[shader];
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
StateTrack::parse(const std::string &output) {
  if (output.size() == 0)
    return;

  std::string fs_ir, fs_simd8, fs_simd16, vs_ir, vs_vec4, line,
      *current_target = NULL;
  std::stringstream line_split(output);
  while (std::getline(line_split, line, '\n')) {
    int line_shader;
    int matches = sscanf(line.c_str(),
                         "GLSL IR for native vertex shader %d:", &line_shader);
    if (matches > 0)
      current_target = &vs_ir;

    if (matches <= 0) {
      matches = sscanf(line.c_str(),
                       "Native code for unnamed vertex shader %d:",
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
    if (line_shader != current_program)
      // this is probably a shader that mesa uses
      current_target = NULL;
    if (current_target) {
      *current_target += line + "\n";
    }
  }
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
