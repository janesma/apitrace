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
 * Authors:
 *   Mark Janes <mark.a.janes@intel.com>
 **************************************************************************/

#include <gtest/gtest.h>

#include <map>
#include <vector>
#include <string>

#include "glws.hpp"

#include "retrace_test.hpp"
#include "glframe_retrace.hpp"
#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"

using glretrace::ErrorSeverity;
using glretrace::ExperimentId;
using glretrace::FrameRetrace;
using glretrace::GlFunctions;
using glretrace::Logger;
using glretrace::MetricId;
using glretrace::MetricSeries;
using glretrace::OnFrameRetrace;
using glretrace::RenderId;
using glretrace::RenderSelection;
using glretrace::RenderSequence;
using glretrace::RenderTargetType;
using glretrace::SelectionId;
using glretrace::ShaderAssembly;

TEST(Build, Cmake) {
}

static const char *test_file = CMAKE_CURRENT_SOURCE_DIR "/simple.trace";

class NullCallback : public OnFrameRetrace {
 public:
  void onFileOpening(bool needUpload,
                     bool finished,
                     uint32_t frame_count) {}
  void onShaderAssembly(RenderId renderId,
                        SelectionId selectionCount,
                        const ShaderAssembly &vertex,
                        const ShaderAssembly &fragment,
                        const ShaderAssembly &tess_control,
                        const ShaderAssembly &tess_eval,
                        const ShaderAssembly &geom,
                        const ShaderAssembly &comp) {
    fs.push_back(fragment.shader);
  }
  void onRenderTarget(SelectionId selectionCount,
                      ExperimentId experimentCount,
                      const uvec & pngImageData) {
    ++renderTargetCount;
  }
  void onShaderCompile(RenderId renderId, ExperimentId count,
                       bool status,
                       const std::string &errorString) {
    compile_error = errorString;
  }
  void onMetricList(const std::vector<MetricId> &ids,
                    const std::vector<std::string> &names,
                    const std::vector<std::string> &desc) {}
  void onMetrics(const MetricSeries &metricData,
                 ExperimentId experimentCount,
                 SelectionId selectionCount) {}
  void onApi(SelectionId selectionCount,
             RenderId renderId,
             const std::vector<std::string> &api_calls) {
    last_selection = selectionCount;
    calls[renderId] = api_calls;
  }
  void onError(ErrorSeverity s, const std::string &message) {
    file_error = true;
  }
  int renderTargetCount;
  SelectionId last_selection;
  std::string compile_error;
  std::vector<std::string> fs;
  std::map<RenderId, std::vector<std::string>> calls;
  bool file_error;
};

void
get_md5(const std::string filename, std::vector<unsigned char> *md5,
        uint32_t *fileSize) {
  struct MD5Context md5c;
  MD5Init(&md5c);
  std::vector<unsigned char> buf(1024 * 1024);
  FILE * fh = fopen(filename.c_str(), "r");
  assert(fh);
  size_t total_bytes = 0;
  while (true) {
    const size_t bytes = fread(buf.data(), 1, 1024 * 1024, fh);
    total_bytes += bytes;
    MD5Update(&md5c, buf.data(), bytes);
    if (feof(fh))
      break;
    assert(!ferror(fh));
  }
  md5->resize(16);
  MD5Final(md5->data(), &md5c);
  *fileSize = total_bytes;
}

TEST_F(RetraceTest, LoadFile) {
  retrace::setUp();
  GlFunctions::Init();

  NullCallback cb;

  FrameRetrace rt;
  get_md5(test_file, &md5, &fileSize);
  rt.openFile(test_file, md5, fileSize, 7, &cb);
  if (cb.file_error)
    return;
  int renderCount = rt.getRenderCount();
  EXPECT_EQ(renderCount, 2);  // 1 for clear, 1 for draw
  cb.renderTargetCount = 0;
  for (int i = 0; i < renderCount; ++i) {
    // retrace the rt for each render
    RenderSelection s;
    s.id = SelectionId(0);
    s.series.push_back(RenderSequence(RenderId(i), RenderId(i+1)));
    rt.retraceRenderTarget(ExperimentId(0),
                           s,
                           glretrace::NORMAL_RENDER,
                           glretrace::STOP_AT_RENDER, &cb);
  }
  EXPECT_EQ(cb.renderTargetCount, 2);
  // retrace the rt for the full frame
  RenderSelection s;
  s.id = SelectionId(0);
  s.series.push_back(RenderSequence(RenderId(0), RenderId(2)));
  rt.retraceRenderTarget(ExperimentId(0),
                         s,
                         glretrace::NORMAL_RENDER,
                         glretrace::STOP_AT_RENDER, &cb);
  EXPECT_EQ(cb.renderTargetCount, 3);
}

