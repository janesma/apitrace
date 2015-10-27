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

#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "glframe_glhelper.hpp"
#include "glframe_metrics.hpp"
#include "glframe_retrace.hpp"
#include "test_bargraph_ctx.hpp"

namespace glretrace {
class MetricsCallback : public OnFrameRetrace {
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
  void onMetricList(const std::vector<MetricId> &i,
                    const std::vector<std::string> &n) {
    ids = i;
    names = n;
  }
  void onMetrics(const MetricSeries &metricData,
                 ExperimentId experimentCount) {
    data = metricData;
    experiment_count = experimentCount;
  }
  std::vector<MetricId> ids;
  std::vector<std::string> names;
  MetricSeries data;
  ExperimentId experiment_count;
};

TEST(Metrics, ReadMetrics) {
  GlFunctions::Init();
  TestContext c;
  MetricsCallback cb;
  PerfMetrics p(&cb);
  EXPECT_EQ(cb.ids.size(), cb.names.size());
  // check ids are unique
  std::set<MetricId> mets;
  for (auto id : cb.ids) {
    EXPECT_EQ(mets.find(id), mets.end());
    mets.insert(id);
  }
}

static const char *test_file = CMAKE_CURRENT_SOURCE_DIR "/simple.test_trace";

TEST(Metrics, SingleMetricData) {
  retrace::setUp();
  GlFunctions::Init();
  TestContext c;
  MetricsCallback cb;
  PerfMetrics p(&cb);
  if (!cb.ids.size())
    return;
  bool found = false;
  for (int i = 0; i < cb.ids.size(); ++i) {
    if (cb.names[i] == "GPU Time Elapsed") {
      found = true;
      p.selectMetric(cb.ids[i]);
      break;
    }
  }
  EXPECT_TRUE(found);

  FrameRetrace rt;
  rt.openFile(test_file, 7, &cb);
  p.begin(RenderId(1));
  rt.retraceRenderTarget(RenderId(1), 0, glretrace::NORMAL_RENDER,
                         glretrace::STOP_AT_RENDER, &cb);
  p.end();
  p.publish(ExperimentId(1), &cb);
  EXPECT_EQ(cb.experiment_count.count(), 1);
  for (float d : cb.data.data) {
    std::cout << "data: " << d << std::endl;
    EXPECT_GT(d, 0.0);
  }
  retrace::cleanUp();
}

TEST(Metrics, FrameMetricData) {
  retrace::setUp();
  GlFunctions::Init();
  TestContext c;
  MetricsCallback cb;
  PerfMetrics p(&cb);
  if (!cb.ids.size())
    return;
  MetricId id;
  for (int i = 0; i < cb.ids.size(); ++i) {
    if (cb.names[i] == "GPU Time Elapsed") {
      id = cb.ids[i];
      break;
    }
  }
  EXPECT_GT(id(), 0);

  FrameRetrace rt;
  rt.openFile(test_file, 7, &cb);
  const std::vector<MetricId> mets = { id };
  const ExperimentId experiment(1);
  rt.retraceMetrics(mets, experiment, &cb);
  EXPECT_EQ(cb.experiment_count.count(), 1);
  for (float d : cb.data.data) {
    std::cout << "data: " << d << std::endl;
    EXPECT_GT(d, 0.0);
  }
  retrace::cleanUp();
}

}  // namespace glretrace
