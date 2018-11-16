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

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <map>
#include <sstream>
#include <string>

#include "glframe_stderr.hpp"
#include "glframe_logger.hpp"
#include "glframe_glhelper.hpp"
#include "glretrace.hpp"

using glretrace::GlFunctions;
using glretrace::ShaderCallback;
using glretrace::StdErrRedirect;
using glretrace::getCurrentContext;

StdErrRedirect::StdErrRedirect() {
}

void
StdErrRedirect::poll(int current_program, StateTrack *cb) {
  std::string fs_ir, fs_simd, fs_simd8, fs_simd16, vs_ir, vs_simd8, line,
      fs_nir_ssa, fs_nir_final, vs_nir_ssa, vs_nir_final,
      tess_eval_ir, tess_eval_ssa, tess_eval_final, tess_eval_simd8,
      tess_control_ir, tess_control_ssa, tess_control_final, tess_control_simd8,
      geom_ir, geom_ssa, geom_final, geom_simd8,
      comp_ir, comp_ssa, comp_final, comp_simd8,
      arb_vertex_shader, *current_target = NULL;

  int line_shader = -1;

  fflush(stdout);
  std::string assembly_output;
  int bytes = read(out_pipe[0], buf.data(), buf.size() - 1);
  while (0 < bytes) {
    buf[bytes] = '\0';
    assembly_output.append(buf.data());
    bytes = read(out_pipe[0], buf.data(), buf.size() - 1);
  }

  std::stringstream line_split(assembly_output);
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
        if (!matches) {
          /* for non-intel drivers we won't have this line, just re-
           * use fs_simd8 for the one possible FS mode:
           */
          current_target = &fs_simd;
        } else {
          current_target = ((wide == 16) ? &fs_simd16 : &fs_simd8);
        }

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

    if (matches <= 0) {
      matches = sscanf(line.c_str(),
                       "GLSL IR for native compute shader %d:",
                       &line_shader);
      if (matches > 0)
        current_target = &comp_ir;
    }
    if (0 == strcmp(line.c_str(),
                    "NIR (SSA form) for compute shader:"))
      current_target = &comp_ssa;

    if (0 == strcmp(line.c_str(),
                    "NIR (final form) for compute shader:"))
      current_target = &comp_final;

    if (matches <= 0) {
      matches = sscanf(line.c_str(),
                       "Native code for unnamed compute "
                       "shader GLSL%d:",
                       &line_shader);
      if (matches > 0)
        current_target = &comp_simd8;
    }

    if (matches <= 0) {
      matches = sscanf(line.c_str(),
                       "ARB_vertex_program %d ir for native vertex shader",
                       &line_shader);
      if (matches > 0)
        current_target = &arb_vertex_shader;  // ignored
    }

    if (current_target) {
      *current_target += line + "\n";
    } else {
      GRLOGF(glretrace::WARN, "%s", line.c_str());
    }
  }

  if (line_shader != current_program) {
    // this is probably a shader that mesa uses, ignore
    return;
  }

  if (fs_ir.length() > 0)
    cb->onAssembly(kFragment, kIr, fs_ir);
  if (fs_nir_ssa.length() > 0)
    cb->onAssembly(kFragment, kNirSsa, fs_nir_ssa);
  if (fs_nir_final.length() > 0)
    cb->onAssembly(kFragment, kNirFinal, fs_nir_final);
  if (fs_simd.length() > 0)
    cb->onAssembly(kFragment, kSimd, fs_simd);
  if (fs_simd8.length() > 0)
    cb->onAssembly(kFragment, kSimd8, fs_simd8);
  if (fs_simd16.length() > 0)
    cb->onAssembly(kFragment, kSimd16, fs_simd16);

  if (vs_ir.length() > 0)
    cb->onAssembly(kVertex, kIr, vs_ir);
  if (vs_nir_ssa.length() > 0)
    cb->onAssembly(kVertex, kNirSsa, vs_nir_ssa);
  if (vs_nir_final.length() > 0)
    cb->onAssembly(kVertex, kNirFinal, vs_nir_final);
  if (vs_simd8.length() > 0)
    cb->onAssembly(kVertex, kSimd8, vs_simd8);

  if (tess_control_ir.length() > 0)
    cb->onAssembly(kTessControl, kIr, tess_control_ir);
  if (tess_control_ssa.length() > 0)
    cb->onAssembly(kTessControl, kNirSsa, tess_control_ssa);
  if (tess_control_final.length() > 0)
    cb->onAssembly(kTessControl, kNirFinal, tess_control_final);
  if (tess_control_simd8.length() > 0)
    cb->onAssembly(kTessControl, kSimd8, tess_control_simd8);

  if (tess_eval_ir.length() > 0)
    cb->onAssembly(kTessEval, kIr, tess_eval_ir);
  if (tess_eval_ssa.length() > 0)
    cb->onAssembly(kTessEval, kNirSsa, tess_eval_ssa);
  if (tess_eval_final.length() > 0)
    cb->onAssembly(kTessEval, kNirFinal, tess_eval_final);
  if (tess_eval_simd8.length() > 0)
    cb->onAssembly(kTessEval, kSimd8, tess_eval_simd8);

  if (geom_ir.length() > 0)
    cb->onAssembly(kGeometry, kIr, geom_ir);
  if (geom_ssa.length() > 0)
    cb->onAssembly(kGeometry, kNirSsa, geom_ssa);
  if (geom_final.length() > 0)
    cb->onAssembly(kGeometry, kNirFinal, geom_final);
  if (geom_simd8.length() > 0)
    cb->onAssembly(kGeometry, kSimd8, geom_simd8);

  if (comp_ir.length() > 0)
    cb->onAssembly(kCompute, kIr, comp_ir);
  if (comp_ssa.length() > 0)
    cb->onAssembly(kCompute, kNirSsa, comp_ssa);
  if (comp_final.length() > 0)
    cb->onAssembly(kCompute, kNirFinal, comp_final);
  if (comp_simd8.length() > 0)
    cb->onAssembly(kCompute, kSimd8, comp_simd8);
}

