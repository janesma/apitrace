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

#include "glframe_retrace_skeleton.hpp"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>

#include <string>
#include <sstream>
#include <vector>

#include "glframe_os.hpp"
#include "glframe_retrace_interface.hpp"
#include "glframe_retrace.hpp"
#include "glframe_socket.hpp"
#include "playback.pb.h" // NOLINT

using ApiTrace::RetraceRequest;
using ApiTrace::RetraceResponse;
using glretrace::ErrorSeverity;
using glretrace::ExperimentId;
using glretrace::FrameRetrace;
using glretrace::FrameRetraceSkeleton;
using glretrace::IFrameRetrace;
using glretrace::MetricId;
using glretrace::MetricSeries;
using glretrace::RenderId;
using glretrace::RenderOptions;
using glretrace::RenderTargetType;
using glretrace::SelectionId;
using glretrace::ShaderAssembly;
using glretrace::Socket;
using glretrace::UniformDimension;
using glretrace::UniformType;
using glretrace::application_cache_directory;
using google::protobuf::io::ArrayInputStream;
using google::protobuf::io::ArrayOutputStream;
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::CodedOutputStream;

FrameRetraceSkeleton::FrameRetraceSkeleton(Socket *sock,
                                           IFrameRetrace *frameretrace)
    : Thread("retrace_skeleton"),
      m_force_upload(false),
      m_socket(sock),
      m_frame(frameretrace),
      m_fatal_error(false),
      m_multi_metrics_response(new RetraceResponse) {
  if (!m_frame)
    m_frame = new FrameRetrace();
}

void
writeResponse(Socket *s,
              const RetraceResponse &response,
              std::vector<unsigned char> *buf) {
  const uint32_t write_size = response.ByteSize();
  if (!s->Write(write_size)) {
    return;
  }

  buf->clear();
  buf->resize(write_size);
  ArrayOutputStream array_out(buf->data(), write_size);
  CodedOutputStream coded_out(&array_out);
  response.SerializeToCodedStream(&coded_out);
  if (!s->WriteVec(*buf)) {
    return;
  }
}

void
makeRenderSelection(const ApiTrace::RenderSelection &sel,
                    glretrace::RenderSelection *selection) {
  selection->id = SelectionId(sel.selection_count());
  selection->series.resize(sel.render_series_size());
  for (int i = 0; i < sel.render_series_size(); ++i) {
    auto sequence = sel.render_series(i);
    auto &series = selection->series[i];
    series.begin = RenderId(sequence.begin());
    series.end = RenderId(sequence.end());
  }
}

void set_shader_assembly(const ShaderAssembly &assembly,
                         ApiTrace::ShaderAssembly *response) {
  response->set_shader(assembly.shader);
  response->set_ir(assembly.ir);
  response->set_nir_ssa(assembly.ssa);
  response->set_nir_final(assembly.nir);
  response->set_simd8(assembly.simd8);
  response->set_simd16(assembly.simd16);
  response->set_simd32(assembly.simd32);
  response->set_before_unification(assembly.beforeUnification);
  response->set_after_unification(assembly.afterUnification);
  response->set_before_optimization(assembly.beforeOptimization);
  response->set_const_coalescing(assembly.constCoalescing);
  response->set_gen_ir_lowering(assembly.genIrLowering);
  response->set_layout(assembly.layout);
  response->set_optimized(assembly.optimized);
  response->set_push_analysis(assembly.pushAnalysis);
  response->set_code_hoisting(assembly.codeHoisting);
  response->set_code_sinking(assembly.codeSinking);
}

