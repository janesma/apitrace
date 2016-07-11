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

#include "glframe_retrace_skeleton.hpp"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>

#include <string>
#include <vector>

#include "glframe_retrace.hpp"
#include "glframe_socket.hpp"
#include "playback.pb.h" // NOLINT

using glretrace::FrameRetraceSkeleton;
using glretrace::FrameRetrace;
using glretrace::RenderTargetType;
using glretrace::RenderOptions;
using glretrace::Socket;
using glretrace::RenderId;
using glretrace::ExperimentId;
using glretrace::MetricId;
using glretrace::MetricSeries;
using ApiTrace::RetraceResponse;
using ApiTrace::RetraceRequest;
using google::protobuf::io::ArrayInputStream;
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::CodedOutputStream;
using google::protobuf::io::ArrayOutputStream;

FrameRetraceSkeleton::FrameRetraceSkeleton(Socket *sock)
    : Thread("retrace_skeleton"), m_socket(sock),
      m_frame(new FrameRetrace),
      m_remaining_metrics_requests(0),
      m_multi_metrics_response(new RetraceResponse) {
}

void
FrameRetraceSkeleton::Run() {
  while (true) {
    // leading 4 bytes is the message length
    uint32_t msg_len;
    if (!m_socket->Read(&msg_len)) {
      std::cout << "no read: len\n";
      return;
    }
    // std::cout << "read len: " << msg_len << "\n";
    m_buf.resize(msg_len);
    if (!m_socket->ReadVec(&m_buf)) {
      std::cout << "no read: buf\n";
      return;
    }

    const size_t buf_size = m_buf.size();
    ArrayInputStream array_in(m_buf.data(), buf_size);
    CodedInputStream coded_in(&array_in);
    RetraceRequest request;
    CodedInputStream::Limit msg_limit = coded_in.PushLimit(buf_size);
    request.ParseFromCodedStream(&coded_in);
    coded_in.PopLimit(msg_limit);
    switch (request.requesttype()) {
      case ApiTrace::OPEN_FILE_REQUEST:
        {
          auto of = request.fileopen();
          m_frame->openFile(of.filename(), of.framenumber(), this);
          break;
        }
      case ApiTrace::RENDER_TARGET_REQUEST:
        {
          assert(request.has_rendertarget());
          auto rt = request.rendertarget();
          // std::cout << "skel rt: "
          //           << rt.renderid() << ", "
          //           << rt.type() << ", "
          //           << rt.options() << ", "
          //           << "\n";

          m_frame->retraceRenderTarget(glretrace::RenderId(rt.renderid()),
                                       0,
                                       (RenderTargetType)rt.type(),
                                       (RenderOptions)rt.options(),
                                       this);
          break;
        }
      case ApiTrace::METRICS_REQUEST:
        {
          assert(request.has_metrics());
          auto met = request.metrics();
          std::vector<MetricId> ids;
          m_remaining_metrics_requests = met.metric_ids().size();
          for (int i : met.metric_ids())
            ids.push_back(MetricId(i));
          m_frame->retraceMetrics(ids,
                                  ExperimentId(met.experiment_count()),
                                  this);
          break;
        }
      case ApiTrace::SHADER_ASSEMBLY_REQUEST:
        {
          assert(request.has_shaderassembly());
          auto shader = request.shaderassembly();
          m_frame->retraceShaderAssembly(glretrace::RenderId(shader.renderid()),
                                         this);
          break;
        }
      case ApiTrace::REPLACE_SHADERS_REQUEST:
        {
          assert(request.has_shaders());
          auto shader = request.shaders();
          m_frame->replaceShaders(RenderId(shader.render_id()),
                                  ExperimentId(shader.experiment_count()),
                                  shader.vs(),
                                  shader.fs(),
                                  this);
          break;
        }
    }
  }
}

void
writeResponse(Socket *s,
              const RetraceResponse &response,
              std::vector<unsigned char> *buf) {
  const uint32_t write_size = response.ByteSize();
  // std::cout << "response: writing size: " << write_size << "\n";
  if (!s->Write(write_size)) {
    std::cout << "no write: len\n";
    return;
  }

  buf->clear();
  buf->resize(write_size);
  ArrayOutputStream array_out(buf->data(), write_size);
  CodedOutputStream coded_out(&array_out);
  response.SerializeToCodedStream(&coded_out);
  if (!s->WriteVec(*buf)) {
    std::cout << "no write: buf\n";
    return;
  }
}

