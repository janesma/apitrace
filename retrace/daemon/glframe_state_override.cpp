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


// **********************************************************************/
// Checklist for enabling state overrides:
//  - in dispatch/glframe_glhelper.hpp
//    - add entry point for required GL calls in the header
//    - glframe_glhelper.cpp
//      - declare static pointer to GL function
//      - look up function pointer in ::Init
//      - implement entry point to call GL function at the pointer
//  - in glframe_state_enums.cpp:
//    - for overrides with choices, add all possibilities to
//      state_name_to_choices (including true/false state)
//    - add all enum values in state_name_to_enum
//    - add all enum values in state_enum_to_name
//    - if state item has multiple values (eg ClearColor value is
//      RGBA), define the indices that should be displayed for each
//      value in state_name_to_indices.
//  - in glframe_state_override.cpp
//    - add entries to ::interpret_value, which converts a string
//      representation of the items value into the bytes sent to the
//      GL.  Internally, StateOverride holds the data as uint32_t, and
//      converts them to the appropriate type before calling the GL.
//    - add entries to ::getState, calling the GL to get the current
//      state of the item, and converting into bytes uint32_t.
//    - add entries to ::enact_state, which calls the GL to set state
//      according to what is stored in StateOverride.  Convert the
//      bytes from uint32_t to whatever is required by the GL api
//      before calling the GL entry point.
//    - add entries to ::onState, which queries current state from the
//      GL and makes the onState callback to pass data back to the UI.
//      For each entry, choose a hierarchical path to help organize
//      the state in the UI.
// **********************************************************************/

#include "glframe_state_override.hpp"

#include <stdio.h>

#include <map>
#include <string>
#include <vector>

#include "glframe_glhelper.hpp"
#include "glframe_state_enums.hpp"

using glretrace::ExperimentId;
using glretrace::OnFrameRetrace;
using glretrace::RenderId;
using glretrace::SelectionId;
using glretrace::StateOverride;
using glretrace::StateKey;

union IntFloat {
  uint32_t i;
  float f;
};

void
StateOverride::setState(const StateKey &item,
                        int offset,
                        const std::string &value) {
  StateKey adjusted_item(item);
  adjust_item(&adjusted_item);
  auto &i = m_overrides[adjusted_item];
  if (i.empty()) {
    // save the prior state so we can restore it
    getState(adjusted_item, &i);
    m_saved_state[adjusted_item] = i;
  }
  adjust_offset(item, &offset);
  const uint32_t data_value = interpret_value(item, value);
  i[offset] = data_value;
}

// Not all StateKey items from the UI will match the data model for
// state overrides during retrace.  For example, GL_COLOR_WRITEMASK is
// stored in 4 separate items in the UI, but a single override in this
// model.  The reason for this discrepancy is to allow mapping of
// model offsets to UI colors (0->red, 1->green, etc).  This method
// interprets a StateKey from the UI and adjusts it to match the
// model.
void
StateOverride::adjust_item(StateKey *item) const {
  // initially required by GL_COLOR_WRITEMASK, which can now be
  // implemented within the generic pattern.
  return;
}

// As with adjust_item, offsets from the UI do not always match the
// model used by this override.  This method interprets an offset from
// the UI, and adjusts it to match the model implementation.
void
StateOverride::adjust_offset(const StateKey &item,
                             int *offset) const {
  // initially required by GL_COLOR_WRITEMASK, which can now be
  // implemented within the generic pattern.
  return;
}

