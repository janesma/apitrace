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

#include "glframe_state_enums.hpp"

#include <assert.h>
#include <GL/glext.h>

#include <map>
#include <string>
#include <vector>

uint32_t
glretrace::state_name_to_enum(const std::string &value) {
  static const std::map<std::string, uint32_t> names {
    {"GL_ALWAYS", GL_ALWAYS},
    {"GL_BACK", GL_BACK},
    {"GL_BLEND", GL_BLEND},
    {"GL_BLEND_COLOR", GL_BLEND_COLOR},
    {"GL_BLEND_DST", GL_BLEND_DST},
    {"GL_BLEND_DST_ALPHA", GL_BLEND_DST_ALPHA},
    {"GL_BLEND_DST_RGB", GL_BLEND_DST_RGB},
    {"GL_BLEND_EQUATION_ALPHA", GL_BLEND_EQUATION_ALPHA},
    {"GL_BLEND_EQUATION_RGB", GL_BLEND_EQUATION_RGB},
    {"GL_BLEND_SRC", GL_BLEND_SRC},
    {"GL_BLEND_SRC_ALPHA", GL_BLEND_SRC_ALPHA},
    {"GL_BLEND_SRC_RGB", GL_BLEND_SRC_RGB},
    {"GL_COLOR_CLEAR_VALUE", GL_COLOR_CLEAR_VALUE},
    {"GL_COLOR_WRITEMASK", GL_COLOR_WRITEMASK},
    {"GL_CONSTANT_ALPHA", GL_CONSTANT_ALPHA},
    {"GL_CONSTANT_COLOR", GL_CONSTANT_COLOR},
    {"GL_CULL_FACE", GL_CULL_FACE},
    {"GL_CULL_FACE_MODE", GL_CULL_FACE_MODE},
    {"GL_DEPTH_CLEAR_VALUE", GL_DEPTH_CLEAR_VALUE},
    {"GL_DEPTH_FUNC", GL_DEPTH_FUNC},
    {"GL_DEPTH_RANGE", GL_DEPTH_RANGE},
    {"GL_DEPTH_TEST", GL_DEPTH_TEST},
    {"GL_DEPTH_WRITEMASK", GL_DEPTH_WRITEMASK},
    {"GL_DST_ALPHA", GL_DST_ALPHA},
    {"GL_DST_COLOR", GL_DST_COLOR},
    {"GL_EQUAL", GL_EQUAL},
    {"GL_FRONT", GL_FRONT},
    {"GL_FRONT_AND_BACK", GL_FRONT_AND_BACK},
    {"GL_FUNC_ADD", GL_FUNC_ADD},
    {"GL_FUNC_REVERSE_SUBTRACT", GL_FUNC_REVERSE_SUBTRACT},
    {"GL_FUNC_SUBTRACT", GL_FUNC_SUBTRACT},
    {"GL_GEQUAL", GL_GEQUAL},
    {"GL_GREATER", GL_GREATER},
    {"GL_LEQUAL", GL_LEQUAL},
    {"GL_LESS", GL_LESS},
    {"GL_LINE_SMOOTH", GL_LINE_SMOOTH},
    {"GL_LINE_WIDTH", GL_LINE_WIDTH},
    {"GL_MAX", GL_MAX},
    {"GL_MIN", GL_MIN},
    {"GL_NEVER", GL_NEVER},
    {"GL_NOTEQUAL", GL_NOTEQUAL},
    {"GL_ONE", GL_ONE},
    {"GL_ONE_MINUS_CONSTANT_ALPHA", GL_ONE_MINUS_CONSTANT_ALPHA},
    {"GL_ONE_MINUS_CONSTANT_COLOR", GL_ONE_MINUS_CONSTANT_COLOR},
    {"GL_ONE_MINUS_DST_ALPHA", GL_ONE_MINUS_DST_ALPHA},
    {"GL_ONE_MINUS_DST_COLOR", GL_ONE_MINUS_DST_COLOR},
    {"GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA},
    {"GL_ONE_MINUS_SRC_COLOR", GL_ONE_MINUS_SRC_COLOR},
    {"GL_SRC_ALPHA", GL_SRC_ALPHA},
    {"GL_SRC_ALPHA_SATURATE", GL_SRC_ALPHA_SATURATE},
    {"GL_SRC_COLOR", GL_SRC_COLOR},
    {"GL_ZERO", GL_ZERO},
    {"false", 0},
    {"true", 1},
  };
  const auto i = names.find(value);
  if (i == names.end())
    return GL_INVALID_ENUM;
  return i->second;
}


std::string
glretrace::state_enum_to_name(GLint value) {
  static const std::map<uint32_t, std::string> names {
    {GL_ALWAYS, "GL_ALWAYS"},
    {GL_BACK, "GL_BACK"},
    {GL_BLEND, "GL_BLEND"},
    {GL_BLEND_COLOR, "GL_BLEND_COLOR"},
    {GL_BLEND_DST, "GL_BLEND_DST"},
    {GL_BLEND_DST_ALPHA, "GL_BLEND_DST_ALPHA"},
    {GL_BLEND_DST_RGB, "GL_BLEND_DST_RGB"},
    {GL_BLEND_EQUATION_ALPHA, "GL_BLEND_EQUATION_ALPHA"},
    {GL_BLEND_EQUATION_RGB, "GL_BLEND_EQUATION_RGB"},
    {GL_BLEND_SRC, "GL_BLEND_SRC"},
    {GL_BLEND_SRC_ALPHA, "GL_BLEND_SRC_ALPHA"},
    {GL_BLEND_SRC_RGB, "GL_BLEND_SRC_RGB"},
    {GL_COLOR_CLEAR_VALUE, "GL_COLOR_CLEAR_VALUE"},
    {GL_COLOR_WRITEMASK, "GL_COLOR_WRITEMASK"},
    {GL_CONSTANT_ALPHA, "GL_CONSTANT_ALPHA"},
    {GL_CONSTANT_COLOR, "GL_CONSTANT_COLOR"},
    {GL_CULL_FACE, "GL_CULL_FACE"},
    {GL_CULL_FACE_MODE, "GL_CULL_FACE_MODE"},
    {GL_DEPTH_CLEAR_VALUE, "GL_DEPTH_CLEAR_VALUE"},
    {GL_DEPTH_FUNC, "GL_DEPTH_FUNC"},
    {GL_DEPTH_RANGE, "GL_DEPTH_RANGE"},
    {GL_DEPTH_TEST, "GL_DEPTH_TEST"},
    {GL_DEPTH_WRITEMASK, "GL_DEPTH_WRITEMASK"},
    {GL_DST_ALPHA, "GL_DST_ALPHA"},
    {GL_DST_COLOR, "GL_DST_COLOR"},
    {GL_EQUAL, "GL_EQUAL"},
    {GL_FRONT, "GL_FRONT"},
    {GL_FRONT_AND_BACK, "GL_FRONT_AND_BACK"},
    {GL_FUNC_ADD, "GL_FUNC_ADD"},
    {GL_FUNC_REVERSE_SUBTRACT, "GL_FUNC_REVERSE_SUBTRACT"},
    {GL_FUNC_SUBTRACT, "GL_FUNC_SUBTRACT"},
    {GL_GEQUAL, "GL_GEQUAL"},
    {GL_GREATER, "GL_GREATER"},
    {GL_LEQUAL, "GL_LEQUAL"},
    {GL_LESS, "GL_LESS"},
    {GL_LINE_SMOOTH, "GL_LINE_SMOOTH"},
    {GL_LINE_WIDTH, "GL_LINE_WIDTH"},
    {GL_MAX, "GL_MAX"},
    {GL_MIN, "GL_MIN"},
    {GL_NEVER, "GL_NEVER"},
    {GL_NOTEQUAL, "GL_NOTEQUAL"},
    {GL_ONE, "GL_ONE"},
    {GL_ONE_MINUS_CONSTANT_ALPHA, "GL_ONE_MINUS_CONSTANT_ALPHA"},
    {GL_ONE_MINUS_CONSTANT_COLOR, "GL_ONE_MINUS_CONSTANT_COLOR"},
    {GL_ONE_MINUS_DST_ALPHA, "GL_ONE_MINUS_DST_ALPHA"},
    {GL_ONE_MINUS_DST_COLOR, "GL_ONE_MINUS_DST_COLOR"},
    {GL_ONE_MINUS_SRC_ALPHA, "GL_ONE_MINUS_SRC_ALPHA"},
    {GL_ONE_MINUS_SRC_COLOR, "GL_ONE_MINUS_SRC_COLOR"},
    {GL_SRC_ALPHA, "GL_SRC_ALPHA"},
    {GL_SRC_ALPHA_SATURATE, "GL_SRC_ALPHA_SATURATE"},
    {GL_SRC_COLOR, "GL_SRC_COLOR"},
    {GL_ZERO, "GL_ZERO"},
        };
  const auto i = names.find(value);
  if (i == names.end()) {
    assert(false);
    return "GL_INVALID_ENUM";
  }
  return i->second;
}

std::vector<std::string>
glretrace::state_name_to_choices(const std::string &n) {
  switch (state_name_to_enum(n)) {
    case GL_BLEND:
    case GL_COLOR_WRITEMASK:
    case GL_CULL_FACE:
    case GL_DEPTH_TEST:
    case GL_DEPTH_WRITEMASK:
    case GL_LINE_SMOOTH:
      return {"true", "false"};
    case GL_BLEND_DST:
    case GL_BLEND_DST_ALPHA:
    case GL_BLEND_DST_RGB:
    case GL_BLEND_SRC:
    case GL_BLEND_SRC_ALPHA:
    case GL_BLEND_SRC_RGB:
      return {"GL_ZERO",
            "GL_ONE",
            "GL_SRC_COLOR",
            "GL_ONE_MINUS_SRC_COLOR",
            "GL_DST_COLOR",
            "GL_ONE_MINUS_DST_COLOR",
            "GL_SRC_ALPHA",
            "GL_ONE_MINUS_SRC_ALPHA",
            "GL_DST_ALPHA",
            "GL_ONE_MINUS_DST_ALPHA",
            "GL_CONSTANT_COLOR",
            "GL_ONE_MINUS_CONSTANT_COLOR",
            "GL_CONSTANT_ALPHA",
            "GL_ONE_MINUS_CONSTANT_ALPHA",
            "GL_SRC_ALPHA_SATURATE"
            };
    case GL_BLEND_EQUATION_ALPHA:
    case GL_BLEND_EQUATION_RGB:
      return {"GL_FUNC_ADD",
            "GL_FUNC_SUBTRACT",
            "GL_FUNC_REVERSE_SUBTRACT",
            "GL_MIN",
            "GL_MAX"
            };
    case GL_CULL_FACE_MODE:
      return {"GL_FRONT", "GL_BACK", "GL_FRONT_AND_BACK"};
    case GL_DEPTH_FUNC:
      return {"GL_NEVER", "GL_LESS", "GL_EQUAL", "GL_LEQUAL",
            "GL_GREATER", "GL_NOTEQUAL", "GL_GEQUAL", "GL_ALWAYS"};
    case GL_INVALID_ENUM:
      assert(false);
    default:
      return {};
  }
}

std::vector<std::string>
glretrace::state_name_to_indices(const std::string &n) {
  switch (state_name_to_enum(n)) {
    case GL_COLOR_WRITEMASK:
      return {"Red Enabled", "Green Enabled", "Blue Enabled", "Alpha Enabled"};
    case GL_BLEND_COLOR:
    case GL_COLOR_CLEAR_VALUE:
      return {"Red", "Green", "Blue", "Alpha"};
    case GL_DEPTH_RANGE:
      return {"Near", "Far"};
    case GL_DEPTH_WRITEMASK:
      return {"Enabled"};
    default:
      return {};
  }
}
