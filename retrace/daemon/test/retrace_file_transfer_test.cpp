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

#include <string>
#include <vector>

#include "glframe_os.hpp"
#include "glframe_retrace_interface.hpp"
#include "glframe_retrace_skeleton.hpp"
#include "glframe_retrace_stub.hpp"
#include "glframe_socket.hpp"
#include "retrace_test.hpp"

using glretrace::ExperimentId;
using glretrace::FrameRetraceSkeleton;
using glretrace::FrameRetraceSkeleton;
using glretrace::FrameRetraceSkeleton;
using glretrace::FrameRetraceSkeleton;
using glretrace::FrameRetraceStub;
using glretrace::IFrameRetrace;
using glretrace::MetricId;
using glretrace::MetricSeries;
using glretrace::OnFrameRetrace;
using glretrace::RenderId;
using glretrace::RenderOptions;
using glretrace::RenderSelection;
using glretrace::RenderTargetType;
using glretrace::SelectionId;
using glretrace::ServerSocket;
using glretrace::ShaderAssembly;
using glretrace::Socket;

class FileTransfer : public IFrameRetrace {
  void openFile(const std::string &filename,
                const std::vector<unsigned char> &md5,
                uint64_t fileSize,
                uint32_t frameNumber,
                OnFrameRetrace *callback) {
    callback->onFileOpening(false, true, 101);
  }
  void retraceRenderTarget(RenderId renderId,
                           int render_target_number,
                           RenderTargetType type,
                           RenderOptions options,
                           OnFrameRetrace *callback) const {}
  void retraceShaderAssembly(RenderId renderId,
                             OnFrameRetrace *callback) {}
  void retraceMetrics(const std::vector<MetricId> &ids,
                      ExperimentId experimentCount,
                      OnFrameRetrace *callback) const {}
  virtual void retraceAllMetrics(const RenderSelection &selection,
                                 ExperimentId experimentCount,
                                 OnFrameRetrace *callback) const {}
  void replaceShaders(RenderId renderId,
                      ExperimentId experimentCount,
                      const std::string &vs,
                      const std::string &fs,
                      const std::string &tessControl,
                      const std::string &tessEval,
                      const std::string &geom,
                      OnFrameRetrace *callback) {}
  void retraceApi(RenderId renderId,
                  OnFrameRetrace *callback) {}
};

class FileTransferCB : public OnFrameRetrace {
 public:
  FileTransferCB() : m_needUpload(false) {}
  void onFileOpening(bool needUpload,
                     bool finished,
                     uint32_t percent_complete) {
    m_needUpload |= needUpload;
  }
  void onShaderAssembly(RenderId renderId,
                        const ShaderAssembly &vertex,
                        const ShaderAssembly &fragment,
                        const ShaderAssembly &tess_control,
                        const ShaderAssembly &tess_eval,
                        const ShaderAssembly &geon) {}
  void onRenderTarget(RenderId renderId, RenderTargetType type,
                      const uvec & pngImageData) {}
  void onMetricList(const std::vector<MetricId> &ids,
                    const std::vector<std::string> &names) {}
  void onMetrics(const MetricSeries &metricData,
                 ExperimentId experimentCount,
                 SelectionId selectionCount) {}
  void onShaderCompile(RenderId renderId,
                       ExperimentId experimentCount,
                       bool status,
                       const std::string &errorString) {}
  void onApi(RenderId renderId,
             const std::vector<std::string> &api_calls) {}
  bool m_needUpload;
};

static const char *test_file = CMAKE_CURRENT_SOURCE_DIR "/simple.trace";

class UploadSkel : public FrameRetraceSkeleton {
 public:
  UploadSkel(Socket *s, IFrameRetrace *f) :
      FrameRetraceSkeleton(s, f) {
    m_force_upload = true;
  }
  void noForce() { m_force_upload = false; }
};

bool exists(const char *f) {
  FILE * fh = fopen(f, "r");
  if (!fh)
    return false;
  fclose(fh);
  return true;
}

TEST(FrameRetrace, MD5) {
  std::vector<unsigned char> md5, md5b;
  uint32_t fileSize, fileSizeb;
  get_md5(test_file, &md5, &fileSize);
  get_md5(test_file, &md5b, &fileSizeb);
  EXPECT_TRUE(md5 == md5b);
  EXPECT_EQ(fileSize, fileSizeb);
}

TEST(FrameRetrace, FileTransfer) {
  FrameRetraceStub stub;
  FileTransfer frameretrace;
  ServerSocket server(0);
  stub.Init("localhost", server.GetPort());
  UploadSkel skel(server.Accept(), &frameretrace);
  skel.Start();
  std::vector<unsigned char> md5;
  uint32_t fileSize;

  get_md5(test_file, &md5, &fileSize);

  // clear cached file
  std::stringstream cache_file_s;
  cache_file_s << glretrace::application_cache_directory();
  cache_file_s << std::hex;
  for (auto byte : md5)
    cache_file_s << static_cast<unsigned int>(byte);
  cache_file_s << ".trace";
  const std::string cs = cache_file_s.str();
  const char *target = cs.c_str();
  remove(target);
  EXPECT_FALSE(exists(target));

  FileTransferCB cb;
  stub.openFile(std::string(test_file), md5,
                fileSize, 1, &cb);
  stub.Flush();
  EXPECT_TRUE(cb.m_needUpload);
  EXPECT_TRUE(exists(target));

  cb.m_needUpload = false;
  skel.noForce();
  // resend bogus path, and verify that upload is not requested (it is
  // in the cache)
  stub.openFile(std::string("bogus_path"), md5,
                fileSize, 1, &cb);
  stub.Flush();
  EXPECT_FALSE(cb.m_needUpload);
  stub.Shutdown();
  skel.Join();
  remove(target);
}

