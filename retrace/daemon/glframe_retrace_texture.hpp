/**************************************************************************
 *
 * Copyright 2019 Intel Corporation
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

#ifndef _GLFRAME_RETRACE_TEXTURE_HPP_
#define _GLFRAME_RETRACE_TEXTURE_HPP_

#include <map>
#include <string>
#include <vector>

#include "glframe_retrace_interface.hpp"

namespace glretrace {

class Textures {
 public:
  void retraceTextures(RenderId render,
                       SelectionId selectionCount,
                       ExperimentId experimentCount,
                       OnFrameRetrace *callback);
  void onTextureData(ExperimentId experimentCount,
                     const std::string &md5sum,
                     const std::vector<unsigned char> &image);
 private:
  std::map<std::string, bool> m_cached_textures;
  ExperimentId m_last_experiment;
  OnFrameRetrace *m_callback;
};

}  // namespace glretrace

#endif  // _GLFRAME_RETRACE_TEXTURE_HPP_
