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

#include <unistd.h>

#include <gtest/gtest.h>

#include <vector>

#include "glframe_bargraph.hpp"
#include "glframe_glhelper.hpp"
#include "glframe_qbargraph.hpp"
#include "glframe_qselection.hpp"
#include "test_bargraph_ctx.hpp"
#include "test_selection.hpp"

using glretrace::BarGraphRenderer;
using glretrace::BarGraphSubscriber;
using glretrace::BarGraphView;
using glretrace::BarMetrics;
using glretrace::GlFunctions;
using glretrace::QBarGraphRenderer;
using glretrace::QSelection;
using glretrace::SelectionObserver;
using glretrace::TestContext;

TEST(BarGraph, Create) {
  GlFunctions::Init();
  TestContext c;
  BarGraphRenderer r;
}

struct Pixel {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char alpha;
};

TEST(BarGraph, Render) {
  GlFunctions::Init();
  TestContext c;
  BarGraphRenderer r;
  r.render();

  Pixel data;
  GlFunctions::ReadPixels(500, 500, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);

  // check that the clearcolor is white
  EXPECT_EQ(data.red, 255);
  EXPECT_EQ(data.blue, 255);
  EXPECT_EQ(data.green, 255);
  EXPECT_EQ(data.alpha, 255);
}


TEST(BarGraph, MultiBar) {
  GlFunctions::Init();
  TestContext c;
  BarGraphRenderer r;
  std::vector<BarMetrics> bars(3);
  bars[0].metric1 = 25;
  bars[0].metric2 = 5;
  bars[1].metric1 = 50;
  bars[1].metric2 = 10;
  bars[2].metric1 = 10;
  bars[2].metric2 = 2;
  r.setBars(bars);
  r.render();

  Pixel data;

  // top left should be at 50%
  GlFunctions::ReadPixels(0, 501, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
  EXPECT_EQ(data.red, 255);
  GlFunctions::ReadPixels(0, 499, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
  EXPECT_EQ(data.red, 0);

  // center should be at 100%
  GlFunctions::ReadPixels(500, 999, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
  EXPECT_EQ(data.red, 0);

  // top right should be at 20%
  GlFunctions::ReadPixels(999, 199, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
  EXPECT_EQ(data.red, 0);
  GlFunctions::ReadPixels(999, 201, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
  EXPECT_EQ(data.red, 255);
}

TEST(BarGraph, BarSpacing) {
  GlFunctions::Init();
  TestContext c;
  BarGraphRenderer r;
  std::vector<BarMetrics> bars(2);
  bars[0].metric1 = 25;
  bars[0].metric2 = 0;
  bars[1].metric1 = 50;
  bars[1].metric2 = 0;
  r.setBars(bars);
  r.render();

  Pixel data;

  // whitespace should be at center
  GlFunctions::ReadPixels(500, 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
  EXPECT_EQ(data.red, 255);
  EXPECT_EQ(data.blue, 255);
  EXPECT_EQ(data.green, 255);
}

TEST(BarGraph, MouseSelect) {
  GlFunctions::Init();
  TestContext c;
  BarGraphRenderer r;
  std::vector<BarMetrics> bars(2);
  bars[0].metric1 = 25;
  bars[0].metric2 = 0;
  bars[1].metric1 = 50;
  bars[1].metric2 = 0;
  r.setBars(bars);
  r.setMouseArea(0.25, 0.25, 0.75, 0.75);
  r.render();

  Pixel data;

  // grey blended on white should be at center
  GlFunctions::ReadPixels(500, 500, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
  EXPECT_EQ(data.red, 191);
  EXPECT_EQ(data.green, 191);
  EXPECT_EQ(data.blue, 191);
  // alpha blend bug at rib's mesa branchpoint
  // EXPECT_EQ(data.alpha, 255);

  // white should be at the bottom
  GlFunctions::ReadPixels(500, 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
  EXPECT_EQ(data.red, 255);
  EXPECT_EQ(data.green, 255);
  EXPECT_EQ(data.blue, 255);
  EXPECT_EQ(data.alpha, 255);

  // grey on blue blend should be on the bar
  GlFunctions::ReadPixels(300, 300, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
  EXPECT_EQ(data.red, 64);
  EXPECT_EQ(data.green, 64);
  EXPECT_EQ(data.blue, 191);
  // alpha blend bug at rib's mesa branchpoint
  // EXPECT_EQ(data.alpha, 255);

  // turn off mouse area, check that the mouse rect is gone
  r.setMouseArea(0.0, 0.0, 0.0, 0.0);
  r.render();

  // white should be at center
  GlFunctions::ReadPixels(500, 500, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
  EXPECT_EQ(data.red, 255);
  EXPECT_EQ(data.green, 255);
  EXPECT_EQ(data.blue, 255);
  EXPECT_EQ(data.alpha, 255);

  // blue should be on the bar
  GlFunctions::ReadPixels(300, 300, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &data);
  EXPECT_EQ(data.red, 0);
  EXPECT_EQ(data.green, 0);
  EXPECT_EQ(data.blue, 255);
  EXPECT_EQ(data.alpha, 255);
}

TEST(BarGraph, SelectedBar) {
  GlFunctions::Init();
  TestContext c;
  BarGraphRenderer r;
  std::vector<BarMetrics> bars(2);
  bars[0].metric1 = 25;
  bars[0].metric2 = 0;
  bars[1].metric1 = 50;
  bars[1].metric2 = 0;
  r.setBars(bars);
  r.setMouseArea(0.25, 0.25, 0.75, 0.75);
  r.render();
  // todo: check colors in this test
}

class MockSubscriber : public BarGraphSubscriber {
 public:
  void onBarSelect(const std::vector<int> s) {
    selection = s;
  }
  std::vector<int> selection;
};

TEST(BarGraph, MouseSelectBars) {
  GlFunctions::Init();
  TestContext c;
  BarGraphRenderer r;
  std::vector<BarMetrics> bars(2);
  bars[0].metric1 = 25;
  bars[0].metric2 = 0;
  // bars[0].selected = true;
  bars[1].metric1 = 50;
  bars[1].metric2 = 0;
  MockSubscriber s;
  r.subscribe(&s);
  r.setBars(bars);
  r.setMouseArea(0.0, 0.0, 0.5, 0.5);
  r.selectMouseArea();
  EXPECT_EQ(s.selection, std::vector<int> {0});
  r.setMouseArea(0.5, 0.0, 1.0, 0.5);
  r.selectMouseArea();
  EXPECT_EQ(s.selection, std::vector<int> {1});
  r.setMouseArea(0.0, 0.0, 1.0, 0.5);
  r.selectMouseArea();
  EXPECT_EQ(s.selection, (std::vector<int> {0, 1}));
}