void
FrameRetraceSkeleton::Run() {
  while (true) {
    // leading 4 bytes is the message length
    uint32_t msg_len;
    if (!m_socket->Read(&msg_len)) {
      return;
    }
    m_buf.resize(msg_len);
    if (!m_socket->ReadVec(&m_buf)) {
      return;
    }

    if (m_fatal_error)
      // after fatal error is encountered, do not process subsequent
      // requests.
      continue;

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
          std::string file_path = of.filename();
          const std::vector<unsigned char> vsum(of.md5sum().begin(),
                                                of.md5sum().end());
          std::stringstream cache_file_s;
          cache_file_s << application_cache_directory();
          cache_file_s << std::hex;
          for (auto byte : vsum)
            cache_file_s << static_cast<unsigned int>(byte);
          cache_file_s << ".trace";

          FILE * fh = fopen(of.filename().c_str(), "rb");
          if (!fh) {
            file_path = cache_file_s.str();
            fh = fopen(file_path.c_str(), "r");
          }
          if (!fh || m_force_upload) {
            // request file data
            file_path = cache_file_s.str();
            onFileOpening(true, false, 0);

            if (fh)
              fclose(fh);
            fh = fopen(file_path.c_str(), "wb");

            uint32_t bytes_received = 0;
            std::vector<unsigned char> buf;
            while (true) {
              uint32_t bytes;
              m_socket->Read(&bytes);
              buf.resize(bytes);
              m_socket->ReadVec(&buf);
              bytes_received += bytes;
              const uint32_t w_bytes = fwrite(buf.data(), 1, bytes, fh);
              assert(w_bytes == bytes);
              if (bytes_received == of.filesize())
                break;
            }
          }
          fclose(fh);

          m_frame->openFile(file_path, vsum, of.filesize(),
                            of.framenumber(), this);
          break;
        }
      case ApiTrace::RENDER_TARGET_REQUEST:
        {
          assert(request.has_rendertarget());
          auto rt = request.rendertarget();
          RenderSelection selection;
          makeRenderSelection(rt.render_selection(), &selection);
          m_frame->retraceRenderTarget(ExperimentId(rt.experiment_count()),
                                       selection,
                                       (RenderTargetType)rt.type(),
                                       (RenderOptions)rt.options(),
                                       this);
          // send empty message to signal the last response
          RetraceResponse proto_response;
          auto rt_resp = proto_response.mutable_rendertarget();
          rt_resp->set_selection_count(-1);
          rt_resp->set_experiment_count(-1);
          rt_resp->set_image("");
          ShaderAssembly s;
          writeResponse(m_socket, proto_response, &m_buf);
          break;
        }
      case ApiTrace::METRICS_REQUEST:
        {
          assert(request.has_metrics());
          auto met = request.metrics();
          std::vector<MetricId> ids;
          for (int i : met.metric_ids())
            ids.push_back(MetricId(i));
          m_multi_metrics_response->Clear();
          m_frame->retraceMetrics(ids,
                                  ExperimentId(met.experiment_count()),
                                  this);
          // callbacks have accumulated in m_multi_metrics_response
          writeResponse(m_socket, *m_multi_metrics_response, &m_buf);
          m_multi_metrics_response->Clear();
          break;
        }
      case ApiTrace::ALL_METRICS_REQUEST:
        {
          assert(request.has_allmetrics());
          auto met = request.allmetrics();
          RenderSelection selection;
          makeRenderSelection(met.selection(), &selection);
          m_multi_metrics_response->Clear();
          m_frame->retraceAllMetrics(selection,
                                     ExperimentId(met.experiment_count()),
                                     this);
          // callbacks have accumulated in m_multi_metrics_response
          writeResponse(m_socket, *m_multi_metrics_response, &m_buf);
          m_multi_metrics_response->Clear();
          break;
        }
      case ApiTrace::SHADER_ASSEMBLY_REQUEST:
        {
          assert(request.has_shaderassembly());
          auto shader = request.shaderassembly();
          const ExperimentId exp(shader.experiment_count());
          RenderSelection selection;
          makeRenderSelection(shader.render_selection(), &selection);
          m_frame->retraceShaderAssembly(selection,
                                         exp,
                                         this);
          // send empty message to signal the last response
          RetraceResponse proto_response;
          auto shader_resp = proto_response.mutable_shaderassembly();
          shader_resp->set_render_id(-1);
          shader_resp->set_selection_id(-1);
          shader_resp->set_experiment_count(-1);
          ShaderAssembly s;
          set_shader_assembly(s, shader_resp->mutable_vertex());
          set_shader_assembly(s, shader_resp->mutable_fragment());
          set_shader_assembly(s, shader_resp->mutable_tess_control());
          set_shader_assembly(s, shader_resp->mutable_tess_eval());
          set_shader_assembly(s, shader_resp->mutable_geom());
          set_shader_assembly(s, shader_resp->mutable_comp());
          writeResponse(m_socket, proto_response, &m_buf);
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
                                  shader.tess_control(),
                                  shader.tess_eval(),
                                  shader.geom(),
                                  shader.comp(),
                                  this);
          break;
        }
      case ApiTrace::API_REQUEST:
        {
          assert(request.has_api());
          auto api = request.api();
          RenderSelection selection;
          makeRenderSelection(api.selection(), &selection);
          m_frame->retraceApi(selection,
                              this);
          // send empty message to signal the last response
          RetraceResponse proto_response;
          auto shader_resp = proto_response.mutable_api();
          shader_resp->set_render_id(-1);
          shader_resp->set_selection_count(-1);
          writeResponse(m_socket, proto_response, &m_buf);
          break;
        }
      case ApiTrace::BATCH_REQUEST:
        {
          assert(request.has_batch());
          auto batch = request.batch();
          RenderSelection selection;
          makeRenderSelection(batch.selection(), &selection);
          m_frame->retraceBatch(selection,
                                ExperimentId(batch.experiment_count()),
                                this);
          // send empty message to signal the last response
          RetraceResponse proto_response;
          auto batch_resp = proto_response.mutable_batch();
          batch_resp->set_render_id(-1);
          batch_resp->set_selection_count(-1);
          batch_resp->set_experiment_count(-1);
          batch_resp->set_batch("");
          writeResponse(m_socket, proto_response, &m_buf);
          break;
        }
      case ApiTrace::DISABLE_REQUEST:
        {
          assert(request.has_disable());
          auto disable = request.disable();
          RenderSelection selection;
          makeRenderSelection(disable.selection(), &selection);
          m_frame->disableDraw(selection, disable.disable());
          break;
        }
      case ApiTrace::SIMPLE_SHADER_REQUEST:
        {
          assert(request.has_simpleshader());
          auto simple = request.simpleshader();
          RenderSelection selection;
          makeRenderSelection(simple.selection(), &selection);
          m_frame->simpleShader(selection, simple.simple_shader());
          break;
        }
      case ApiTrace::UNIFORM_REQUEST:
        {
          assert(request.has_uniform());
          auto uniform = request.uniform();
          RenderSelection selection;
          makeRenderSelection(uniform.selection(), &selection);
          m_frame->retraceUniform(selection,
                                  ExperimentId(uniform.experiment_count()),
                                  this);
          // send empty message to signal the last response
          RetraceResponse proto_response;
          auto uniform_resp = proto_response.mutable_uniform();
          uniform_resp->set_render_id(-1);
          uniform_resp->set_selection_count(-1);
          uniform_resp->set_experiment_count(-1);
          uniform_resp->set_name("");
          uniform_resp->set_uniform_type(ApiTrace::FLOAT_UNIFORM);
          uniform_resp->set_uniform_dimension(ApiTrace::D_1x1);
          uniform_resp->set_data("");
          writeResponse(m_socket, proto_response, &m_buf);
          break;
        }
      case ApiTrace::SET_UNIFORM_REQUEST:
        {
          assert(request.has_set_uniform());
          auto set_uniform = request.set_uniform();
          RenderSelection selection;
          makeRenderSelection(set_uniform.selection(), &selection);
          auto data_begin = set_uniform.data().begin();
          auto data_end = set_uniform.data().end();
          const std::vector<unsigned char> uniform_data(data_begin,
                                                        data_end);
          m_frame->setUniform(selection,
                              set_uniform.name(),
                              set_uniform.index(),
                              set_uniform.data());
          break;
        }
    }
  }
}

