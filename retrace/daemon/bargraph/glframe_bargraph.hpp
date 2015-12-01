// Copyright (C) Intel Corp.  2015.  All Rights Reserved.

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

#ifndef _GLFRAME_BARGRAPH_HPP_
#define _GLFRAME_BARGRAPH_HPP_

#include <QOpenGLFunctions>

#include <set>
#include <string>
#include <vector>

namespace glretrace {


// helper class to encapsulate color, dimension and placement of a
// metric bar.  Coordinates indicate the lower left/top right of the
// bar.  The coordinate system is 0.0 - 1.0 on both axis.
class BarMetrics {
 public:
  float metric1;  // bar height
  float metric2;  // bar width
};

class BarGraphSubscriber {
 public:
  virtual void onBarSelect(const std::vector<int> selection) = 0;
  virtual ~BarGraphSubscriber() {}
};

// - BarGraphRenderer
//   - draws image, including all details:
//     - selected ergs
//     - mouse area
//     - bars
//   - Independent of Qt
class BarGraphRenderer : protected QOpenGLFunctions {
 public:
  explicit BarGraphRenderer(bool invert = false);  // Qt draws top-to-bottom
  void setBars(const std::vector<BarMetrics> &bars);
  void setSelection(const std::set<int> &selection);
  void setMouseArea(float x1, float y1, float x2, float y2);
  void selectMouseArea();  // on click or drag-release
  void render();
  void subscribe(BarGraphSubscriber *s);

 private:
  static const char *vshader, *fshader;
  GLuint vbo;
  GLint att_coord, uni_max_x, uni_max_y, uni_invert_y, uni_bar_color, prog;
  struct Vertex {
    float x;
    float y;
  };
  struct BarVertices {
    Vertex bottom_left;
    Vertex top_left;
    Vertex bottom_right;
    Vertex top_right;
  };

  void CheckError(const char * file, int line);
  void GetCompileError(GLint shader, std::string *message);
  void PrintCompileError(GLint shader);

  std::vector<bool> selected;
  std::vector<Vertex> vertices;
  std::vector<Vertex> mouse_vertices;

  // selection box.  coordinates are in the 0.0 - 1.0 range
  std::vector<Vertex> mouse_area;

  float max_y, total_x, invert_y;
  BarGraphSubscriber *subscriber;
};

}  // namespace glretrace

#endif  // _GLFRAME_BARGRAPH_HPP_
