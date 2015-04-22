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

#include <unistd.h>

#include <vector>
#include "glws.hpp"
#include <gtest/gtest.h>

#include "glframe_retrace.hpp"

using glretrace::FrameRetrace;
using glretrace::RenderBookmark;

TEST(Build, Cmake)
{
    
}

// TODO(majanes) find a way to make this available
static const char *test_file = "/home/majanes/src/apitrace/retrace/daemon/test/simple.trace";
// static const char *test_file = "/home/majanes/.steam/steam/steamapps/common/dota/dota_linux.2.trace"

TEST(Daemon, LoadFile)
{
    glws::init();

    FrameRetrace rt(test_file, 7);
    std::vector<RenderBookmark> renders = rt.getRenders();
    EXPECT_EQ(renders.size(), 2);  // 1 for clear, 1 for draw
    for (int i = 0; i < renders.size(); ++i)
    {
        rt.retraceRenderTarget(renders[i], 0, glretrace::NORMAL_RENDER,
                               glretrace::STOP_AT_RENDER, NULL);
    }
}
