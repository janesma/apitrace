/**************************************************************************
 *
 * Copyright 2017 Intel Corporation
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

#ifndef _GLFRAME_UNIFORMS_HPP_
#define _GLFRAME_UNIFORMS_HPP_

#include <string>
#include <vector>

#include "glframe_retrace_interface.hpp"

namespace glretrace {

class Uniform;
class Uniforms {
  // Holds all uniform constants at a specified render.  Enables
  // uniform handling during shader replacement, and setting uniform
  // values for IFrameRetrace::setUniforms
 public:
  Uniforms();
  ~Uniforms();
  void set() const;
  void onUniform(SelectionId selectionCount,
                 ExperimentId experimentCount,
                 RenderId renderId,
                 OnFrameRetrace *callback) const;
  void overrideUniform(const std::string &name,
                       int index,
                       const std::string &value);
 private:
  std::vector<Uniform*> m_uniforms;
};
}  // namespace glretrace

#endif  // _GLFRAME_UNIFORMS_HPP_
