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

using glretrace::ExperimentId;
using glretrace::FrameRetrace;
using glretrace::MetricId;
using glretrace::MetricSeries;
using glretrace::OnFrameRetrace;
using glretrace::RenderId;
using glretrace::RenderTargetType;

TEST(Build, Cmake)
{
    
}

static const char *test_file = CMAKE_CURRENT_SOURCE_DIR "/simple.test_trace";

class NullCallback : public OnFrameRetrace {
 public:
  void onFileOpening(bool finished,
                     uint32_t percent_complete) {}
  void onShaderAssembly(RenderId renderId,
                        const std::string &vertex_shader,
                        const std::string &vertex_ir,
                        const std::string &vertex_vec4,
                        const std::string &fragemnt_shader,
                        const std::string &fragemnt_ir,
                        const std::string &fragemnt_simd8,
                        const std::string &fragemnt_simd16) {}
  void onRenderTarget(RenderId renderId, RenderTargetType type,
                      const uvec & pngImageData) {}
  void onShaderCompile(RenderId renderId, int status,
                       std::string errorString) {}
  void onMetricList(const std::vector<MetricId> ids,
                    const std::vector<std::string> &names) {}
  void onMetrics(const MetricSeries &metricData,
                 ExperimentId experimentCount) {}
};


TEST(Daemon, LoadFile)
{
  retrace::setUp();

  NullCallback cb;
  
  FrameRetrace rt;
  rt.openFile(test_file, 7, &cb);
  //FrameRetrace rt(test_file, 7);
  int renderCount = rt.getRenderCount();
  EXPECT_EQ(renderCount, 2);  // 1 for clear, 1 for draw
  for (int i = 0; i < renderCount; ++i)
  {
    rt.retraceRenderTarget(RenderId(i), 0, glretrace::NORMAL_RENDER,
                           glretrace::STOP_AT_RENDER, &cb);
  }
}