StdErrRedirect::~StdErrRedirect() {
  close(out_pipe[0]);
}

void
StdErrRedirect::init() {
  setenv("INTEL_DEBUG", "vs,fs,tcs,tes,gs,cs", 1);
  setenv("FD_SHADER_DEBUG", "vs,fs,tcs,tes,gs,cs", 1);
  setenv("vblank_mode", "0", 1);
  int dev_null = open("/dev/null", O_WRONLY);
  dup2(dev_null, STDERR_FILENO);
  close(dev_null);
}

void
StdErrRedirect::pollBatch(SelectionId selectionCount,
                          ExperimentId experimentCount,
                          RenderId id,
                          OnFrameRetrace *cb) {
  fflush(stdout);
  std::string batch_output;
  int bytes = read(out_pipe[0], buf.data(), buf.size() - 1);
  while (0 < bytes) {
    buf[bytes] = '\0';
    batch_output.append(buf.data());
    bytes = read(out_pipe[0], buf.data(), buf.size() - 1);
  }

  cb->onBatch(selectionCount, experimentCount, id, batch_output);
}

void
StdErrRedirect::flush() {
  fflush(stdout);
  int bytes = read(out_pipe[0], buf.data(), buf.size() - 1);
  while (0 < bytes) {
    buf[bytes] = '\0';
    bytes = read(out_pipe[0], buf.data(), buf.size() - 1);
  }
}
void shader_callback(GLenum source, GLenum type, GLuint id,
                     GLenum severity, GLsizei length, const GLchar *message,
                     const void *userParam) {
  ShaderCallback* s = reinterpret_cast<ShaderCallback*>(
      const_cast<void*>(userParam));
  s->_callback(source, type, id, severity, length, message);
}