// The UI uses strings for all state values.  The override model
// stores all data types in a vector of uint32_t.
uint32_t
StateOverride::interpret_value(const StateKey &item,
                               const std::string &value) {
  switch (state_name_to_enum(item.name)) {
    // enumeration values
    // true/false values
    case GL_BLEND:
    case GL_BLEND_DST:
    case GL_BLEND_DST_ALPHA:
    case GL_BLEND_DST_RGB:
    case GL_BLEND_EQUATION_ALPHA:
    case GL_BLEND_EQUATION_RGB:
    case GL_BLEND_SRC:
    case GL_BLEND_SRC_ALPHA:
    case GL_BLEND_SRC_RGB:
    case GL_COLOR_WRITEMASK:
    case GL_CULL_FACE:
    case GL_CULL_FACE_MODE:
    case GL_LINE_SMOOTH:
    case GL_DEPTH_FUNC:
    case GL_DEPTH_TEST:
    case GL_DEPTH_WRITEMASK:
    case GL_DITHER:
    case GL_FRONT_FACE:
    case GL_POLYGON_OFFSET_FILL:
    case GL_SAMPLE_COVERAGE_INVERT:
    case GL_SCISSOR_TEST:
    case GL_STENCIL_BACK_FAIL:
    case GL_STENCIL_BACK_FUNC:
    case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
    case GL_STENCIL_BACK_PASS_DEPTH_PASS:
    case GL_STENCIL_FAIL:
    case GL_STENCIL_FUNC:
    case GL_STENCIL_PASS_DEPTH_FAIL:
    case GL_STENCIL_PASS_DEPTH_PASS:
    case GL_STENCIL_TEST:
      return state_name_to_enum(value);

    // float values
    case GL_BLEND_COLOR:
    case GL_COLOR_CLEAR_VALUE:
    case GL_DEPTH_CLEAR_VALUE:
    case GL_DEPTH_RANGE:
    case GL_LINE_WIDTH:
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_SAMPLE_COVERAGE_VALUE:
      {
      IntFloat i_f;
      i_f.f = std::stof(value);
      return i_f.i;
    }

    // int values
    case GL_SCISSOR_BOX:
      return std::stoi(value);

    // hex values
    case GL_STENCIL_BACK_REF:
    case GL_STENCIL_BACK_VALUE_MASK:
    case GL_STENCIL_BACK_WRITEMASK:
    case GL_STENCIL_CLEAR_VALUE:
    case GL_STENCIL_REF:
    case GL_STENCIL_VALUE_MASK:
    case GL_STENCIL_WRITEMASK:
      return std::stoul(value, 0, 16);

    default:
      assert(false);
      return 0;
  }
}

enum StateCategory {
  kStateEnabled,
  kStateBoolean,
  kStateBoolean4,
  kStateInteger,
  kStateInteger4,
  kStateFloat,
  kStateFloat2,
  kStateFloat4,
  kStateInvalid
};

StateCategory
state_type(uint32_t state) {
  switch (state) {
    case GL_BLEND:
    case GL_CULL_FACE:
    case GL_DEPTH_TEST:
    case GL_DITHER:
    case GL_LINE_SMOOTH:
    case GL_POLYGON_OFFSET_FILL:
    case GL_SCISSOR_TEST:
    case GL_STENCIL_TEST:
      return kStateEnabled;

    case GL_BLEND_DST:
    case GL_BLEND_DST_ALPHA:
    case GL_BLEND_DST_RGB:
    case GL_BLEND_EQUATION_ALPHA:
    case GL_BLEND_EQUATION_RGB:
    case GL_BLEND_SRC:
    case GL_BLEND_SRC_ALPHA:
    case GL_BLEND_SRC_RGB:
    case GL_CULL_FACE_MODE:
    case GL_DEPTH_FUNC:
    case GL_FRONT_FACE:
    case GL_STENCIL_BACK_FAIL:
    case GL_STENCIL_BACK_FUNC:
    case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
    case GL_STENCIL_BACK_PASS_DEPTH_PASS:
    case GL_STENCIL_BACK_REF:
    case GL_STENCIL_BACK_VALUE_MASK:
    case GL_STENCIL_BACK_WRITEMASK:
    case GL_STENCIL_CLEAR_VALUE:
    case GL_STENCIL_FAIL:
    case GL_STENCIL_FUNC:
    case GL_STENCIL_PASS_DEPTH_FAIL:
    case GL_STENCIL_PASS_DEPTH_PASS:
    case GL_STENCIL_REF:
    case GL_STENCIL_VALUE_MASK:
    case GL_STENCIL_WRITEMASK:
      return kStateInteger;

    case GL_COLOR_WRITEMASK:
      return kStateBoolean4;

    case GL_DEPTH_WRITEMASK:
    case GL_SAMPLE_COVERAGE_INVERT:
      return kStateBoolean;

    case GL_BLEND_COLOR:
    case GL_COLOR_CLEAR_VALUE:
      return kStateFloat4;

    case GL_DEPTH_CLEAR_VALUE:
    case GL_LINE_WIDTH:
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_SAMPLE_COVERAGE_VALUE:
      return kStateFloat;

    case GL_DEPTH_RANGE:
      return kStateFloat2;

    case GL_SCISSOR_BOX:
      return kStateFloat4;
  }
  assert(false);
  return kStateInvalid;
}

