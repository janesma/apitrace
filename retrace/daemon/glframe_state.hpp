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

#ifndef _GLFRAME_STATE_HPP_
#define _GLFRAME_STATE_HPP_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "glframe_retrace_interface.hpp"
#include "retrace.hpp"

namespace trace {
class Call;
}

namespace glretrace {
class OnFrameRetrace;

class StateTrack;
class OutputPoller {
 public:
  virtual void poll(int current_program, StateTrack *cb) = 0;
  virtual void pollBatch(SelectionId selectionCount,
                         ExperimentId experimentCount,
                         RenderId id,
                         OnFrameRetrace *cb) = 0;
  virtual ~OutputPoller() {}
  virtual void init() = 0;
};

enum ShaderType {
  kShaderTypeUnknown,
  kVertex,
  kFragment,
  kTessEval,
  kTessControl,
  kGeometry,
  kCompute
};

enum AssemblyType {
  kAssemblyTypeUnknown,
  kOriginal,
  kBeforeUnification,
  kAfterUnification,
  kBeforeOptimization,
  kConstCoalescing,
  kGenIrLowering,
  kLayout,
  kOptimized,
  kPushAnalysis,
  kCodeHoisting,
  kCodeSinking,
  kSimd8,
  kSimd16,
  kSimd32,
  kIr,
  kNirSsa,
  kNirFinal
};

// tracks subset of gl state for frameretrace purposes
class StateTrack {
 public:
  explicit StateTrack(OutputPoller *p);
  ~StateTrack() {}
  void track(const trace::Call &call);
  void flush();
  int CurrentProgram() const { return current_program; }
  const ShaderAssembly &currentVertexShader() const;
  const ShaderAssembly &currentFragmentShader() const;
  const ShaderAssembly &currentTessControlShader() const;
  const ShaderAssembly &currentTessEvalShader() const;
  const ShaderAssembly &currentGeomShader() const;
  const ShaderAssembly &currentCompShader() const;
  void onAssembly(ShaderType st, AssemblyType at, const std::string &assembly);
  int useProgram(int orig_program,
                 const std::string &vs, const std::string &fs,
                 const std::string &tessControl, const std::string &tessEval,
                 const std::string &geom, const std::string &comp,
                 std::string *message = NULL);
  void useProgram(int program);
  void retraceProgramSideEffects(int orig_program, trace::Call *c,
                                 retrace::Retracer *retracer) const;
  static void useProgramGL(int program);

 private:
  class TrackMap {
   public:
    TrackMap();
    bool track(StateTrack *tracker, const trace::Call &call);
   private:
    typedef void (glretrace::StateTrack::*MemberFunType)(const trace::Call&);
    std::map <std::string, MemberFunType> lookup;
  };
  static TrackMap lookup;
  class ProgramKey {
   public:
    ProgramKey(int orig_progam,
               const std::string &v, const std::string &f,
               const std::string &t_c, const std::string &t_e,
               const std::string &geom, const std::string &comp);
    bool operator<(const ProgramKey &o) const;
   private:
    int orig;
    std::string vs, fs, tess_control, tess_eval, geom, comp;
  };

  void parse();
  void trackCreateProgram(const trace::Call &);
  void trackAttachShader(const trace::Call &);
  void trackCreateShader(const trace::Call &);
  void trackShaderSource(const trace::Call &);
  void trackLinkProgram(const trace::Call &);
  void trackUseProgram(const trace::Call &);
  void trackDeleteProgram(const trace::Call &);
  void trackBindAttribLocation(const trace::Call &);
  void trackGetAttribLocation(const trace::Call &);
  void trackGetUniformLocation(const trace::Call &);
  void trackGetUniformBlockIndex(const trace::Call &);
  void trackUniformBlockBinding(const trace::Call &);
  void trackBindFragDataLocation(const trace::Call &);

  OutputPoller *m_poller;
  int current_program;
  std::map<int, std::string> shader_to_source;
  std::map<int, int> shader_to_type;
  std::map<std::string, int> source_to_shader;
  std::map<ProgramKey, int> m_sources_to_program;
  std::map<int, std::map<int, std::string>> m_program_to_bound_attrib;
  std::map<int, std::map<int, std::string>> m_program_to_uniform_name;
  std::map<int, std::map<std::string, int>> m_program_to_uniform_block_index;
  std::map<int, std::map<std::string, int>> m_program_to_frag_data_location;
  // key is program, internal key is index, value is binding
  std::map<int, std::map<int, int>> m_program_to_uniform_block_binding;

  // for these maps, key is program
  std::map<int, ShaderAssembly> program_to_vertex;
  std::map<int, ShaderAssembly> program_to_fragment;
  std::map<int, ShaderAssembly> program_to_tess_control;
  std::map<int, ShaderAssembly> program_to_tess_eval;
  std::map<int, ShaderAssembly> program_to_geom;
  std::map<int, ShaderAssembly> program_to_comp;

  std::map<int, int> vertex_to_program;
  std::map<int, int> fragment_to_program;
  std::map<int, int> tess_control_to_program;
  std::map<int, int> tess_eval_to_program;
  std::map<int, int> geom_to_program;
  std::map<int, int> comp_to_program;

  const ShaderAssembly empty_shader;

  // any call with program side effects (eg glUniform) needs to be
  // executed on replacement programs (eg rendertarget highlighting)
  std::map<int, std::vector<int>> program_to_replacements;
};
}  // namespace glretrace
#endif

