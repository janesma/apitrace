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

#ifndef _GLFRAME_RETRACE_SKELETON_HPP_
#define _GLFRAME_RETRACE_SKELETON_HPP_

#include <string>
#include <vector>

#include "glframe_thread.hpp"
#include "glframe_retrace.hpp"

namespace ApiTrace {
class RetraceResponse;
}  // namespace ApiTrace

namespace glretrace {
class Socket;
class FrameRetrace;

// handles retrace requests coming in through a socket, executes them,
// and formats responses back through the socket
class FrameRetraceSkeleton : public Thread,
                             public OnFrameRetrace {
 public:
  // call once, to set up the retrace socket, and shut it down at
  // exit
  explicit FrameRetraceSkeleton(Socket *sock);
  virtual void Run();

  // callback responses, to be sent through the socket to the caller
  virtual void onShaderAssembly(RenderId renderId,
                                const ShaderAssembly &vertex_shader,
                                const ShaderAssembly &fragment_shader,
                                const ShaderAssembly &tess_control_shader,
                                const ShaderAssembly &tess_eval_shader,
                                const ShaderAssembly &geom_shader);
  virtual void onFileOpening(bool finished,
                             uint32_t percent_complete);
  virtual void onRenderTarget(RenderId renderId, RenderTargetType type,
                              const std::vector<unsigned char> &pngImageData);
  virtual void onShaderCompile(RenderId renderId,
                               ExperimentId experimentCount,
                               bool status,
                               const std::string &errorString);
  virtual void onMetricList(const std::vector<MetricId> &ids,
                            const std::vector<std::string> &names);
  virtual void onMetrics(const MetricSeries &metricData,
                         ExperimentId experimentCount);
  virtual void onApi(RenderId renderid,
                     const std::vector<std::string> &api_calls);

 private:
  Socket *m_socket;
  std::vector<unsigned char> m_buf;
  FrameRetrace *m_frame;
  int m_remaining_metrics_requests;

  // for aggregating metrics callbacks on a series of requests
  ApiTrace::RetraceResponse *m_multi_metrics_response;
};

}  // namespace glretrace

#endif  // _GLFRAME_RETRACE_SKELETON_HPP_