void
FrameRetraceSkeleton::onFileOpening(bool needUpload,
                                    bool finished,
                                    uint32_t frame_count) {
  RetraceResponse proto_response;
  auto status = proto_response.mutable_filestatus();
  status->set_needs_upload(needUpload);
  status->set_finished(finished);
  status->set_frame_count(frame_count);
  writeResponse(m_socket, proto_response, &m_buf);
}

void
FrameRetraceSkeleton::onShaderAssembly(
    RenderId renderId,
    SelectionId selectionCount,
    ExperimentId experimentCount,
    const ShaderAssembly &vertex,
    const ShaderAssembly &fragment,
    const ShaderAssembly &tess_control,
    const ShaderAssembly &tess_eval,
    const ShaderAssembly &geom,
    const ShaderAssembly &comp)  {
  RetraceResponse proto_response;
  auto shader = proto_response.mutable_shaderassembly();
  shader->set_render_id(renderId());
  shader->set_selection_id(selectionCount());
  shader->set_experiment_count(experimentCount.count());
  auto vertex_response = shader->mutable_vertex();
  set_shader_assembly(vertex, vertex_response);
  auto fragment_response = shader->mutable_fragment();
  set_shader_assembly(fragment, fragment_response);
  auto tess_control_response = shader->mutable_tess_control();
  set_shader_assembly(tess_control, tess_control_response);
  auto tess_eval_response = shader->mutable_tess_eval();
  set_shader_assembly(tess_eval, tess_eval_response);
  auto geom_response = shader->mutable_geom();
  set_shader_assembly(geom, geom_response);
  auto comp_response = shader->mutable_comp();
  set_shader_assembly(comp, comp_response);
  writeResponse(m_socket, proto_response, &m_buf);
}

typedef std::vector<unsigned char> Buffer;

void
FrameRetraceSkeleton::onRenderTarget(SelectionId selectionCount,
                                     ExperimentId experimentCount,
                                     const uvec & pngImageData) {
  RetraceResponse proto_response;
  auto rt_response = proto_response.mutable_rendertarget();
  rt_response->set_selection_count(selectionCount());
  rt_response->set_experiment_count(experimentCount());
  std::string *image = rt_response->mutable_image();
  image->assign((const char *)pngImageData.data(), pngImageData.size());
  writeResponse(m_socket, proto_response, &m_buf);
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
                                   const std::vector<std::string> &names,
                                   const std::vector<std::string> &desc) {
  RetraceResponse proto_response;
  auto metrics_response = proto_response.mutable_metricslist();
  for (auto i : ids)
    metrics_response->add_metric_ids(i());
  for (auto i : names)
    metrics_response->add_metric_names(i);
  for (auto i : desc)
    metrics_response->add_metric_descriptions(i);
  writeResponse(m_socket, proto_response, &m_buf);
}

