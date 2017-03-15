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

#include <sstream>
#include <string>

#include "glframe_stderr.hpp"
#include "glframe_logger.hpp"

using glretrace::StdErrRedirect;

StdErrRedirect::StdErrRedirect() {
}

void
StdErrRedirect::poll(int current_program, StateTrack *cb) {
  std::string fs_ir, fs_simd8, fs_simd16, vs_ir, vs_simd8, line,
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

  if (tess_eval_ir.length() > 0)
    cb->onAssembly(kTessEval, kIr, tess_eval_ir);
  if (tess_eval_ssa.length() > 0)
    cb->onAssembly(kTessEval, kNirSsa, tess_eval_ssa);
  if (tess_eval_final.length() > 0)
    cb->onAssembly(kTessEval, kNirFinal, tess_eval_final);
  if (tess_eval_simd8.length() > 0)
    cb->onAssembly(kTessEval, kSimd8, tess_eval_simd8);

  if (tess_control_ir.length() > 0)
    cb->onAssembly(kTessControl, kIr, tess_control_ir);
  if (tess_control_ssa.length() > 0)
    cb->onAssembly(kTessControl, kNirSsa, tess_control_ssa);
  if (tess_control_final.length() > 0)
    cb->onAssembly(kTessControl, kNirFinal, tess_control_final);
  if (tess_control_simd8.length() > 0)
    cb->onAssembly(kTessControl, kSimd8, tess_control_simd8);

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
  setenv("vblank_mode", "0", 1);
  pipe2(out_pipe, O_NONBLOCK);
  fcntl(out_pipe[1], F_SETPIPE_SZ, 1048576);
  dup2(out_pipe[1], STDERR_FILENO);
  close(out_pipe[1]);
  buf.resize(1024);
}
