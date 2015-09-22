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
#include "test_bargraph_ctx.hpp"

using glretrace::BarGraphRenderer;
using glretrace::BarMetrics;
using glretrace::GlFunctions;
using glretrace::TestContext;

TEST(BarGraph, Create) {
  GlFunctions::Init();
  TestContext c;
  BarGraphRenderer r;
}

struct Pixel {
  unsigned char red;
  unsigned char blue;
  unsigned char green;
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
  c.swapBuffers();
  c.swapBuffers();

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