void
FrameRetraceSkeleton::onMetrics(const MetricSeries &metricData,
                                ExperimentId experimentCount,
                                SelectionId selectionCount) {
  auto metrics_response = m_multi_metrics_response->mutable_metricsdata();
  metrics_response->set_experiment_count(experimentCount());
  metrics_response->set_selection_count(selectionCount());
  ApiTrace::MetricSeries *s = metrics_response->add_metric_data();
  s->set_metric_id(metricData.metric());
  for (auto d : metricData.data) {
    s->add_data(d);
  }
}

void
FrameRetraceSkeleton::onApi(SelectionId selectionCount,
                            RenderId renderId,
                            const std::vector<std::string> &api_calls) {
  // TODO(majanes) encode selectionCount in message
  RetraceResponse proto_response;
  auto api = proto_response.mutable_api();
  api->set_selection_count(selectionCount());
  api->set_render_id(renderId());
  for (auto a : api_calls) {
    api->add_apis(a);
  }
  writeResponse(m_socket, proto_response, &m_buf);
}

void
FrameRetraceSkeleton::onError(ErrorSeverity s, const std::string &message) {
  RetraceResponse proto_response;
  auto error = proto_response.mutable_error();
  error->set_severity(s);
  error->set_message(message);
  writeResponse(m_socket, proto_response, &m_buf);
  if (s == RETRACE_FATAL)
    m_fatal_error = true;
}

void
FrameRetraceSkeleton::onBatch(SelectionId selectionCount,
                              ExperimentId experimentCount,
                              RenderId renderId,
                              const std::string &batch) {
  RetraceResponse proto_response;
  auto response = proto_response.mutable_batch();
  response->set_render_id(renderId());
  response->set_selection_count(selectionCount());
  response->set_experiment_count(experimentCount.count());
  response->set_batch(batch);
  writeResponse(m_socket, proto_response, &m_buf);
}

void
FrameRetraceSkeleton::onUniform(SelectionId selectionCount,
                                ExperimentId experimentCount,
                                RenderId renderId,
                                const std::string &name,
                                UniformType type,
                                UniformDimension dimension,
                                const std::vector<unsigned char> &data) {
  RetraceResponse proto_response;
  auto response = proto_response.mutable_uniform();
  response->set_render_id(renderId());
  response->set_selection_count(selectionCount());
  response->set_experiment_count(experimentCount.count());
  response->set_name(name);
  switch (type) {
    case kFloatUniform:
      response->set_uniform_type(ApiTrace::FLOAT_UNIFORM);
      break;
    case kIntUniform:
      response->set_uniform_type(ApiTrace::INT_UNIFORM);
      break;
    case kUIntUniform:
      response->set_uniform_type(ApiTrace::UINT_UNIFORM);
      break;
    case kBoolUniform:
      response->set_uniform_type(ApiTrace::BOOL_UNIFORM);
      break;
  }
  switch (dimension) {
    case k1x1:
      response->set_uniform_dimension(ApiTrace::D_1x1);
      break;
    case k2x1:
      response->set_uniform_dimension(ApiTrace::D_2x1);
      break;
    case k3x1:
      response->set_uniform_dimension(ApiTrace::D_3x1);
      break;
    case k4x1:
      response->set_uniform_dimension(ApiTrace::D_4x1);
      break;
    case k2x2:
      response->set_uniform_dimension(ApiTrace::D_2x2);
      break;
    case k3x2:
      response->set_uniform_dimension(ApiTrace::D_3x2);
      break;
    case k4x2:
      response->set_uniform_dimension(ApiTrace::D_4x2);
      break;
    case k2x3:
      response->set_uniform_dimension(ApiTrace::D_2x3);
      break;
    case k3x3:
      response->set_uniform_dimension(ApiTrace::D_3x3);
      break;
    case k4x3:
      response->set_uniform_dimension(ApiTrace::D_4x3);
      break;
    case k2x4:
      response->set_uniform_dimension(ApiTrace::D_2x4);
      break;
    case k3x4:
      response->set_uniform_dimension(ApiTrace::D_3x4);
      break;
    case k4x4:
      response->set_uniform_dimension(ApiTrace::D_4x4);
      break;
  }
  response->set_data(data.data(), data.size());
  writeResponse(m_socket, proto_response, &m_buf);
}
