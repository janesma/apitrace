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

#include "glframe_uniforms.hpp"

#include <assert.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <sstream>
#include <string>
#include <vector>

#include "glframe_glhelper.hpp"

using glretrace::ExperimentId;
using glretrace::OnFrameRetrace;
using glretrace::RenderId;
using glretrace::SelectionId;
using glretrace::Uniforms;
using glretrace::Uniform;

namespace glretrace {
class Uniform {
  // Holds a single uniform.
 public:
  explicit Uniform(int i);
  void set() const;
  void onUniform(SelectionId selectionCount,
                 ExperimentId experimentCount,
                 RenderId renderId,
                 OnFrameRetrace *callback) const;

 private:
  enum UniformType {
    k_float,
    k_vec2,
    k_vec3,
    k_vec4,
    k_int,
    k_ivec2,
    k_ivec3,
    k_ivec4,
    k_uint,
    k_uivec2,
    k_uivec3,
    k_uivec4,
    k_bool,
    k_bvec2,
    k_bvec3,
    k_bvec4,
    k_mat2,
    k_mat3,
    k_mat4,
    k_mat2x3,
    k_mat2x4,
    k_mat3x2,
    k_mat3x4,
    k_mat4x2,
    k_mat4x3,
    k_sampler
  };

  UniformType m_dataType;
  int m_location, m_data_size, m_array_size;
  std::string m_name;
  std::vector<unsigned char> m_data;
};
}  // namespace glretrace