TEST_F(RetraceTest, ReplaceShaders) {
  NullCallback cb;
  FrameRetrace rt;
  get_md5(test_file, &md5, &fileSize);
  rt.openFile(test_file, md5, fileSize, 7, &cb);
  if (cb.file_error)
    return;
  rt.replaceShaders(RenderId(1), ExperimentId(0), "bug", "blarb", "",
                    "", "", "", &cb);
  EXPECT_GT(cb.compile_error.size(), 0);
  RenderSelection rs;
  rs.id = SelectionId(1);
  rs.series.push_back(RenderSequence(RenderId(1), RenderId(2)));
  rt.retraceShaderAssembly(rs, &cb);
  EXPECT_GT(cb.fs.back().size(), 0);
  std::string vs("attribute vec2 coord2d;\n"
                 "varying vec2 v_TexCoordinate;\n"
                 "void main(void) {\n"
                 "  gl_Position = vec4(coord2d.x, -1.0 * coord2d.y, 0, 1);\n"
                 "  v_TexCoordinate = vec2(coord2d.x, coord2d.y);\n"
                 "}\n");
  rt.replaceShaders(RenderId(1), ExperimentId(0), vs, cb.fs.back(),
                    "", "", "", "", &cb);
  EXPECT_EQ(cb.compile_error.size(), 0);
}

TEST_F(RetraceTest, ApiCalls) {
  NullCallback cb;
  FrameRetrace rt;
  get_md5(test_file, &md5, &fileSize);
  rt.openFile(test_file, md5, fileSize, 7, &cb);
  if (cb.file_error)
    return;
  RenderSelection sel;
  sel.id = SelectionId(5);

  // retrace empty selection to get all api calls
  rt.retraceApi(sel, &cb);
  EXPECT_EQ(cb.calls.size(), 2);
  EXPECT_EQ(cb.last_selection, sel.id);

  // retrace single selection
  cb.calls.clear();
  sel.clear();
  sel.id = SelectionId(6);
  sel.push_back(0, 1);
  rt.retraceApi(sel, &cb);
  EXPECT_EQ(cb.calls.size(), 1);
  EXPECT_EQ(cb.last_selection, sel.id);

  // retrace single selection with 2 series
  cb.calls.clear();
  sel.id = SelectionId(7);
  sel.push_back(1, 2);
  rt.retraceApi(sel, &cb);
  EXPECT_EQ(cb.calls.size(), 2);
  EXPECT_EQ(cb.last_selection, sel.id);

  // retrace single selection with 1 multi-series
  cb.calls.clear();
  sel.clear();
  sel.id = SelectionId(8);
  sel.push_back(0, 2);
  rt.retraceApi(sel, &cb);
  EXPECT_EQ(cb.calls.size(), 2);
  EXPECT_EQ(cb.last_selection, sel.id);
}

TEST_F(RetraceTest, ShaderAssembly) {
  NullCallback cb;
  FrameRetrace rt;
  get_md5(test_file, &md5, &fileSize);
  rt.openFile(test_file, md5, fileSize, 7, &cb);
  if (cb.file_error)
    return;
  RenderSelection selection;
  std::string expected("uniform sampler2D texUnit;\n"
                       "varying vec2 v_TexCoordinate;\nvoid main(void) {\n"
                       "  gl_FragColor = texture2D(texUnit, v_TexCoordinate);\n"
                       "}");
  // retrace shaders for render 0
  selection.series.push_back(RenderSequence(RenderId(0), RenderId(1)));
  rt.retraceShaderAssembly(selection, &cb);
  EXPECT_EQ(cb.fs.size(), 1);
  EXPECT_EQ(cb.fs[0], expected);

  // retrace shaders for render 1
  cb.fs.clear();
  selection.series.clear();
  selection.series.push_back(RenderSequence(RenderId(1), RenderId(2)));
  rt.retraceShaderAssembly(selection, &cb);
  EXPECT_EQ(cb.fs.size(), 1);
  EXPECT_EQ(cb.fs[0], expected);

  // retrace shaders for render 0 & 1 (single sequence)
  cb.fs.clear();
  selection.series.clear();
  selection.series.push_back(RenderSequence(RenderId(0), RenderId(2)));
  rt.retraceShaderAssembly(selection, &cb);
  EXPECT_EQ(cb.fs.size(), 2);
  EXPECT_EQ(cb.fs[0], expected);
  EXPECT_EQ(cb.fs[1], expected);

  // retrace shaders for render 0 & 1 (separate sequence)
  cb.fs.clear();
  selection.series.clear();
  selection.series.push_back(RenderSequence(RenderId(0), RenderId(1)));
  selection.series.push_back(RenderSequence(RenderId(1), RenderId(2)));
  rt.retraceShaderAssembly(selection, &cb);
  EXPECT_EQ(cb.fs.size(), 2);
  EXPECT_EQ(cb.fs[0], expected);
  EXPECT_EQ(cb.fs[1], expected);
}
