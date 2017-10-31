// Copyright (C) Intel Corp.  2017.  All Rights Reserved.

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

#include "glframe_state_override.hpp"

#include <string>
#include <vector>

#include "glframe_glhelper.hpp"
#include "glframe_state_enums.hpp"

using glretrace::StateOverride;

union IntFloat {
  uint32_t i;
  float f;
};

void
StateOverride::setState(const StateKey &item,
                        GLint value) {
  auto &i = m_overrides[item];
  if (i.empty()) {
    // save the prior state so we can restore it
    getState(item, &i);
    m_saved_state[item] = i;
  }
  assert(i.size() == 1);
  i[0] = value;
}

void
StateOverride::setState(const StateKey &item,
                        int offset,
                        float value) {
  m_data_types[item] = kFloat;
  auto &i = m_overrides[item];
  if (i.empty()) {
    // save the prior state so we can restore it
    getState(item, &i);
    m_saved_state[item] = i;
  }
  IntFloat u;
  u.f = value;
  i[offset] = u.i;
}

void
StateOverride::getState(const StateKey &item,
                        std::vector<uint32_t> *data) {
  const auto n = state_name_to_enum(item.name);
  switch (n) {
    case GL_CULL_FACE:
    case GL_LINE_SMOOTH:
    case GL_BLEND: {
      get_enabled_state(n, data);
      break;
    }
    case GL_CULL_FACE_MODE:
    case GL_BLEND_SRC:
    case GL_BLEND_SRC_ALPHA:
    case GL_BLEND_SRC_RGB:
    case GL_BLEND_DST:
    case GL_BLEND_DST_ALPHA:
    case GL_BLEND_DST_RGB: {
      get_integer_state(n, data);
      break;
    }
    case GL_BLEND_COLOR: {
      get_float_state(n, data);
      break;
    }
    case GL_LINE_WIDTH: {
      get_float_state(n, data);
      break;
    }
  }
}

void
StateOverride::get_enabled_state(GLint k,
                                 std::vector<uint32_t> *data) {
  GL::GetError();
  data->clear();
  data->resize(1);
  (*data)[0] = GlFunctions::IsEnabled(k);
  assert(!GL::GetError());
}


void
StateOverride::get_integer_state(GLint k,
                                 std::vector<uint32_t> *data) {
  GL::GetError();
  data->clear();
  data->resize(1);
  GlFunctions::GetIntegerv(k, reinterpret_cast<GLint*>(data->data()));
  assert(!GL::GetError());
}

void
StateOverride::get_float_state(GLint k, std::vector<uint32_t> *data) {
  GL::GetError();
  data->clear();
  data->resize(4);
  GlFunctions::GetFloatv(k, reinterpret_cast<GLfloat*>(data->data()));
  assert(!GL::GetError());
}

void
StateOverride::overrideState() const {
  enact_state(m_overrides);
}

void
StateOverride::restoreState() const {
  enact_state(m_saved_state);
}

void
StateOverride::enact_enabled_state(GLint setting, bool v) const {
  if (v)
    GlFunctions::Enable(setting);
  else
    GlFunctions::Disable(setting);
  assert(GL::GetError() == GL_NO_ERROR);
}