void
FrameRetraceSkeleton::onFileOpening(bool finished,
                                    uint32_t percent_complete) {
  RetraceResponse proto_response;
  auto status = proto_response.mutable_filestatus();
  status->set_finished(finished);
  status->set_percent_complete(percent_complete);
  writeResponse(m_socket, proto_response, &m_buf);
}

void
FrameRetraceSkeleton::onShaderAssembly(RenderId renderId,
                                       const std::string &vertex_shader,
                                       const std::string &vertex_ir,
                                       const std::string &vertex_vec4,
                                       const std::string &fragment_shader,
                                       const std::string &fragment_ir,
                                       const std::string &fragment_simd8,
                                       const std::string &fragment_simd16,
                                       const std::string &fragment_nir_ssa,
                                       const std::string &fragment_nir_final) {
  RetraceResponse proto_response;
  auto shader = proto_response.mutable_shaderassembly();
  shader->set_vertex_shader(vertex_shader);
  shader->set_vertex_ir(vertex_ir);
  shader->set_vertex_vec4(vertex_vec4);
  shader->set_fragment_shader(fragment_shader);
  shader->set_fragment_ir(fragment_ir);
  shader->set_fragment_simd8(fragment_simd8);
  shader->set_fragment_simd16(fragment_simd16);
  shader->set_fragment_ssa(fragment_nir_ssa);
  shader->set_fragment_nir(fragment_nir_final);
  writeResponse(m_socket, proto_response, &m_buf);
}

typedef std::vector<unsigned char> Buffer;

void
FrameRetraceSkeleton::onRenderTarget(RenderId renderId,
                                     RenderTargetType type,
                                     const Buffer &pngImageData) {
  // std::cout << "onRenderTarget\n";
  RetraceResponse proto_response;
  auto rt_response = proto_response.mutable_rendertarget();
  std::string *image = rt_response->mutable_image();
  image->assign((const char *)pngImageData.data(), pngImageData.size());
  writeResponse(m_socket, proto_response, &m_buf);
  // const uint32_t write_size = proto_response.ByteSize();
  // std::cout << "onRenderTarget: writing size: " << write_size << "\n";
  // if (! m_socket->Write(write_size)) {
  //     std::cout << "no write: len\n";
  //     return;
  // }

  // m_buf.clear();
  // m_buf.resize(write_size);
  // ArrayOutputStream array_out(m_buf.data(), write_size);
  // CodedOutputStream coded_out(&array_out);
  // proto_response.SerializeToCodedStream(&coded_out);
  // std::cout << "onRenderTarget: writing image\n";
  // if (!m_socket->WriteVec(m_buf)) {
  //     std::cout << "no write: buf\n";
  //     return;
  // }
}

void
FrameRetraceSkeleton::onShaderCompile(RenderId renderId,
                                      ExperimentId experimentCount,
                                      bool status,
                                      const std::string &errorString) {
  RetraceResponse proto_response;
  auto shader = proto_response.mutable_shadersdata();
  shader->set_render_id(renderId());
  shader->set_experiment_count(experimentCount());
  shader->set_status(status);
  shader->set_message(errorString);
  writeResponse(m_socket, proto_response, &m_buf);
}

void
FrameRetraceSkeleton::onMetricList(const std::vector<MetricId> &ids,
                                   const std::vector<std::string> &names) {
  RetraceResponse proto_response;
  auto metrics_response = proto_response.mutable_metricslist();
  for (auto i : ids)
    metrics_response->add_metric_ids(i());
  for (auto i : names)
    metrics_response->add_metric_names(i);
  writeResponse(m_socket, proto_response, &m_buf);
}

void
FrameRetraceSkeleton::onMetrics(const MetricSeries &metricData,
                                ExperimentId experimentCount) {
  auto metrics_response = m_multi_metrics_response->mutable_metricsdata();
  metrics_response->set_experiment_count(experimentCount());
  ApiTrace::MetricSeries *s = metrics_response->add_metric_data();
  s->set_metric_id(metricData.metric());
  for (auto d : metricData.data) {
    s->add_data(d);
  }

  --m_remaining_metrics_requests;
  if (m_remaining_metrics_requests > 0)
    return;

  // else
  writeResponse(m_socket, *m_multi_metrics_response, &m_buf);
  m_remaining_metrics_requests = 0;
  m_multi_metrics_response->Clear();
}
