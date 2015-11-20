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

#ifndef _GLFRAME_STATE_HPP_
#define _GLFRAME_STATE_HPP_

#include <map>
#include <string>
#include <vector>

namespace trace {
class Call;
}

namespace glretrace {

class OutputPoller {
 public:
  virtual std::string poll() = 0;
  virtual ~OutputPoller() {}
};

// tracks subset of gl state for frameretrace purposes
class StateTrack {
 public:
  explicit StateTrack(OutputPoller *p) : m_poller(p) {}
  ~StateTrack() {}
  void track(const trace::Call &call);
  void flush();
  void reset() { current_program = 0; }
  int CurrentProgram() const { return current_program; }
  std::string currentVertexShader() const;
  std::string currentVertexIr() const;
  std::string currentVertexVec4() const;
  std::string currentFragmentShader() const;
  std::string currentFragmentIr() const;
  std::string currentFragmentSimd8() const;
  std::string currentFragmentSimd16() const;
  std::string currentFragmentSSA() const;
  std::string currentFragmentNIR() const;
  uint64_t currentContext() const { return current_context; }
  int useProgram(const std::string &vs, const std::string &fs,
                 std::string *message = NULL);
  void useProgram(int program);

 private:
  class TrackMap {
   public:
    TrackMap();
    void track(StateTrack *tracker, const trace::Call &call);
   private:
    typedef void (glretrace::StateTrack::*MemberFunType)(const trace::Call&);
    std::map <std::string, MemberFunType> lookup;
  };
  static TrackMap lookup;
  class ProgramKey {
   public:
    ProgramKey(const std::string &v, const std::string &f);
    bool operator<(const ProgramKey &o) const;
   private:
    std::string vs; std::string fs;
  };

  void parse();
  void trackAttachShader(const trace::Call &);
  void trackCreateShader(const trace::Call &);
  void trackShaderSource(const trace::Call &);
  void trackLinkProgram(const trace::Call &);
  void trackUseProgram(const trace::Call &);

  OutputPoller *m_poller;
  int current_program;
  uint64_t current_context;
  std::map<int, std::string> shader_to_source;
  std::map<int, int> shader_to_type;
  std::map<std::string, int> source_to_shader;
  std::map<ProgramKey, int> m_sources_to_program;

  // for these maps, key is program
  std::map<int, std::string> program_to_vertex_shader_source;
  std::map<int, std::string> program_to_vertex_shader_ir;
  std::map<int, std::string> program_to_vertex_shader_vec4;
  std::map<int, std::string> program_to_fragment_shader_source;
  std::map<int, std::string> program_to_fragment_shader_ir;
  std::map<int, std::string> program_to_fragment_shader_simd8;
  std::map<int, std::string> program_to_fragment_shader_simd16;
  std::map<int, std::string> program_to_fragment_shader_ssa;
  std::map<int, std::string> program_to_fragment_shader_nir;
};
}  // namespace glretrace
#endif

