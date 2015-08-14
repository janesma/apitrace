// Copyright (C) Intel Corp.  2014.  All Rights Reserved.

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

#ifndef RETRACE_DAEMON_BARGRAPH_GLFRAME_BARGRAPHMODEL_H_
#define RETRACE_DAEMON_BARGRAPH_GLFRAME_BARGRAPHMODEL_H_

#include <QtQuick/QQuickFramebufferObject>

#include <vector>

#include "glframe_retrace.hpp"
#include "glframe_traits.hpp"

namespace glretrace {

class BarGraphRenderer;


// Converts native units to the 0.0 -> 1.0 scale required my
// BarGraphRenderer.  Calculates height and width of all metric bars.
class BarGraphModel : NoCopy, NoAssign, NoMove {
 public:
  void onMetrics(const std::vector<float> &metric1,
                 const std::vector<float> &metric2);
  void onMouseSelect(int x1, int y1, int x2, int y2);
  void onSelected(const std::vector<RenderId> &renders,
                  int selectionCount);
  const void updateRenderer(BarGraphRenderer *renderer);
 private:
  // requires reference to RenderSelection, which will be called when the mouse
  // area crosses a bar.
};

}  // namespace glretrace

#endif  // RETRACE_DAEMON_BARGRAPH_GLFRAME_BARGRAPHMODEL_H_