Uniform::Uniform(int i) {
  GLint prog, name_len, name_buf_len;
  GLenum data_type;
  GL_CHECK();
  GlFunctions::GetIntegerv(GL_CURRENT_PROGRAM, &prog);
  GL_CHECK();
  GlFunctions::GetProgramiv(prog, GL_ACTIVE_UNIFORM_MAX_LENGTH, &name_buf_len);
  {
    std::vector<char> name_buf(name_buf_len * 2);
    GlFunctions::GetActiveUniform(prog,
                                  i,
                                  name_buf_len, &name_len,
                                  &m_array_size, &data_type,
                                  reinterpret_cast<GLchar*>(name_buf.data()));
    GL_CHECK();
    name_buf[name_len] = '\0';
    m_location = GlFunctions::GetUniformLocation(prog,
                                                 name_buf.data());
    if (m_array_size > 1)
      // strip the [0] off of the name, we will iterate on it later
      name_buf[name_len - 3] = '\0';
    m_name = std::string(name_buf.data());
  }

  if (m_location == -1)
    // uniform was eliminated during linking
    return;

  for (int i = 0; i < m_array_size; ++i) {
    bool is_float = false;
    switch (data_type) {
      case GL_FLOAT:
        m_dataType = k_float;
        is_float = true;
        m_data_size = 1 * sizeof(GLfloat);
        break;
      case GL_FLOAT_VEC2:
        m_dataType = k_vec2;
        is_float = true;
        m_data_size = 2 * sizeof(GLfloat);
        break;
      case GL_FLOAT_VEC3:
        m_dataType = k_vec3;
        is_float = true;
        m_data_size = 3 * sizeof(GLfloat);
        break;
      case GL_FLOAT_VEC4:
        m_dataType = k_vec4;
        is_float = true;
        m_data_size = 4 * sizeof(GLfloat);
        break;
      case GL_INT:
        m_dataType = k_int;
        is_float = false;
        m_data_size = 1 * sizeof(GLint);
        break;
      case GL_INT_VEC2:
        m_dataType = k_ivec2;
        is_float = false;
        m_data_size = 2 * sizeof(GLint);
        break;
      case GL_INT_VEC3:
        m_dataType = k_ivec3;
        is_float = false;
        m_data_size = 3 * sizeof(GLint);
        break;
      case GL_INT_VEC4:
        m_dataType = k_ivec4;
        is_float = false;
        m_data_size = 4 * sizeof(GLint);
        break;
      case GL_UNSIGNED_INT:
        m_dataType = k_uint;
        is_float = false;
        m_data_size = 1 * sizeof(GLint);
        break;
      case GL_UNSIGNED_INT_VEC2:
        m_dataType = k_uivec2;
        is_float = false;
        m_data_size = 2 * sizeof(GLint);
        break;
      case GL_UNSIGNED_INT_VEC3:
        m_dataType = k_uivec3;
        is_float = false;
        m_data_size = 3 * sizeof(GLint);
        break;
      case GL_UNSIGNED_INT_VEC4:
        m_dataType = k_uivec4;
        is_float = false;
        m_data_size = 4 * sizeof(GLint);
        break;
      case GL_BOOL:
        m_dataType = k_bool;
        is_float = false;
        m_data_size = 1 * sizeof(GLint);
        break;
      case GL_BOOL_VEC2:
        m_dataType = k_bvec2;
        is_float = false;
        m_data_size = 2 * sizeof(GLint);
        break;
      case GL_BOOL_VEC3:
        m_dataType = k_bvec3;
        is_float = false;
        m_data_size = 3 * sizeof(GLint);
        break;
      case GL_BOOL_VEC4:
        m_dataType = k_bvec4;
        is_float = false;
        m_data_size = 4 * sizeof(GLint);
        break;
      case GL_FLOAT_MAT2:
        m_dataType = k_mat2;
        is_float = true;
        m_data_size = 4 * sizeof(GLfloat);
        break;
      case GL_FLOAT_MAT3:
        m_dataType = k_mat3;
        is_float = true;
        m_data_size = 9 * sizeof(GLfloat);
        break;
      case GL_FLOAT_MAT4:
        m_dataType = k_mat4;
        is_float = true;
        m_data_size = 16 * sizeof(GLfloat);
        break;
      case GL_FLOAT_MAT2x3:
        m_dataType = k_mat2x3;
        is_float = true;
        m_data_size = 6 * sizeof(GLfloat);
        break;
      case GL_FLOAT_MAT2x4:
        m_dataType = k_mat2x4;
        is_float = true;
        m_data_size = 8 * sizeof(GLfloat);
        break;
      case GL_FLOAT_MAT3x2:
        m_dataType = k_mat3x2;
        is_float = true;
        m_data_size = 6 * sizeof(GLfloat);
        break;
      case GL_FLOAT_MAT3x4:
        m_dataType = k_mat3x4;
        is_float = true;
        m_data_size = 12 * sizeof(GLfloat);
        break;
      case GL_FLOAT_MAT4x2:
        m_dataType = k_mat4x2;
        is_float = true;
        m_data_size = 8 * sizeof(GLfloat);
        break;
      case GL_FLOAT_MAT4x3:
        m_dataType = k_mat4x3;
        is_float = true;
        m_data_size = 12 * sizeof(GLfloat);
        break;
      case GL_INT_SAMPLER_2D:
      case GL_INT_SAMPLER_2D_ARRAY:
      case GL_INT_SAMPLER_3D:
      case GL_INT_SAMPLER_CUBE:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_2D_ARRAY_SHADOW:
      case GL_SAMPLER_2D_SHADOW:
      case GL_SAMPLER_3D:
      case GL_SAMPLER_BUFFER:
      case GL_SAMPLER_CUBE:
      case GL_SAMPLER_CUBE_SHADOW:
      case GL_UNSIGNED_INT_SAMPLER_2D:
      case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
      case GL_UNSIGNED_INT_SAMPLER_3D:
      case GL_UNSIGNED_INT_SAMPLER_CUBE:
      case GL_SAMPLER_CUBE_MAP_ARRAY:
      case GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW:
      case GL_INT_SAMPLER_CUBE_MAP_ARRAY:
      case GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY:
      case GL_IMAGE_2D:
      case GL_IMAGE_CUBE_MAP_ARRAY:
      case GL_INT_IMAGE_CUBE_MAP_ARRAY:
      case GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY:
      case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      case GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY:
      case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:
      case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
      case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
      case GL_SAMPLER_2D_RECT_ARB:
        m_dataType = k_sampler;
        is_float = false;
        m_data_size = 1 * sizeof(GLint);
        break;
      default:
        assert(false);
    }
    m_data.resize(m_data_size * m_array_size);
    for (int i = 0; i < m_array_size; ++i) {
      GLint location = m_location;
      int offset = 0;
      if (m_array_size > 1) {
        // get the location of the array element
        std::stringstream ss;
        ss << m_name << "[" << i << "]";
        location = GlFunctions::GetUniformLocation(prog, ss.str().c_str());
        assert(location != -1);
        // calculate offset
        offset = i * m_data_size;
      }
      if (is_float) {
        GlFunctions::GetUniformfv(prog, location,
                                  reinterpret_cast<GLfloat*>(&m_data[offset]));
        GL_CHECK();
      } else {
        GlFunctions::GetUniformiv(prog, location,
                                  reinterpret_cast<GLint *>(&m_data[offset]));
        GL_CHECK();
      }
    }
  }
}