void
StateOverride::getState(const StateKey &item,
                        std::vector<uint32_t> *data) {
  const auto n = state_name_to_enum(item.name);
  data->clear();
  switch (state_type(n)) {
    case kStateEnabled: {
      data->resize(1);
      get_enabled_state(n, data);
      break;
    }
    case kStateInteger: {
      data->resize(1);
      get_integer_state(n, data);
      break;
    }
    case kStateBoolean4: {
        data->resize(4);
        get_bool_state(n, data);
        break;
      }
    case kStateBoolean: {
        data->resize(1);
        get_bool_state(n, data);
        break;
      }
    case kStateFloat4: {
      data->resize(4);
      get_float_state(n, data);
      break;
    }
    case kStateFloat: {
        data->resize(1);
        get_float_state(n, data);
        break;
      }
    case kStateFloat2: {
      data->resize(2);
      get_float_state(n, data);
      break;
    }
    case kStateInteger4: {
      data->resize(4);
      get_integer_state(n, data);
      break;
    }
    case kStateInvalid:
      assert(false);
      break;
  }
}

void
StateOverride::get_enabled_state(GLint k,
                                 std::vector<uint32_t> *data) {
  GL::GetError();
  assert(data->size() == 1);
  (*data)[0] = GlFunctions::IsEnabled(k);
  assert(!GL::GetError());
}


void
StateOverride::get_integer_state(GLint k,
                                 std::vector<uint32_t> *data) {
  GL::GetError();
  GlFunctions::GetIntegerv(k, reinterpret_cast<GLint*>(data->data()));
  assert(!GL::GetError());
}

void
StateOverride::get_bool_state(GLint k,
                              std::vector<uint32_t> *data) {
  GL::GetError();
  std::vector<GLboolean> b(data->size());
  GlFunctions::GetBooleanv(k, b.data());
  auto d = data->begin();
  for (auto v : b) {
    *d = v ? 1 : 0;
    ++d;
  }
  assert(!GL::GetError());
}

