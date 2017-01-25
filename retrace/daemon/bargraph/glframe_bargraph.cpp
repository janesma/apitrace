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
#include <stdint.h>

#include <algorithm>
#include <vector>
#include <set>
#include <string>

using glretrace::BarGraphRenderer;
using glretrace::BarGraphSubscriber;
using glretrace::BarMetrics;

#define GL_CHECK() CheckError(__FILE__, __LINE__)

void
BarGraphRenderer::GetCompileError(GLint shader, std::string *message) {
  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if (status == GL_TRUE)
    return;
  static const int MAXLEN = 1024;
  std::vector<char> log(MAXLEN);
  GLsizei len;
  glGetShaderInfoLog(shader,  MAXLEN,  &len, log.data());
  *message = log.data();
}

void
BarGraphRenderer::CheckError(const char * file, int line) {
  const int error = glGetError();
  if ( error == GL_NO_ERROR)
    return;
  printf("ERROR: %x %s:%i\n", error, file, line);
}

void
BarGraphRenderer::PrintCompileError(GLint shader) {
  std::string message;
  GetCompileError(shader, &message);
  if (message.size())
    printf("ERROR -- compile failed: %s\n", message.c_str());
}

const char *
BarGraphRenderer::vshader =
    "#version 100\n"
    "attribute mediump vec2 coord; \n"
    "uniform mediump float max_x; \n"
    "uniform mediump float max_y; \n"
    "uniform mediump float invert_y; \n"
    "uniform mediump float zoom_translate_x; \n"
    "uniform mediump float zoom_x; \n"
    "void main(void) { \n"
    "  /* translate zoom_center to 0,0, then zoom */ \n"
    "  vec2 zoom_coord = vec2(zoom_x * coord.x, coord.y); \n"
    "  /* normalize y */ \n"
    "  /* normalize x */ \n"
    "  mat2 normalize = mat2(2.0 / max_x , 0.0, 0.0, 2.0 / max_y); \n"
    "  vec2 translate = vec2(-1.0 + 2.0 * zoom_translate_x, -1.0); \n"
    "  vec2 pos = translate + normalize * zoom_coord; \n"
    "  gl_Position = vec4(pos.x, invert_y * pos.y, 0.0, 1.0); \n"
    "}";

const char *
BarGraphRenderer::fshader =
    "#version 100\n"
    "uniform mediump vec4 bar_color;"
    "void main(void) {"
    "   gl_FragColor = bar_color;"
    "}";

BarGraphRenderer::BarGraphRenderer(bool invert)
    : mouse_vertices(4),
      mouse_area(2),
      invert_y(invert ? -1 : 1),
      zoom(1.0),
      zoom_translate(0.0),
      subscriber(NULL) {
  initializeOpenGLFunctions();
  // generate vbo
  glGenBuffers(1, &vbo);
  GL_CHECK();
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  GL_CHECK();
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  GL_CHECK();

  const int vs = glCreateShader(GL_VERTEX_SHADER);
  GL_CHECK();
  int len = strlen(vshader);
  glShaderSource(vs, 1, &vshader, &len);
  GL_CHECK();
  glCompileShader(vs);
  PrintCompileError(vs);

  const int fs = glCreateShader(GL_FRAGMENT_SHADER);
  GL_CHECK();
  len = strlen(fshader);
  glShaderSource(fs, 1, &fshader, &len);
  GL_CHECK();
  glCompileShader(fs);
  PrintCompileError(fs);

  prog = glCreateProgram();
  glAttachShader(prog, vs);
  GL_CHECK();
  glAttachShader(prog, fs);
  GL_CHECK();
  glLinkProgram(prog);
  GL_CHECK();

  // get attribute locations
  att_coord = glGetAttribLocation(prog,  "coord");
  GL_CHECK();
  uni_max_x = glGetUniformLocation(prog,  "max_x");
  GL_CHECK();
  uni_max_y = glGetUniformLocation(prog,  "max_y");
  GL_CHECK();
  uni_invert_y = glGetUniformLocation(prog,  "invert_y");
  GL_CHECK();
  uni_bar_color = glGetUniformLocation(prog, "bar_color");
  GL_CHECK();
  uni_zoom_translate_x = glGetUniformLocation(prog,  "zoom_translate_x");
  GL_CHECK();
  uni_zoom_x = glGetUniformLocation(prog,  "zoom_x");
  GL_CHECK();
}

