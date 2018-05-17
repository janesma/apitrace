// Copyright (C) Intel Corp.  2018.  All Rights Reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice (including the
// next paragraph) shall be included in all copies or substantial
// portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  **********************************************************************/
//  * Authors:
//  *   Mark Janes <mark.a.janes@intel.com>
//  **********************************************************************/

#ifndef _GLFRAME_TEXTURE_OVERRIDE_HPP__
#define _GLFRAME_TEXTURE_OVERRIDE_HPP__

#include <map>

#include "glframe_retrace_interface.hpp"

namespace glretrace {

class TextureOverride {
 public:
  explicit TextureOverride(unsigned tex2x2);
  void set2x2(bool enable) { m_is2x2 = enable; }
  void overrideTexture() const;
  void restoreTexture() const;
  void revertExperiments() { m_is2x2 = false; }

 private:
  bool m_is2x2;
  int m_tex2x2, m_texture_units;
  std::map<int, int> m_texture_unit_to_texture_id;
};

}  // namespace glretrace

#endif  //  _GLFRAME_TEXTURE_OVERRIDE_HPP__