void
StateOverride::get_float_state(GLint k, std::vector<uint32_t> *data) {
  GL::GetError();
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
      case GL_BLEND:
      case GL_CULL_FACE:
      case GL_DEPTH_TEST:
      case GL_DITHER:
      case GL_LINE_SMOOTH:
      case GL_POLYGON_OFFSET_FILL:
      case GL_SCISSOR_TEST:
      case GL_STENCIL_TEST: {
        enact_enabled_state(n, i.second[0]);
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
      case GL_BLEND_EQUATION_ALPHA:
      case GL_BLEND_EQUATION_RGB: {
        GLint modeRGB, modeAlpha;
        GlFunctions::GetIntegerv(GL_BLEND_EQUATION_RGB, &modeRGB);
        GlFunctions::GetIntegerv(GL_BLEND_EQUATION_ALPHA, &modeAlpha);
        GlFunctions::BlendEquationSeparate(
            n == GL_BLEND_EQUATION_RGB ? i.second[0] : modeRGB,
            n == GL_BLEND_EQUATION_ALPHA ? i.second[0] : modeAlpha);
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
      case GL_BLEND_DST_ALPHA:
      case GL_BLEND_DST_RGB:
      case GL_BLEND_SRC_ALPHA:
      case GL_BLEND_SRC_RGB: {
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
      case GL_COLOR_CLEAR_VALUE: {
        std::vector<float> f(4);
        IntFloat u;
        for (int j = 0; j < 4; ++j) {
          u.i = i.second[j];
          f[j] = u.f;
        }
        GlFunctions::ClearColor(f[0], f[1], f[2], f[3]);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_COLOR_WRITEMASK: {
        GlFunctions::ColorMask(i.second[0],
                               i.second[1],
                               i.second[2],
                               i.second[3]);
        break;
      }
      case GL_CULL_FACE_MODE: {
        GlFunctions::CullFace(i.second[0]);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_DEPTH_CLEAR_VALUE: {
        IntFloat u;
        u.i = i.second[0];
        GlFunctions::ClearDepthf(u.f);
        break;
      }
      case GL_DEPTH_FUNC: {
        GlFunctions::DepthFunc(i.second[0]);
        break;
      }
      case GL_LINE_WIDTH: {
        assert(i.second.size() == 1);
        IntFloat u;
        u.i = i.second[0];
        GlFunctions::LineWidth(u.f);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_DEPTH_RANGE: {
        assert(i.second.size() == 2);
        IntFloat _near, _far;
        _near.i = i.second[0];
        _far.i = i.second[1];
        GlFunctions::DepthRangef(_near.f, _far.f);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_DEPTH_WRITEMASK: {
        GlFunctions::DepthMask(i.second[0]);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_FRONT_FACE: {
        GlFunctions::FrontFace(i.second[0]);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_POLYGON_OFFSET_FACTOR:
      case GL_POLYGON_OFFSET_UNITS: {
        GLfloat factor, units;
        GlFunctions::GetFloatv(GL_POLYGON_OFFSET_FACTOR, &factor);
        GlFunctions::GetFloatv(GL_POLYGON_OFFSET_UNITS, &units);
        IntFloat convert;
        convert.i = i.second[0];
        GlFunctions::PolygonOffset(
            n == GL_POLYGON_OFFSET_FACTOR ? convert.f : factor,
            n == GL_POLYGON_OFFSET_UNITS ? convert.f : units);
        assert(GL::GetError() == GL_NO_ERROR);
        break;
      }
      case GL_SAMPLE_COVERAGE_INVERT:
      case GL_SAMPLE_COVERAGE_VALUE: {
        GLfloat value;
        GLboolean invert;
        GlFunctions::GetFloatv(GL_SAMPLE_COVERAGE_VALUE, &value);
        GlFunctions::GetBooleanv(GL_SAMPLE_COVERAGE_INVERT, &invert);
        IntFloat convert;
        convert.i = i.second[0];
        GlFunctions::SampleCoverage(
            n == GL_SAMPLE_COVERAGE_VALUE ? convert.f : value,
            n == GL_SAMPLE_COVERAGE_INVERT ? i.second[0] : invert);
        break;
      }
      case GL_SCISSOR_BOX: {
        GlFunctions::Scissor(i.second[0], i.second[1],
                             i.second[2], i.second[3]);
        break;
      }
      case GL_STENCIL_BACK_FAIL:
      case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
      case GL_STENCIL_BACK_PASS_DEPTH_PASS: {
        GLint sfail = 0, dpfail = 0, dppass = 0;
        GlFunctions::GetIntegerv(GL_STENCIL_BACK_FAIL, &sfail);
        GlFunctions::GetIntegerv(
            GL_STENCIL_BACK_PASS_DEPTH_FAIL, &dpfail);
        GlFunctions::GetIntegerv(
            GL_STENCIL_BACK_PASS_DEPTH_PASS, &dppass);
        GlFunctions::StencilOpSeparate(
            GL_BACK,
            n == GL_STENCIL_BACK_FAIL ? i.second[0] : sfail,
            n == GL_STENCIL_BACK_PASS_DEPTH_FAIL ? i.second[0] : dpfail,
            n == GL_STENCIL_BACK_PASS_DEPTH_PASS ? i.second[0] : dppass);
        break;
      }
      case GL_STENCIL_BACK_FUNC:
      case GL_STENCIL_BACK_REF:
      case GL_STENCIL_BACK_VALUE_MASK: {
        GLint func = 0, ref = 0, mask = 0;
        GlFunctions::GetIntegerv(GL_STENCIL_BACK_FUNC, &func);
        GlFunctions::GetIntegerv(GL_STENCIL_BACK_REF, &ref);
        GlFunctions::GetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &mask);
        GlFunctions::StencilFuncSeparate(
            GL_BACK,
            n == GL_STENCIL_BACK_FUNC ? i.second[0] : func,
            n == GL_STENCIL_BACK_REF ? i.second[0] : ref,
            n == GL_STENCIL_BACK_VALUE_MASK ? i.second[0] : mask);
        break;
      }
      case GL_STENCIL_FAIL:
      case GL_STENCIL_PASS_DEPTH_FAIL:
      case GL_STENCIL_PASS_DEPTH_PASS: {
        GLint sfail, dpfail, dppass;
        GlFunctions::GetIntegerv(GL_STENCIL_FAIL, &sfail);
        GlFunctions::GetIntegerv(
            GL_STENCIL_PASS_DEPTH_FAIL, &dpfail);
        GlFunctions::GetIntegerv(
            GL_STENCIL_PASS_DEPTH_PASS, &dppass);
        GlFunctions::StencilOpSeparate(
            GL_FRONT,
            n == GL_STENCIL_FAIL ? i.second[0] : sfail,
            n == GL_STENCIL_PASS_DEPTH_FAIL ? i.second[0] : dpfail,
            n == GL_STENCIL_PASS_DEPTH_PASS ? i.second[0] : dppass);
        break;
      }
      case GL_STENCIL_FUNC:
      case GL_STENCIL_REF:
      case GL_STENCIL_VALUE_MASK: {
        GLint func = 0, ref = 0, mask = 0;
        GlFunctions::GetIntegerv(GL_STENCIL_FUNC, &func);
        GlFunctions::GetIntegerv(GL_STENCIL_REF, &ref);
        GlFunctions::GetIntegerv(GL_STENCIL_VALUE_MASK, &mask);
        GlFunctions::StencilFuncSeparate(
            GL_FRONT,
            n == GL_STENCIL_FUNC ? i.second[0] : func,
            n == GL_STENCIL_REF ? i.second[0] : ref,
            n == GL_STENCIL_VALUE_MASK ? i.second[0] : mask);
        break;
      }
      case GL_STENCIL_WRITEMASK:
      case GL_STENCIL_BACK_WRITEMASK: {
        GlFunctions::StencilMaskSeparate(
            n == GL_STENCIL_WRITEMASK ? GL_FRONT : GL_BACK,
            i.second[0]);
        break;
      }
      case GL_STENCIL_CLEAR_VALUE: {
        GlFunctions::ClearStencil(i.second[0]);
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

void intStrings(const std::vector<uint32_t> &i,
                std::vector<std::string> *s) {
  s->clear();
  for (auto d : i) {
    s->push_back(std::to_string(d));
  }
}

void floatString(const uint32_t i,
                 std::string *s) {
  IntFloat u;
  u.i = i;
  *s = std::to_string(u.f);
}

void hexString(const uint32_t i,
               std::string *s) {
  std::vector<char> hexstr(11);
  snprintf(hexstr.data(), hexstr.size(), "0x%08x", i);
  *s = hexstr.data();
}

bool
is_supported(uint32_t state) {
  glretrace::GL::GetError();
  std::vector<uint32_t> data(4);
  switch (state_type(state)) {
   case kStateEnabled:
      glretrace::GL::IsEnabled(state);
      break;
    case kStateInteger:
    case kStateInteger4:
      glretrace::GL::GetIntegerv(state, reinterpret_cast<GLint*>(data.data()));
      break;
    case kStateBoolean:
    case kStateBoolean4:
      glretrace::GL::GetBooleanv(state,
                                 reinterpret_cast<GLboolean*>(data.data()));
      break;
    case kStateFloat:
    case kStateFloat2:
    case kStateFloat4:
      glretrace::GL::GetFloatv(state, reinterpret_cast<GLfloat*>(data.data()));
      break;
    case kStateInvalid:
      assert(false);
      return false;
  }
  return(glretrace::GL::GetError() == false);
}

void
StateOverride::onState(SelectionId selId,
                       ExperimentId experimentCount,
                       RenderId renderId,
                       OnFrameRetrace *callback) {
  std::vector<uint32_t> data;

  // these entries are roughly in the order of the items in the glGet
  // man page:
  // https://www.khronos.org/registry/OpenGL-Refpages/es3.1/html/glGet.xhtml
  if (is_supported(GL_CULL_FACE)) {
    StateKey k("Primitive/Cull", "GL_CULL_FACE");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {data[0] ? "true" : "false"});
  }
  if (is_supported(GL_CULL_FACE_MODE)) {
    StateKey k("Primitive/Cull", "GL_CULL_FACE_MODE");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_BLEND)) {
    StateKey k("Fragment/Blend", "GL_BLEND");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {data[0] ? "true" : "false"});
  }
  if (is_supported(GL_BLEND_SRC)) {
    StateKey k("Fragment/Blend", "GL_BLEND_SRC");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_BLEND_SRC_ALPHA)) {
    StateKey k("Fragment/Blend", "GL_BLEND_SRC_ALPHA");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_BLEND_SRC_RGB)) {
    StateKey k("Fragment/Blend", "GL_BLEND_SRC_RGB");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_BLEND_DST)) {
    StateKey k("Fragment/Blend", "GL_BLEND_DST");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_BLEND_DST_ALPHA)) {
    StateKey k("Fragment/Blend", "GL_BLEND_DST_ALPHA");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_BLEND_DST_RGB)) {
    StateKey k("Fragment/Blend", "GL_BLEND_DST_RGB");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_BLEND_COLOR)) {
    StateKey k("Fragment/Blend", "GL_BLEND_COLOR");
    getState(k, &data);
    std::vector<std::string> color;
    floatStrings(data, &color);
    callback->onState(selId, experimentCount, renderId, k, color);
  }
  if (is_supported(GL_LINE_WIDTH)) {
    StateKey k("Primitive/Line", "GL_LINE_WIDTH");
    getState(k, &data);
    std::string value;
    floatString(data[0], &value);
    callback->onState(selId, experimentCount, renderId, k, {value});
  }
  if (is_supported(GL_LINE_SMOOTH)) {
    StateKey k("Primitive/Line", "GL_LINE_SMOOTH");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {data[0] ? "true" : "false"});
  }
  if (is_supported(GL_BLEND_EQUATION_RGB)) {
    StateKey k("Fragment/Blend", "GL_BLEND_EQUATION_RGB");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_BLEND_EQUATION_ALPHA)) {
    StateKey k("Fragment/Blend", "GL_BLEND_EQUATION_ALPHA");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_COLOR_CLEAR_VALUE)) {
    StateKey k("Framebuffer", "GL_COLOR_CLEAR_VALUE");
    getState(k, &data);
    std::vector<std::string> color;
    floatStrings(data, &color);
    callback->onState(selId, experimentCount, renderId, k, color);
  }
  if (is_supported(GL_COLOR_WRITEMASK)) {
    StateKey k("Framebuffer/Mask", "GL_COLOR_WRITEMASK");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId, k,
                      {data[0] ? "true" : "false",
                            data[1] ? "true" : "false",
                            data[2] ? "true" : "false",
                            data[3] ? "true" : "false"});
  }
  if (is_supported(GL_DEPTH_CLEAR_VALUE)) {
    StateKey k("Fragment/Depth", "GL_DEPTH_CLEAR_VALUE");
    getState(k, &data);
    std::string value;
    floatString(data[0], &value);
    callback->onState(selId, experimentCount, renderId, k, {value});
  }
  if (is_supported(GL_DEPTH_FUNC)) {
    StateKey k("Fragment/Depth", "GL_DEPTH_FUNC");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_DEPTH_RANGE)) {
    StateKey k("Fragment/Depth", "GL_DEPTH_RANGE");
    getState(k, &data);
    std::vector<std::string> range;
    floatStrings(data, &range);
    callback->onState(selId, experimentCount, renderId,
                      k, range);
  }
  if (is_supported(GL_DEPTH_TEST)) {
    StateKey k("Fragment/Depth", "GL_DEPTH_TEST");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {data[0] ? "true" : "false"});
  }
  if (is_supported(GL_DEPTH_WRITEMASK)) {
    StateKey k("Framebuffer/Mask", "GL_DEPTH_WRITEMASK");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId, k,
                      {data[0] ? "true" : "false"});
  }
  if (is_supported(GL_DITHER)) {
    StateKey k("Framebuffer", "GL_DITHER");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId, k,
                      {data[0] ? "true" : "false"});
  }
  if (is_supported(GL_FRONT_FACE)) {
    StateKey k("Primitive", "GL_FRONT_FACE");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_POLYGON_OFFSET_FACTOR)) {
    StateKey k("Primitive/Polygon", "GL_POLYGON_OFFSET_FACTOR");
    getState(k, &data);
    std::string value;
    floatString(data[0], &value);
    callback->onState(selId, experimentCount, renderId,
                      k, {value});
  }
  if (is_supported(GL_POLYGON_OFFSET_FILL)) {
    StateKey k("Primitive/Polygon", "GL_POLYGON_OFFSET_FILL");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {data[0] ? "true" : "false"});
  }
  if (is_supported(GL_POLYGON_OFFSET_UNITS)) {
    StateKey k("Primitive/Polygon", "GL_POLYGON_OFFSET_UNITS");
    getState(k, &data);
    std::string value;
    floatString(data[0], &value);
    callback->onState(selId, experimentCount, renderId,
                      k, {value});
  }
  if (is_supported(GL_SAMPLE_COVERAGE_VALUE)) {
    StateKey k("Fragment/Multisample", "GL_SAMPLE_COVERAGE_VALUE");
    getState(k, &data);
    std::string value;
    floatString(data[0], &value);
    callback->onState(selId, experimentCount, renderId,
                      k, {value});
  }
  if (is_supported(GL_SAMPLE_COVERAGE_INVERT)) {
    StateKey k("Fragment/Multisample", "GL_SAMPLE_COVERAGE_INVERT");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {data[0] ? "true" : "false"});
  }
  if (is_supported(GL_SCISSOR_TEST)) {
    StateKey k("Fragment/Scissor", "GL_SCISSOR_TEST");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {data[0] ? "true" : "false"});
  }
  if (is_supported(GL_SCISSOR_BOX)) {
    StateKey k("Fragment/Scissor", "GL_SCISSOR_BOX");
    getState(k, &data);
    std::vector<std::string> box;
    intStrings(data, &box);
    callback->onState(selId, experimentCount, renderId,
                      k, box);
  }
  if (is_supported(GL_STENCIL_BACK_FAIL)) {
    StateKey k("Stencil/Back", "GL_STENCIL_BACK_FAIL");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_STENCIL_BACK_PASS_DEPTH_FAIL)) {
    StateKey k("Stencil/Back", "GL_STENCIL_BACK_PASS_DEPTH_FAIL");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_STENCIL_BACK_PASS_DEPTH_PASS)) {
    StateKey k("Stencil/Back", "GL_STENCIL_BACK_PASS_DEPTH_PASS");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_STENCIL_BACK_FUNC)) {
    StateKey k("Stencil/Back", "GL_STENCIL_BACK_FUNC");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_STENCIL_BACK_REF)) {
    StateKey k("Stencil/Back", "GL_STENCIL_BACK_REF");
    getState(k, &data);
    std::string value;
    hexString(data[0], &value);
    callback->onState(selId, experimentCount, renderId,
                      k, {value});
  }
  if (is_supported(GL_STENCIL_BACK_VALUE_MASK)) {
    StateKey k("Stencil/Back", "GL_STENCIL_BACK_VALUE_MASK");
    getState(k, &data);
    std::string value;
    hexString(data[0], &value);
    callback->onState(selId, experimentCount, renderId,
                      k, {value});
  }
  if (is_supported(GL_STENCIL_BACK_WRITEMASK)) {
    StateKey k("Stencil/Back", "GL_STENCIL_BACK_WRITEMASK");
    getState(k, &data);
    std::string value;
    hexString(data[0], &value);
    callback->onState(selId, experimentCount, renderId,
                      k, {value});
  }
  if (is_supported(GL_STENCIL_FAIL)) {
    StateKey k("Stencil/Front", "GL_STENCIL_FAIL");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_STENCIL_PASS_DEPTH_FAIL)) {
    StateKey k("Stencil/Front", "GL_STENCIL_PASS_DEPTH_FAIL");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_STENCIL_PASS_DEPTH_PASS)) {
    StateKey k("Stencil/Front", "GL_STENCIL_PASS_DEPTH_PASS");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_STENCIL_TEST)) {
    StateKey k("Stencil", "GL_STENCIL_TEST");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {data[0] ? "true" : "false"});
  }
  if (is_supported(GL_STENCIL_FUNC)) {
    StateKey k("Stencil/Front", "GL_STENCIL_FUNC");
    getState(k, &data);
    callback->onState(selId, experimentCount, renderId,
                      k, {state_enum_to_name(data[0])});
  }
  if (is_supported(GL_STENCIL_REF)) {
    StateKey k("Stencil/Front", "GL_STENCIL_REF");
    getState(k, &data);
    std::string value;
    hexString(data[0], &value);
    callback->onState(selId, experimentCount, renderId,
                      k, {value});
  }
  if (is_supported(GL_STENCIL_VALUE_MASK)) {
    StateKey k("Stencil/Front", "GL_STENCIL_VALUE_MASK");
    getState(k, &data);
    std::string value;
    hexString(data[0], &value);
    callback->onState(selId, experimentCount, renderId,
                      k, {value});
  }
  if (is_supported(GL_STENCIL_WRITEMASK)) {
    StateKey k("Stencil/Front", "GL_STENCIL_WRITEMASK");
    getState(k, &data);
    std::string value;
    hexString(data[0], &value);
    callback->onState(selId, experimentCount, renderId,
                      k, {value});
  }
  if (is_supported(GL_STENCIL_CLEAR_VALUE)) {
    StateKey k("Stencil", "GL_STENCIL_CLEAR_VALUE");
    getState(k, &data);
    std::string value;
    hexString(data[0], &value);
    callback->onState(selId, experimentCount, renderId,
                      k, {value});
  }
}

void
StateOverride::revertExperiments() {
  m_overrides.clear();
}
