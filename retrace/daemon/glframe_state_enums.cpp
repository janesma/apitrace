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
#include <map>
#include <string>
#include <vector>

uint32_t
glretrace::state_name_to_enum(const std::string &value) {
  static const std::map<std::string, uint32_t> names {
    {"GL_CULL_FACE", GL_CULL_FACE},
    {"GL_CULL_FACE_MODE", GL_CULL_FACE_MODE},
    {"GL_FRONT", GL_FRONT},
    {"GL_BACK", GL_BACK},
    {"GL_FRONT_AND_BACK", GL_FRONT_AND_BACK},
    {"GL_BLEND", GL_BLEND},
    {"GL_BLEND_SRC", GL_BLEND_SRC},
    {"GL_BLEND_DST", GL_BLEND_DST},
    {"GL_ZERO", GL_ZERO},
    {"GL_ONE", GL_ONE},
    {"GL_SRC_COLOR", GL_SRC_COLOR},
    {"GL_ONE_MINUS_SRC_COLOR", GL_ONE_MINUS_SRC_COLOR},
    {"GL_DST_COLOR", GL_DST_COLOR},
    {"GL_ONE_MINUS_DST_COLOR", GL_ONE_MINUS_DST_COLOR},
    {"GL_SRC_ALPHA", GL_SRC_ALPHA},
    {"GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA},
    {"GL_DST_ALPHA", GL_DST_ALPHA},
    {"GL_ONE_MINUS_DST_ALPHA", GL_ONE_MINUS_DST_ALPHA},
    {"GL_CONSTANT_COLOR", GL_CONSTANT_COLOR},
    {"GL_ONE_MINUS_CONSTANT_COLOR", GL_ONE_MINUS_CONSTANT_COLOR},
    {"GL_CONSTANT_ALPHA", GL_CONSTANT_ALPHA},
    {"GL_ONE_MINUS_CONSTANT_ALPHA", GL_ONE_MINUS_CONSTANT_ALPHA},
    {"GL_BLEND_COLOR", GL_BLEND_COLOR},
    {"GL_LINE_WIDTH", GL_LINE_WIDTH},
    {"GL_LINE_SMOOTH", GL_LINE_SMOOTH},
    {"true", 1},
    {"false", 0}
  };
  const auto i = names.find(value);
  if (i == names.end())
    return GL_INVALID_ENUM;
  return i->second;
}


std::string
glretrace::state_enum_to_name(GLint value) {
  switch (value) {
    case GL_CULL_FACE:
      return std::string("CULL_FACE");
    case GL_CULL_FACE_MODE:
      return std::string("CULL_FACE_MODE");
    case GL_FRONT:
      return std::string("GL_FRONT");
    case GL_BACK:
      return std::string("GL_BACK");
    case GL_FRONT_AND_BACK:
      return std::string("GL_FRONT_AND_BACK");
    case GL_BLEND:
      return std::string("GL_BLEND");
    case GL_BLEND_SRC:
      return std::string("GL_BLEND_SRC");
    case GL_BLEND_DST:
      return std::string("GL_BLEND_DST");
    case GL_ZERO:
      return std::string("GL_ZERO");
    case GL_ONE:
      return std::string("GL_ONE");
    case GL_SRC_COLOR:
      return std::string("GL_SRC_COLOR");
    case GL_ONE_MINUS_SRC_COLOR:
      return std::string("GL_ONE_MINUS_SRC_COLOR");
    case GL_DST_COLOR:
      return std::string("GL_DST_COLOR");
    case GL_ONE_MINUS_DST_COLOR:
      return std::string("GL_ONE_MINUS_DST_COLOR");
    case GL_SRC_ALPHA:
      return std::string("GL_SRC_ALPHA");
    case GL_ONE_MINUS_SRC_ALPHA:
      return std::string("GL_ONE_MINUS_SRC_ALPHA");
    case GL_DST_ALPHA:
      return std::string("GL_DST_ALPHA");
    case GL_ONE_MINUS_DST_ALPHA:
      return std::string("GL_ONE_MINUS_DST_ALPHA");
    case GL_CONSTANT_COLOR:
      return std::string("GL_CONSTANT_COLOR");
    case GL_ONE_MINUS_CONSTANT_COLOR:
      return std::string("GL_ONE_MINUS_CONSTANT_COLOR");
    case GL_CONSTANT_ALPHA:
      return std::string("GL_CONSTANT_ALPHA");
    case GL_ONE_MINUS_CONSTANT_ALPHA:
      return std::string("GL_ONE_MINUS_CONSTANT_ALPHA");
    default:
      assert(false);
  }
}

std::vector<std::string>
glretrace::state_name_to_choices(const std::string &n) {
  switch (state_name_to_enum(n)) {
    case GL_CULL_FACE:
    case GL_BLEND:
    case GL_LINE_SMOOTH:
      return {"true", "false"};
    case GL_CULL_FACE_MODE:
      return {"GL_FRONT", "GL_BACK", "GL_FRONT_AND_BACK"};
    case GL_BLEND_SRC:
    case GL_BLEND_DST:
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
            "GL_ONE_MINUS_CONSTANT_ALPHA"};
    case GL_INVALID_ENUM:
      assert(false);
    default:
      return {};
  }
}