void
StateOverride::enact_state(const KeyMap &m) const {
  GL::GetError();
  for (auto i : m) {
    GL::GetError();
    const auto n = state_name_to_enum(i.first.name);
    switch (n) {
      case GL_CULL_FACE:
      case GL_BLEND:
      case GL_LINE_SMOOTH: {
        enact_enabled_state(n, i.second[0]);
        break;
      }
      case GL_CULL_FACE_MODE: {
        GlFunctions::CullFace(i.second[0]);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_BLEND_SRC:
      case GL_BLEND_DST: {
        GLint src, dst;
        GlFunctions::GetIntegerv(GL_BLEND_DST, &dst);
        GlFunctions::GetIntegerv(GL_BLEND_SRC, &src);
        assert(GL::GetError() == GL_NO_ERROR);
        GlFunctions::BlendFunc(
            n == GL_BLEND_SRC ? i.second[0] : src,
            n == GL_BLEND_DST ? i.second[0] : dst);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_BLEND_SRC_RGB:
      case GL_BLEND_DST_RGB:
      case GL_BLEND_SRC_ALPHA:
      case GL_BLEND_DST_ALPHA:
        {
          GLint src_rgb, dst_rgb, src_alpha, dst_alpha;
          GlFunctions::GetIntegerv(GL_BLEND_SRC_RGB, &src_rgb);
          GlFunctions::GetIntegerv(GL_BLEND_DST_RGB, &dst_rgb);
          GlFunctions::GetIntegerv(GL_BLEND_SRC_ALPHA, &src_alpha);
          GlFunctions::GetIntegerv(GL_BLEND_DST_ALPHA, &dst_alpha);
          GlFunctions::BlendFuncSeparate(
              n == GL_BLEND_SRC_RGB ? i.second[0] : src_rgb,
              n == GL_BLEND_DST_RGB ? i.second[0] : dst_rgb,
              n == GL_BLEND_SRC_ALPHA ? i.second[0] : src_alpha,
              n == GL_BLEND_DST_ALPHA ? i.second[0] : dst_alpha);
          break;
        }
      case GL_BLEND_COLOR: {
        std::vector<float> f(4);
        IntFloat u;
        for (int j = 0; j < 4; ++j) {
          u.i = i.second[j];
          f[j] = u.f;
        }
        GlFunctions::BlendColor(f[0], f[1], f[2], f[3]);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_LINE_WIDTH: {
        // assert(i.second.size() == 1);
        IntFloat u;
        u.i = i.second[0];
        GlFunctions::LineWidth(u.f);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_INVALID_ENUM:
      default:
        assert(false);
        break;
    }
  }
}

void floatStrings(const std::vector<uint32_t> &i,
                  std::vector<std::string> *s) {
  s->clear();
  IntFloat u;
  for (auto d : i) {
    u.i = d;
    s->push_back(std::to_string(u.f));
  }
}

void floatString(const uint32_t i,
                 std::string *s) {
  IntFloat u;
  u.i = i;
  *s = std::to_string(u.f);
}

void
StateOverride::onState(SelectionId selId,
                       ExperimentId experimentCount,
                       RenderId renderId,
                       OnFrameRetrace *callback) {
  std::vector<uint32_t> data;
  {
    StateKey k("Rendering", "Cull State", "GL_CULL_FACE");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {data[0] ? "true" : "false"});
  }
  {
    StateKey k("Rendering", "Cull State", "GL_CULL_FACE_MODE");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  {
    StateKey k("Rendering", "Blend State", "GL_BLEND");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {data[0] ? "true" : "false"});
  }
  {
    StateKey k("Rendering", "Blend State", "GL_BLEND_SRC");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  {
    StateKey k("Rendering", "Blend State", "GL_BLEND_SRC_ALPHA");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  {
    StateKey k("Rendering", "Blend State", "GL_BLEND_SRC_RGB");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  {
    StateKey k("Rendering", "Blend State", "GL_BLEND_DST");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  {
    StateKey k("Rendering", "Blend State", "GL_BLEND_DST_ALPHA");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  {
    StateKey k("Rendering", "Blend State", "GL_BLEND_DST_RGB");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  {
    StateKey k("Rendering", "Blend State", "GL_BLEND_COLOR");
    getState(k, &data);
    std::vector<std::string> color;
    floatStrings(data, &color);
    callback->onState(selId, experimentCount, renderId, k, color);
  }
  {
    StateKey k("Rendering", "Line State", "GL_LINE_WIDTH");
    getState(k, &data);
    std::string value;
    floatString(data[0], &value);
    callback->onState(selId, experimentCount, renderId, k, {value});
  }
  {
    StateKey k("Rendering", "Line State", "GL_LINE_SMOOTH");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {data[0] ? "true" : "false"});
  }
}