void
Uniform::set() const {
  int prog;
  GL::GetError();
  GlFunctions::GetIntegerv(GL_CURRENT_PROGRAM, &prog);
  GL_CHECK();
  int location = GlFunctions::GetUniformLocation(prog, m_name.c_str());
  GL_CHECK();
  if (location == -1)
    return;

  switch (m_dataType) {
    case k_float:
      GlFunctions::Uniform1fv(location, m_array_size,
                              reinterpret_cast<const GLfloat*>(m_data.data()));
      break;
    case k_vec2:
      GlFunctions::Uniform2fv(location, m_array_size,
                              reinterpret_cast<const GLfloat*>(m_data.data()));
      break;
    case k_vec3:
      GlFunctions::Uniform3fv(location, m_array_size,
                              reinterpret_cast<const GLfloat*>(m_data.data()));
      break;
    case k_vec4:
      GlFunctions::Uniform4fv(location, m_array_size,
                              reinterpret_cast<const GLfloat*>(m_data.data()));
      break;
    case k_bool:
    case k_sampler:
    case k_int:
      GlFunctions::Uniform1iv(location, m_array_size,
                              reinterpret_cast<const GLint*>(m_data.data()));
      break;
    case k_bvec2:
    case k_ivec2:
      GlFunctions::Uniform2iv(location, m_array_size,
                              reinterpret_cast<const GLint*>(m_data.data()));
      break;
    case k_bvec3:
    case k_ivec3:
      GlFunctions::Uniform3iv(location, m_array_size,
                              reinterpret_cast<const GLint*>(m_data.data()));
      break;
    case k_bvec4:
    case k_ivec4:
      GlFunctions::Uniform4iv(location, m_array_size,
                              reinterpret_cast<const GLint*>(m_data.data()));
      break;
    case k_uint:
      GlFunctions::Uniform1uiv(location, m_array_size,
                               reinterpret_cast<const GLuint*>(m_data.data()));
      break;
    case k_uivec2:
      GlFunctions::Uniform2uiv(location, m_array_size,
                               reinterpret_cast<const GLuint*>(m_data.data()));
      break;
    case k_uivec3:
      GlFunctions::Uniform3uiv(location, m_array_size,
                               reinterpret_cast<const GLuint*>(m_data.data()));
      break;
    case k_uivec4:
      GlFunctions::Uniform4uiv(location, m_array_size,
                               reinterpret_cast<const GLuint*>(m_data.data()));
      break;
    case k_mat2:
      GlFunctions::UniformMatrix2fv(location, m_array_size, false,
                                    reinterpret_cast<const GLfloat*>(
                                        m_data.data()));
      break;
    case k_mat3:
      GlFunctions::UniformMatrix3fv(location, m_array_size, false,
                                    reinterpret_cast<const GLfloat*>(
                                        m_data.data()));
      break;
    case k_mat4:
      GlFunctions::UniformMatrix4fv(location, m_array_size, false,
                                    reinterpret_cast<const GLfloat*>(
                                        m_data.data()));
      break;
    case k_mat2x3:
      GlFunctions::UniformMatrix2x3fv(location, m_array_size, false,
                                      reinterpret_cast<const GLfloat*>(
                                          m_data.data()));
      break;
    case k_mat2x4:
      GlFunctions::UniformMatrix2x4fv(location, m_array_size, false,
                                      reinterpret_cast<const GLfloat*>(
                                          m_data.data()));
      break;
    case k_mat3x2:
      GlFunctions::UniformMatrix3x2fv(location, m_array_size, false,
                                      reinterpret_cast<const GLfloat*>(
                                          m_data.data()));
      break;
    case k_mat3x4:
      GlFunctions::UniformMatrix3x4fv(location, m_array_size, false,
                                      reinterpret_cast<const GLfloat*>(
                                          m_data.data()));
      break;
    case k_mat4x2:
      GlFunctions::UniformMatrix4x2fv(location, m_array_size, false,
                                      reinterpret_cast<const GLfloat*>(
                                          m_data.data()));
      break;
    case k_mat4x3:
      GlFunctions::UniformMatrix4x3fv(location, m_array_size, false,
                                      reinterpret_cast<const GLfloat*>(
                                          m_data.data()));
      break;
    default:
      assert(false);
  }
  GL_CHECK();
}