ShaderCallback::ShaderCallback()
    : m_tokens({{"GLSL IR for native ", &ShaderCallback::ir_handler},
          {"NIR (SSA form) for ", &ShaderCallback::nir_handler},
          {"NIR for ",  &ShaderCallback::nir_handler},
          {"NIR (final form) for ", &ShaderCallback::nir_final_handler},
          {"Native code for ", &ShaderCallback::native_handler}}) {
}

void
ShaderCallback::enable_debug_env() {
  // enable shader dumps
  setenv("INTEL_DEBUG", "vs,fs,tcs,tes,gs,cs", 1);
  setenv("FD_SHADER_DEBUG", "vs,fs,tcs,tes,gs,cs", 1);
  setenv("vblank_mode", "0", 1);
  // shader dumps also go to stderr, which is distracting.  Pipe to /dev/null
  int dev_null = open("/dev/null", O_WRONLY);
  dup2(dev_null, STDERR_FILENO);
  close(dev_null);
}

void
ShaderCallback::enable_debug_context() {
  // initialize debug context
  // only initialize debug output for new contexts
  Context *c = getCurrentContext();
  for (auto i : m_known_contexts)
    if (c == i)
      return;
  m_known_contexts.push_back(c);

  // turn off all messages
  GlFunctions::DebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE,
                                   0, NULL, GL_FALSE);
  // turn on shader messages
  GlFunctions::DebugMessageControl(GL_DEBUG_SOURCE_SHADER_COMPILER,
                                   GL_DEBUG_TYPE_OTHER,
                                   GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL,
                                   GL_TRUE);
  GlFunctions::GetIntegerv(GL_MAX_DEBUG_MESSAGE_LENGTH, &m_max_len);
  GlFunctions::DebugMessageCallback(shader_callback, this);
  GlFunctions::Enable(GL_DEBUG_OUTPUT);
  // we want to handle this gracefully
  // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
}

void
ShaderCallback::find_handler(GLuint id, GLsizei length,
                             const GLchar *message) {
  for (auto i : m_tokens) {
    const char * token = i.first.c_str();
    if (strncmp(message,
              token,
                strlen(token)) == 0) {
      m_handlers[id] = i.second;
      return;
    }
  }
  // else message cannot be parsed
  m_handlers[id] = &ShaderCallback::null_handler;

  // disable subsequent messages from the same source
  GlFunctions::DebugMessageControl(GL_DEBUG_SOURCE_SHADER_COMPILER,
                                   GL_DEBUG_TYPE_OTHER,
                                   GL_DEBUG_SEVERITY_NOTIFICATION, 1, &id,
                                   GL_FALSE);
}

void
ShaderCallback::null_handler(GLsizei length,
                             const GLchar *message) {
}

void
ShaderCallback::ir_handler(GLsizei length,
                           const GLchar *message) {
  // TODO(majanes)
  if (handle_partial_message(length, message))
    return;

  static const std::map<std::string, ShaderType> tokens =
      {{"GLSL IR for native vertex shader", kVertex},
       {"GLSL IR for native fragment shader", kFragment},
       {"GLSL IR for native tessellation control shader", kTessControl},
       {"GLSL IR for native tessellation evaluation shader", kTessEval},
       {"GLSL IR for native geometry shader", kGeometry},
       {"GLSL IR for native compute shader", kCompute}};
  for (auto i : tokens) {
    if (m_buf.compare(0, i.first.length(), i.first) == 0) {
      m_cached_assemblies[i.second][kIr] = m_buf;
        m_buf = "";
        return;
    }
  }
  m_buf = "";
}

