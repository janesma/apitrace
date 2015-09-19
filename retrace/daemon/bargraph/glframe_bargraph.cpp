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
#include <assert.h>

#include <GL/gl.h>

#include <vector>

#include "glframe_glhelper.hpp"

using glretrace::BarGraphRenderer;

const char *
BarGraphRenderer::vshader =
    "attribute vec2 coord; \n"
    "uniform float max_x; \n"
    "uniform float max_y; \n"
    "uniform float invert_y; \n"
    "void main(void) { \n"
    "  /* normalize y */ \n"
    "  /* normalize x */ \n"
    "  mat2 normalize = mat2(2.0 / max_x , 0.0, 0.0, 2.0 / max_y); \n"
    "  vec2 translate = vec2(-1.0, -1.0); \n"
    "  vec2 pos = translate + normalize * coord; \n"
    "  gl_Position = vec4(pos.x, invert_y * pos.y, 0.0, 1.0); \n"
    "}";

const char *
BarGraphRenderer::fshader =
    "uniform vec4 bar_color;"
    "void main(void) {"
    "   gl_FragColor = bar_color;"
    "}";

BarGraphRenderer::BarGraphRenderer(bool invert)
    : invert_y(invert ? -1 : 1) {
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
  uni_invert_y = GL::GetUniformLocation(prog,  "invert_y");
  GL_CHECK();
  uni_bar_color = GL::GetUniformLocation(prog, "bar_color");
  GL_CHECK();
}

void
BarGraphRenderer::setBars(const std::vector<BarMetrics> &bars) {
  vertices.resize(bars.size() * 4);
  max_y = 0;
  total_x = 0;
  for (auto bar : bars) {
    if (bar.metric1 > max_y)
      max_y = bar.metric1;
    total_x += bar.metric2;
  }

  float extra_bar_width = 0, bar_spacing = 0, start_x = 0;
  if (total_x == 0) {
    extra_bar_width = 3;
    bar_spacing = 1;
    start_x = 1;
    total_x = bars.size() * (extra_bar_width + bar_spacing) + start_x;
  }

  float current_x = start_x;
  auto current_vertex = vertices.begin();
  for (auto bar : bars) {
    current_vertex->x = current_x;
    current_vertex->y = 0;
    ++current_vertex;

    current_vertex->x = current_x;
    current_vertex->y = bar.metric1;
    ++current_vertex;

    current_x += (bar.metric2 + extra_bar_width);
    current_vertex->x = current_x;
    current_vertex->y = 0;
    ++current_vertex;

    current_vertex->x = current_x;
    current_vertex->y = bar.metric1;
    ++current_vertex;
    current_x += bar_spacing;
  }
  assert(current_vertex == vertices.end());
  total_x = current_x;
}

void
BarGraphRenderer::setMouseArea(float x1, float y1, float x2, float y2) {
}

// solely for debugging: allows gdb to print vertices[n]
template class std::vector<BarGraphRenderer::Vertex>;

void
BarGraphRenderer::render() {
  GL::ClearColor(1.0, 1.0, 1.0, 1.0);
  GL_CHECK();
  GL::Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  GL_CHECK();

  GL::UseProgram(prog);
  GL_CHECK();

  // set uniforms
  GL::Uniform1f(uni_max_y, max_y);
  GL_CHECK();
  GL::Uniform1f(uni_max_x, total_x);
  GL_CHECK();
  GL::Uniform1f(uni_invert_y, invert_y);
  GL_CHECK();
  const float color[4] = { .5, .5, .5, 1.0 };
  GL::Uniform4f(uni_bar_color, color[0], color[1], color[2], color[3]);
  GL_CHECK();

  // bind vbo
  GL::BindBuffer(GL_ARRAY_BUFFER, vbo);
  GL_CHECK();

  // buffer data to vbo
  GL::BufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);
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

  for (GLint i = 0; i < vertices.size(); i += 4) {
    // draw
    GL::DrawArrays(GL_TRIANGLE_STRIP, i, 4);
    GL_CHECK();
  }

  // disable vbo
  GL::DisableVertexAttribArray(att_coord);
  GL_CHECK();

  // unbind vbo
  GL::BindBuffer(GL_ARRAY_BUFFER, 0);
  GL_CHECK();
}