void
BarGraphRenderer::setBars(const std::vector<BarMetrics> &bars) {
  vertices.resize(bars.size() * 4);
  selected.resize(bars.size());

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

float
BarGraphRenderer::unzoomX(float x) {
  return (x - zoom_translate) / zoom * total_x;
}

void
BarGraphRenderer::setMouseArea(float x1, float y1, float x2, float y2) {
  mouse_area[0].x = x1;
  mouse_area[0].y = y1;
  mouse_area[1].x = x2;
  mouse_area[1].y = y2;

  // calculate mouse_vertices based on b
  mouse_vertices[0].x = x1;
  mouse_vertices[0].y = y1;
  mouse_vertices[1].x = x1;
  mouse_vertices[1].y = y2;
  mouse_vertices[2].x = x2;
  mouse_vertices[2].y = y1;
  mouse_vertices[3].x = x2;
  mouse_vertices[3].y = y2;
  for (auto &vertex : mouse_vertices) {
    // unzoom the x coordinate
    vertex.x = unzoomX(vertex.x);
    vertex.y *= max_y;
  }
}

void
BarGraphRenderer::subscribe(BarGraphSubscriber *s) {
  subscriber = s;
}

void
BarGraphRenderer::selectMouseArea(bool shift) {
  if (!subscriber)
    return;

  std::vector<int> selected_renders;
  if (shift) {
    // preserve previous selection for shift-click
    for (int i = 0; i < selected.size(); ++i) {
      if (selected[i])
        selected_renders.push_back(i);
    }
  } else {
    for (int i = 0; i < selected.size(); ++i) {
      selected[i] = false;
    }
  }

  const float min_x = unzoomX(std::min(mouse_area[0].x, mouse_area[1].x));
  const float max_x = unzoomX(std::max(mouse_area[0].x, mouse_area[1].x));
  const float min_y = max_y * std::min(mouse_area[0].y, mouse_area[1].y);

  // iterate over the vertices, to see if the mouse area crosses each bar
  int current_render = -1;
  for (auto i = vertices.begin(); i < vertices.end(); i += 4) {
    BarVertices *current_bar = reinterpret_cast<BarVertices*>(&*i);
    current_render += 1;
    if (current_bar->bottom_right.x < min_x)
      continue;
    if (current_bar->top_left.y < min_y)
      continue;
    if (current_bar->bottom_left.x > max_x)
      break;
    if (!selected[current_render]) {
      // current_render should be added to selection
      selected_renders.push_back(current_render);
      selected[current_render] = true;
    }
  }
  subscriber->onBarSelect(selected_renders);
  mouse_area[0].x = -1;
  mouse_area[0].y = -1;
  mouse_area[1].x = -1;
  mouse_area[1].y = -1;
}

// solely for debugging: allows gdb to print vertices[n]
template class std::vector<BarGraphRenderer::Vertex>;

void
BarGraphRenderer::render() {
  glEnable(GL_BLEND);
  GL_CHECK();
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  GL_CHECK();

  glClearColor(1.0, 1.0, 1.0, 1.0);
  GL_CHECK();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  GL_CHECK();

  glUseProgram(prog);
  GL_CHECK();

  // set uniforms
  glUniform1f(uni_max_y, max_y);
  GL_CHECK();
  glUniform1f(uni_max_x, total_x);
  GL_CHECK();
  glUniform1f(uni_invert_y, invert_y);
  GL_CHECK();
  glUniform1f(uni_zoom_translate_x, zoom_translate);
  GL_CHECK();
  glUniform1f(uni_zoom_x, zoom);
  GL_CHECK();

  // bind vbo
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  GL_CHECK();

  // enable vbo
  glEnableVertexAttribArray(att_coord);
  GL_CHECK();

  // describe vbo
  glVertexAttribPointer(att_coord,           // attribute
                          2,                   // number of elements
                          // per vertex, here
                          // (x,y)
                          GL_FLOAT,            // the type of each element
                          GL_FALSE,            // take our values as-is
                          0,                   // no space between values
                          0);                  // use the vbo
  GL_CHECK();


  // draw gridlines
  static std::vector<Vertex> grid_lines(8);
  for (int i = 0; i < 8; i += 2) {
    grid_lines[i].x = 0;
    grid_lines[i].y = max_y * (0.2 + i * 0.1);
    grid_lines[i+1].x = total_x;
    grid_lines[i+1].y = max_y * (0.2 + i * 0.1);
  }
  const float grey_color[4] = { 0.2, 0.2, 0.2, 0.6 };
  glUniform4f(uni_bar_color, grey_color[0], grey_color[1],
                grey_color[2], grey_color[3]);
  GL_CHECK();
  glBufferData(GL_ARRAY_BUFFER, grid_lines.size() * sizeof(Vertex),
                 grid_lines.data(), GL_STATIC_DRAW);
  GL_CHECK();
  for (int i = 0; i < 4; ++i) {
    glDrawArrays(GL_LINE_STRIP, i*2, 2);
    GL_CHECK();
  }
  // glDisable(GL_BLEND);

  // buffer vertex data to vbo
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);
  GL_CHECK();

  // these indices encircle the first bar.
  std::vector<uint16_t> outline_indices = { 0, 1, 3, 2, 0 };

  for (GLint i = 0; i < vertices.size(); i += 4) {
    // draw the bars
    const float bar_color[4] = { 0.0, 0.0, 1.0, 1.0 };
    const float selected_color[4] = { 1.0, 1.0, 0.0, 1.0 };
    const float* color = selected[i/4] ? selected_color : bar_color;
    glUniform4f(uni_bar_color, color[0], color[1], color[2], color[3]);
    GL_CHECK();
    glDrawArrays(GL_TRIANGLE_STRIP, i, 4);
    GL_CHECK();

    // draw a thin border around each bar
    const float black_color[4] = { 0.0, 0.0, 0.0, 1.0 };
    glUniform4f(uni_bar_color, black_color[0], black_color[1],
                  black_color[2], black_color[3]);
    GL_CHECK();
    glDrawElements(GL_LINE_STRIP, 5, GL_UNSIGNED_SHORT,
                     outline_indices.data());
    GL_CHECK();

    // increment the indices to reflect the outline of the next bar
    for (auto &i : outline_indices) {
      i += 4;
    }
  }

  if ((mouse_area[0].x > 0) ||
      (mouse_area[0].y > 0) ||
      (mouse_area[1].x > 0) ||
      (mouse_area[1].y > 0)) {
    // mouse selection is active, draw a selection rectangle

    // buffer data to vbo
    glBufferData(GL_ARRAY_BUFFER, mouse_vertices.size() * sizeof(Vertex),
                   mouse_vertices.data(), GL_STATIC_DRAW);
    GL_CHECK();

    // yellow selection box
    const float color[4] = { 0.5, 0.5, 0.5, 0.5 };
    glUniform4f(uni_bar_color, color[0], color[1], color[2], color[3]);
    GL_CHECK();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    GL_CHECK();
  }

  // disable vbo
  glDisableVertexAttribArray(att_coord);
  GL_CHECK();

  // unbind vbo
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  GL_CHECK();
}

void
BarGraphRenderer::setSelection(const std::set<int> &selection) {
  for (int i = 0; i < selected.size(); ++i)
    selected[i] = selection.find(i) != selection.end();
}

void
BarGraphRenderer::setZoom(float z, float t) {
  // c is [0..1]
  // translate is [-total_x..0]
  zoom = z;
  zoom_translate = t;
}