void
Uniform::onUniform(SelectionId selectionCount,
                   ExperimentId experimentCount,
                   RenderId renderId,
                   OnFrameRetrace *callback) const {
  // void onUniform(SelectionId selectionCount,
  //                        ExperimentId experimentCount,
  //                        RenderId renderId,
  //                        const std::string &name,
  //                        UniformType type,
  //                        UniformDimension dimension,
  //                        const std::vector<unsigned char> &data) = 0;
  if (m_location == -1)
    return;
  glretrace::UniformType t;
  glretrace::UniformDimension d;
  switch (m_dataType) {
    case k_float:
    case k_vec2:
    case k_vec3:
    case k_vec4:
    case k_mat2:
    case k_mat3:
    case k_mat4:
    case k_mat2x3:
    case k_mat2x4:
    case k_mat3x2:
    case k_mat3x4:
    case k_mat4x2:
    case k_mat4x3:
      t = kFloatUniform;
      break;
    case k_int:
    case k_ivec2:
    case k_ivec3:
    case k_ivec4:
    case k_sampler:
      t = kIntUniform;
      break;
    case k_uint:
    case k_uivec2:
    case k_uivec3:
    case k_uivec4:
      t = kUIntUniform;
      break;
    case k_bool:
    case k_bvec2:
    case k_bvec3:
    case k_bvec4:
      t = kBoolUniform;
      break;
  }
  switch (m_dataType) {
    case k_float:
    case k_sampler:
    case k_int:
    case k_uint:
    case k_bool:
      d = k1x1;
      break;
    case k_vec2:
    case k_ivec2:
    case k_uivec2:
    case k_bvec2:
      d = k2x1;
      break;
    case k_vec3:
    case k_ivec3:
    case k_uivec3:
    case k_bvec3:
      d = k3x1;
      break;
    case k_vec4:
    case k_ivec4:
    case k_uivec4:
    case k_bvec4:
      d = k4x1;
      break;
    case k_mat2:
      d = k2x2;
      break;
    case k_mat3:
      d = k3x3;
      break;
    case k_mat4:
      d = k4x4;
      break;
    case k_mat2x3:
      d = k2x3;
      break;
    case k_mat2x4:
      d = k2x4;
      break;
    case k_mat3x2:
      d = k3x2;
      break;
    case k_mat3x4:
      d = k3x4;
      break;
    case k_mat4x2:
      d = k4x2;
      break;
    case k_mat4x3:
      d = k4x3;
      break;
  }
  callback->onUniform(selectionCount, experimentCount, renderId,
                      m_name, t, d, m_data);
}

Uniforms::Uniforms() {
  int prog, uniform_count;
  GL::GetError();
  GlFunctions::GetIntegerv(GL_CURRENT_PROGRAM, &prog);
  if (prog == 0)
    return;
  GL_CHECK();
  GlFunctions::GetProgramiv(prog, GL_ACTIVE_UNIFORMS, &uniform_count);
  GL_CHECK();
  for (int i = 0; i < uniform_count; ++i) {
    m_uniforms.push_back(new Uniform(i));
  }
}

Uniforms::~Uniforms() {
  for (auto i : m_uniforms)
    delete i;
}

void
Uniforms::set() const {
  for (auto i : m_uniforms) {
    i->set();
  }
}

void
Uniforms::onUniform(SelectionId selectionCount,
                    ExperimentId experimentCount,
                    RenderId renderId,
                    OnFrameRetrace *callback) const {
  for (auto u : m_uniforms)
    u->onUniform(selectionCount, experimentCount,
                 renderId, callback);
}