void
ShaderCallback::native_handler(GLsizei length,
                               const GLchar *message) {
  // TODO(majanes)
  if (handle_partial_message(length, message))
    return;

  static const std::map<std::string, ShaderType> tokens =
      {{"vertex", kVertex},
       {"fragment", kFragment},
       {"tessellation control", kTessControl},
       {"tessellation evaluation", kTessEval},
       {"geometry", kGeometry},
       {"compute", kCompute}};

  std::stringstream line_split(m_buf);
  std::string line;
  std::getline(line_split, line, '\n');
  for (auto i : tokens) {
    if (strstr(line.c_str(), i.first.c_str()) != NULL) {
      std::stringstream word_split(line);
      std::string word;
      while (std::getline(word_split, word, ' ')) {
        int program;
        if (0 < sscanf(word.c_str(), "GLSL%d", &program) &&
            program != m_current_program) {
          std::cout << "mismatched program: "
                    << m_current_program << " " << program
                    << std::endl;
          m_buf = "";
          return;
        }
      }
      std::getline(line_split, line, '\n');
      AssemblyType t = kSimd;
      int width = 0;
      sscanf(line.c_str(), "SIMD%d", &width);
      if (width == 8)
        t = kSimd8;
      else if (width == 16)
        t = kSimd16;
      else if (width == 32)
        t = kSimd32;
      m_cached_assemblies[i.second][t] = m_buf;
      m_buf = "";
      return;
    }
  }
  m_buf = "";
}

void
ShaderCallback::nir_handler(GLsizei length,
                            const GLchar *message) {
  if (handle_partial_message(length, message))
    return;

  static const std::map<std::string, ShaderType> tokens =
      {{"NIR (SSA form) for vertex shader", kVertex},
       {"NIR (SSA form) for fragment shader", kFragment},
       {"NIR (SSA form) for tessellation control shader", kTessControl},
       {"NIR (SSA form) for tessellation evaluation shader", kTessEval},
       {"NIR (SSA form) for geometry shader", kGeometry},
       {"NIR (SSA form) for compute shader", kCompute},
       {"NIR for VS program ", kVertex},
       {"NIR for FS program ", kFragment},
       {"NIR for TCS program ", kTessControl},
       {"NIR for TES program ", kTessEval},
       {"NIR for GS program ", kGeometry},
       {"NIR for CS program ", kCompute}};
  for (auto i : tokens) {
    if (m_buf.compare(0, i.first.length(), i.first) == 0) {
      m_cached_assemblies[i.second][kNirSsa] = m_buf;
      m_buf = "";
      return;
    }
  }
  m_buf = "";
}

void
ShaderCallback::nir_final_handler(GLsizei length,
                                  const GLchar *message) {
  if (handle_partial_message(length, message))
    return;

  static const std::map<std::string, ShaderType> tokens =
      {{"NIR (final form) for vertex shader", kVertex},
       {"NIR (final form) for fragment shader", kFragment},
       {"NIR (final form) for tessellation control shader", kTessControl},
       {"NIR (final form) for tessellation evaluation shader", kTessEval},
       {"NIR (final form) for geometry shader", kGeometry},
       {"NIR (final form) for compute shader", kCompute}};
  for (auto i : tokens) {
    if (m_buf.compare(0, i.first.length(), i.first) == 0) {
      m_cached_assemblies[i.second][kNirFinal] = m_buf;
      m_buf = "";
      return;
    }
  }
  m_buf = "";
}

void
ShaderCallback::_callback(GLenum source, GLenum type, GLuint id,
                          GLenum severity, GLsizei length,
                          const GLchar *message) {
  auto i = m_handlers.find(id);
  if (i == m_handlers.end()) {
    // find handler
    find_handler(id, length, message);
    i = m_handlers.find(id);
  }
  auto func = i->second;
  (this->*func)(length, message);
}

bool
ShaderCallback::handle_partial_message(GLsizei length,
                                       const GLchar *message) {
  m_buf += message;
  if (length >= m_max_len - 1) {
    return true;
  }
  return false;
}

ShaderCallback::~ShaderCallback() {}

void
ShaderCallback::poll(int current_program, StateTrack *cb) {
  m_current_program = current_program;
  for (auto i : m_cached_assemblies) {
    for (auto j : i.second) {
        cb->onAssembly(i.first, j.first, j.second);
    }
  }
  m_cached_assemblies.clear();
}
