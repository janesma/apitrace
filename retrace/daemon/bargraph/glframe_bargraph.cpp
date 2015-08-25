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

#include "glframe_bargraph.hpp"

#include <string.h>

#include <GL/gl.h>

#include <vector>

#include "glframe_glhelper.hpp"

using glretrace::BarGraphRenderer;

const char *
BarGraphRenderer::vshader =
    "attribute vec2 coord; \n"
    "uniform float max_x; \n"
    "uniform float max_y; \n"
    "void main(void) { \n"
    "  /* normalize y */ \n"
    "  /* normalize x */ \n"
    "  mat2 normalize = mat2(2.0 / max_x , 0.0, 0.0, 2.0 / max_y); \n"
    "  vec2 translate = vec2(-1.0, -1.0); \n"
    "  vec2 pos = translate * normalize * coord; \n"
    "  gl_Position = vec4(pos.x, pos.y, 0.0, 1.0); \n"
    "}";

const char *
BarGraphRenderer::fshader =
    "uniform vec4 bar_color;"
    "void main(void) {"
    "   gl_FragColor = bar_color;"
    "}";

BarGraphRenderer::BarGraphRenderer() {
  // generate vbo
  GL::GenBuffers(1, &vbo);
  GL_CHECK();
  GL::BindBuffer(GL_ARRAY_BUFFER, vbo);
  GL_CHECK();
  GL::BindBuffer(GL_ARRAY_BUFFER, 0);
  GL_CHECK();

  const int vs = GL::CreateShader(GL_VERTEX_SHADER);
  GL_CHECK();
  int len = strlen(vshader);
  GL::ShaderSource(vs, 1, &vshader, &len);
  GL_CHECK();
  GL::CompileShader(vs);
  PrintCompileError(vs);

  const int fs = GL::CreateShader(GL_FRAGMENT_SHADER);
  GL_CHECK();
  len = strlen(fshader);
  GL::ShaderSource(fs, 1, &fshader, &len);
  GL_CHECK();
  GL::CompileShader(fs);
  PrintCompileError(fs);

  prog = GL::CreateProgram();
  GL::AttachShader(prog, vs);
  GL_CHECK();
  GL::AttachShader(prog, fs);
  GL_CHECK();
  GL::LinkProgram(prog);
  GL_CHECK();

  // get attribute locations
  att_coord = GL::GetAttribLocation(prog,  "coord");
  GL_CHECK();
  uni_max_x = GL::GetUniformLocation(prog,  "max_x");
  GL_CHECK();
  uni_max_y = GL::GetUniformLocation(prog,  "max_y");
  GL_CHECK();
  uni_bar_color = GL::GetUniformLocation(prog, "bar_color");
  GL_CHECK();
}

void
BarGraphRenderer::setBars(const std::vector<BarCoordinates> &bars) {
}

void
BarGraphRenderer::setMouseArea(float x1, float y1, float x2, float y2) {
}

void
BarGraphRenderer::render() {
  GL::ClearColor(1.0, 1.0, 1.0, 1.0);
  GL_CHECK();
  GL::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  GL_CHECK();

  // coordinate space is 0.0-100.0 on both axis
  std::vector<float> verts = {25, 0,
                              25, 50,
                              75, 0,
                              75, 50};

  GL::UseProgram(prog);
  GL_CHECK();

  // set uniforms
  GL::Uniform1f(uni_max_y, 100.0);
  GL_CHECK();
  GL::Uniform1f(uni_max_x, 100.0);
  GL_CHECK();
  const float color[4] = { .5, .5, .5, 1.0 };
  GL::Uniform4f(uni_bar_color, color[0], color[1], color[2], color[3]);
  GL_CHECK();

  // bind vbo
  GL::BindBuffer(GL_ARRAY_BUFFER, vbo);
  GL_CHECK();

  // buffer data to vbo
  GL::BufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float),
               verts.data(), GL_STATIC_DRAW);
  GL_CHECK();

  // enable vbo
  GL::EnableVertexAttribArray(att_coord);
  GL_CHECK();

  // describe vbo
  GL::VertexAttribPointer(att_coord,           // attribute
                          2,                   // number of elements
                                               // per vertex, here
                                               // (x,y)
                          GL_FLOAT,            // the type of each element
                          GL_FALSE,            // take our values as-is
                          0,                   // no space between values
                          0);                  // use the vbo
  GL_CHECK();

  // draw
  GL::DrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  GL_CHECK();

  // disable vbo
  GL::DisableVertexAttribArray(att_coord);
  GL_CHECK();

  // unbind vbo
  GL::BindBuffer(GL_ARRAY_BUFFER, 0);
  GL_CHECK();
}

