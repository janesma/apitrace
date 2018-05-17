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

#include "glframe_texture_override.hpp"

#include "glframe_glhelper.hpp"

using glretrace::CheckError;
using glretrace::TextureOverride;

TextureOverride::TextureOverride(unsigned tex2x2)
      : m_is2x2(false),
        m_tex2x2(tex2x2) {
  // iterate over the active texture units, and save the active texture
  GlFunctions::GetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_texture_units);
  int prev_active_texture_unit;
  GlFunctions::GetIntegerv(GL_ACTIVE_TEXTURE, &prev_active_texture_unit);
  GL_CHECK();

  for (int i = GL_TEXTURE0; i < m_texture_units + GL_TEXTURE0; ++i) {
    int prev_bound_texture;
    GlFunctions::ActiveTexture(i);
    GlFunctions::GetIntegerv(GL_TEXTURE_BINDING_2D, &prev_bound_texture);
    GL_CHECK();
    m_texture_unit_to_texture_id[i] = prev_bound_texture;
  }
  GlFunctions::ActiveTexture(prev_active_texture_unit);
}

void
TextureOverride::overrideTexture() const {
  if (!m_is2x2)
    return;
  int prev_active_texture_unit;
  GlFunctions::GetIntegerv(GL_ACTIVE_TEXTURE, &prev_active_texture_unit);
  GL_CHECK();
  for (int i = GL_TEXTURE0; i < GL_TEXTURE0 + m_texture_units; ++i) {
    GlFunctions::ActiveTexture(i);
    GL_CHECK();
    GlFunctions::BindTexture(GL_TEXTURE_2D, m_tex2x2);
    GL_CHECK();
  }
  GlFunctions::ActiveTexture(prev_active_texture_unit);
  GL_CHECK();
}

void
TextureOverride::restoreTexture() const {
  if (!m_is2x2)
    return;
  // iterate over the active texture units, and restore the saved texture
  int prev_active_texture_unit;
  GlFunctions::GetIntegerv(GL_ACTIVE_TEXTURE, &prev_active_texture_unit);
  GL_CHECK();
  for (int i = GL_TEXTURE0; i < GL_TEXTURE0 + m_texture_units; ++i) {
    auto t = m_texture_unit_to_texture_id.find(i);
    assert(t != m_texture_unit_to_texture_id.end());
    GlFunctions::ActiveTexture(i);
    GL_CHECK();
    GlFunctions::BindTexture(GL_TEXTURE_2D, t->second);
    GL_CHECK();
  }
  GlFunctions::ActiveTexture(prev_active_texture_unit);
  GL_CHECK();
}

