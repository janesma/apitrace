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

#include "glframe_retrace_stub.hpp"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>

#include <deque>
#include <string>
#include <vector>

#include "glframe_os.hpp"
#include "glframe_retrace.hpp"
#include "glframe_socket.hpp"
#include "glframe_thread.hpp"
#include "playback.pb.h" // NOLINT

using ApiTrace::RetraceRequest;
using ApiTrace::RetraceResponse;
using glretrace::RenderSelection;
using glretrace::ExperimentId;
using glretrace::SelectionId;
using glretrace::FrameRetraceStub;
using glretrace::MetricId;
using glretrace::MetricSeries;
using glretrace::OnFrameRetrace;
using glretrace::RenderId;
using glretrace::RenderOptions;
using glretrace::RenderTargetType;
using glretrace::ScopedLock;
using glretrace::Semaphore;
using glretrace::ShaderAssembly;
using glretrace::Socket;
using glretrace::Thread;
using google::protobuf::io::ArrayInputStream;
using google::protobuf::io::ArrayOutputStream;
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::CodedOutputStream;
namespace {


class RetraceSocket {
 public:
  explicit RetraceSocket(const char *host, int port) : m_sock(host, port) {}
  void request(const RetraceRequest &req) {
    // write message
    const uint32_t write_size = req.ByteSize();
    // std::cout << "RetraceSocket: writing size: " << write_size << "\n";
    if (!m_sock.Write(write_size)) {
      std::cout << "no write: len\n";
      return;
    }
    m_buf.clear();
    m_buf.resize(write_size);
    // std::cout << "array\n";
    ArrayOutputStream array_out(m_buf.data(), write_size);
    // std::cout << "coded\n";
    CodedOutputStream coded_out(&array_out);
    // std::cout << "serial\n";
    req.SerializeToCodedStream(&coded_out);
    // std::cout << "RetraceSocket: writing request\n";
    if (!m_sock.WriteVec(m_buf)) {
      std::cout << "no write: buf\n";
      return;
    }
  }
  void response(RetraceResponse *resp) {
    // read response
    uint32_t read_size;
    if (!m_sock.Read(&read_size)) {
      std::cout << "no read: len\n";
      return;
    }
    // std::cout << "RetraceSocket: read size: " << read_size << "\n";
    m_buf.clear();
    m_buf.resize(read_size);
    m_sock.ReadVec(&m_buf);
    ArrayInputStream array_in(m_buf.data(), read_size);
    CodedInputStream coded_in(&array_in);
    CodedInputStream::Limit msg_limit = coded_in.PushLimit(read_size);
    resp->ParseFromCodedStream(&coded_in);
    coded_in.PopLimit(msg_limit);
  }

  void retrace(const RetraceRequest &req, RetraceResponse *resp) {
    request(req);
    response(resp);
  }

  void write(const std::vector<unsigned char> &buf) {
    const uint32_t write_size = buf.size();
    // std::cout << "RetraceSocket: writing size: " << write_size << "\n";
    if (!m_sock.Write(write_size)) {
      std::cout << "no write: len\n";
      return;
    }
    m_sock.WriteVec(buf);
  }

 private:
  Socket m_sock;
  std::vector<unsigned char> m_buf;
};
// command objects for enqueueing the asynchronous request
class IRetraceRequest {
 public:
  virtual void retrace(RetraceSocket *s) = 0;
  virtual ~IRetraceRequest() {}
};

void makeRenderSelection(const RenderSelection &selection,
                         ApiTrace::RenderSelection *proto_sel) {
    proto_sel->set_selection_count(selection.id());
    for (auto sequence : selection.series) {
      auto series = proto_sel->add_render_series();
      series->set_begin(sequence.begin());
      series->set_end(sequence.end());
    }
}

class RetraceRenderTargetRequest : public IRetraceRequest {
 public:
  RetraceRenderTargetRequest(SelectionId *current_selection,
                             ExperimentId *current_experiment,
                             std::mutex *protect,
                             ExperimentId experimentCount,
                             const RenderSelection &selection,
                             RenderTargetType type,
                             RenderOptions options,
                             OnFrameRetrace *callback)
      : m_sel_count(current_selection),
        m_exp_count(current_experiment),
        m_protect(protect),
        m_callback(callback) {
    // make the proto msg
    auto rtRequest = m_proto_msg.mutable_rendertarget();
    rtRequest->set_experiment_count(experimentCount());
    makeRenderSelection(selection, rtRequest->mutable_render_selection());
    rtRequest->set_type((ApiTrace::RenderTargetType)type);
    rtRequest->set_options(options);
    m_proto_msg.set_requesttype(ApiTrace::RENDER_TARGET_REQUEST);
  }

  virtual void retrace(RetraceSocket *s) {
    {
      std::lock_guard<std::mutex> l(*m_protect);
      const auto &sel = m_proto_msg.rendertarget().render_selection();
      const SelectionId id(sel.selection_count());
      if (*m_sel_count != id)
        // more recent selection was made while this was enqueued
        return;

      if (*m_exp_count !=
          ExperimentId(m_proto_msg.rendertarget().experiment_count()))
        // more experiment was enabled while this was enqueued
        return;
    }
    RetraceResponse response;
    s->retrace(m_proto_msg, &response);
    if (response.has_error()) {
      m_callback->onError(response.error().message());
      return;
    }
    assert(response.has_rendertarget());
    auto rt = response.rendertarget();
    assert(rt.has_image());
    auto imageStr = rt.image();
    std::vector<unsigned char> image(imageStr.size());
    memcpy(image.data(), imageStr.c_str(), imageStr.size());
    m_callback->onRenderTarget(*m_sel_count,
                               *m_exp_count,
                               image);
  }

 private:
  const SelectionId * const m_sel_count;
  const ExperimentId * const m_exp_count;
  std::mutex *m_protect;
  RetraceRequest m_proto_msg;
  OnFrameRetrace *m_callback;
};

void set_shader_assembly(const ApiTrace::ShaderAssembly &response,
                         ShaderAssembly *assembly) {
  assembly->shader = response.shader();
  assembly->ir = response.ir();
  assembly->ssa = response.nir_ssa();
  assembly->nir = response.nir_final();
  assembly->simd8 = response.simd8();
  assembly->simd16 = response.simd16();
  assembly->simd32 = response.simd32();
  assembly->beforeUnification = response.before_unification();
  assembly->afterUnification = response.after_unification();
  assembly->beforeOptimization = response.before_optimization();
  assembly->constCoalescing = response.const_coalescing();
  assembly->genIrLowering = response.gen_ir_lowering();
  assembly->layout = response.layout();
  assembly->optimized = response.optimized();
  assembly->pushAnalysis = response.push_analysis();
  assembly->codeHoisting = response.code_hoisting();
  assembly->codeSinking = response.code_sinking();
}

class RetraceShaderAssemblyRequest : public IRetraceRequest {
 public:
  RetraceShaderAssemblyRequest(SelectionId *current_selection,
                               std::mutex *protect,
                               const RenderSelection &selection,
                               OnFrameRetrace *cb)
      : m_sel_count(current_selection),
        m_protect(protect),
        m_callback(cb) {
    auto shaderRequest = m_proto_msg.mutable_shaderassembly();
    makeRenderSelection(selection, shaderRequest->mutable_render_selection());
    m_proto_msg.set_requesttype(ApiTrace::SHADER_ASSEMBLY_REQUEST);
  }
  virtual void retrace(RetraceSocket *s) {
    {
      std::lock_guard<std::mutex> l(*m_protect);
      const auto &s = m_proto_msg.shaderassembly().render_selection();
      if (*m_sel_count != SelectionId(s.selection_count()))
        // more recent selection was made while this was enqueued
        return;
    }
    RetraceResponse response;
    // sends single request, read multiple responses
    s->request(m_proto_msg);
    while (true) {
      response.Clear();
      s->response(&response);
      assert(response.has_shaderassembly());
      auto shader = response.shaderassembly();
      if (shader.render_id() == -1)
        // all responses sent
        break;
      const auto &selection = m_proto_msg.shaderassembly().render_selection();
      {
        std::lock_guard<std::mutex> l(*m_protect);
        if (*m_sel_count != SelectionId(selection.selection_count()))
          // more recent selection was made while retrace was being
          // executed.
          continue;
      }
      std::vector<ShaderAssembly> assemblies(6);
      set_shader_assembly(shader.vertex(), &(assemblies[0]));
      set_shader_assembly(shader.fragment(), &(assemblies[1]));
      set_shader_assembly(shader.tess_control(), &(assemblies[2]));
      set_shader_assembly(shader.tess_eval(), &(assemblies[3]));
      set_shader_assembly(shader.geom(), &(assemblies[4]));
      set_shader_assembly(shader.comp(), &(assemblies[5]));
      m_callback->onShaderAssembly(
          RenderId(shader.render_id()),
          SelectionId(selection.selection_count()),
          assemblies[0], assemblies[1], assemblies[2], assemblies[3],
          assemblies[4], assemblies[5]);
    }
  }

 private:
  SelectionId *m_sel_count;
  std::mutex *m_protect;
  RetraceRequest m_proto_msg;
  OnFrameRetrace *m_callback;
};

class RetraceOpenFileRequest: public IRetraceRequest {
 public:
  RetraceOpenFileRequest(const std::string &fn,
                         const std::vector<unsigned char> &md5,
                         uint64_t fileSize,
                         uint32_t frame,
                         OnFrameRetrace *cb)
      : m_callback(cb) {
    m_proto_msg.set_requesttype(ApiTrace::OPEN_FILE_REQUEST);
    auto file_open = m_proto_msg.mutable_fileopen();
    file_open->set_filename(fn);
    file_open->set_md5sum(md5.data(), md5.size());
    file_open->set_filesize(fileSize);
    file_open->set_framenumber(frame);
  }
  virtual void retrace(RetraceSocket *s) {
    s->request(m_proto_msg);
    while (true) {
      RetraceResponse response;
      s->response(&response);
      if (response.has_filestatus()) {
        auto status = response.filestatus();
        if (status.needs_upload()) {
          m_callback->onFileOpening(true, false, 0);
          // send data
          auto file_open = m_proto_msg.fileopen();
          FILE * fh = fopen(file_open.filename().c_str(), "rb");
          assert(fh);
          std::vector<unsigned char> buf(1024 * 1024);
          while (true) {
            const size_t bytes = fread(buf.data(), 1, 1024 * 1024, fh);
            buf.resize(bytes);
            s->write(buf);
            if (feof(fh))
              break;
            assert(!ferror(fh));
          }
          continue;
        }
        if (m_callback)
          m_callback->onFileOpening(status.needs_upload(),
                                    status.finished(),
                                    status.percent_complete());
        if (status.finished())
          break;
      } else {
        assert(response.has_metricslist());
        std::vector<MetricId> ids;
        std::vector<std::string> names;
        std::vector<std::string> descriptions;
        auto metrics_list = response.metricslist();
        ids.reserve(metrics_list.metric_ids_size());
        for (int i = 0; i < metrics_list.metric_ids_size(); ++i )
          ids.push_back(MetricId(metrics_list.metric_ids(i)));
        names.reserve(metrics_list.metric_names_size());
        for (int i = 0; i < metrics_list.metric_names_size(); ++i )
          names.push_back(metrics_list.metric_names(i));
        descriptions.reserve(metrics_list.metric_names_size());
        for (int i = 0; i < metrics_list.metric_descriptions_size(); ++i )
          descriptions.push_back(metrics_list.metric_descriptions(i));

        m_callback->onMetricList(ids, names, descriptions);
      }
    }
  }

 private:
  RetraceRequest m_proto_msg;
  OnFrameRetrace *m_callback;
};

class RetraceMetricsRequest : public IRetraceRequest {
 public:
  RetraceMetricsRequest(const std::vector<MetricId> &ids,
                              ExperimentId experimentCount,
                              OnFrameRetrace *cb)
      : m_callback(cb) {
    auto metricsRequest = m_proto_msg.mutable_metrics();
    for (MetricId i : ids)
      metricsRequest->add_metric_ids(i());
    metricsRequest->set_experiment_count(experimentCount());
    m_proto_msg.set_requesttype(ApiTrace::METRICS_REQUEST);
  }
  virtual void retrace(RetraceSocket *s) {
    RetraceResponse response;
    s->retrace(m_proto_msg, &response);
    assert(response.has_metricsdata());
    auto metrics_response = response.metricsdata();

    const ExperimentId eid(metrics_response.experiment_count());
    for (auto &metric_data : metrics_response.metric_data()) {
      MetricSeries met;
      met.metric = MetricId(metric_data.metric_id());
      auto &data_float_vec = metric_data.data();
      met.data.reserve(data_float_vec.size());
      for (auto d : data_float_vec)
        met.data.push_back(d);

      m_callback->onMetrics(met, eid, SelectionId(0));
    }
  }

 private:
  RetraceRequest m_proto_msg;
  OnFrameRetrace *m_callback;
};

class RetraceAllMetricsRequest : public IRetraceRequest {
 public:
  RetraceAllMetricsRequest(SelectionId *current_selection,
                           std::mutex *protect,
                           const RenderSelection &selection,
                           ExperimentId experimentCount,
                           OnFrameRetrace *cb)
      : m_sel_count(current_selection),
        m_protect(protect),
        m_callback(cb) {
    auto metricsRequest = m_proto_msg.mutable_allmetrics();
    metricsRequest->set_experiment_count(experimentCount());
    auto selectionRequest = metricsRequest->mutable_selection();
    makeRenderSelection(selection, selectionRequest);
    m_proto_msg.set_requesttype(ApiTrace::ALL_METRICS_REQUEST);
  }
  virtual void retrace(RetraceSocket *s) {
    {
      const int query_sel_count =
          m_proto_msg.allmetrics().selection().selection_count();
      const SelectionId query_id(query_sel_count);
      std::lock_guard<std::mutex> l(*m_protect);
      if ((*m_sel_count != query_id) &&
          (query_id != SelectionId(0)))
        // more recent selection was made while this was enqueued
        return;
    }

    RetraceResponse response;
    s->retrace(m_proto_msg, &response);
    if (response.has_error()) {
      m_callback->onError(response.error().message());
      return;
    }
    assert(response.has_metricsdata());
    auto metrics_response = response.metricsdata();

    const ExperimentId eid(metrics_response.experiment_count());
    const SelectionId sid(metrics_response.selection_count());
    for (auto &metric_data : metrics_response.metric_data()) {
      MetricSeries met;
      met.metric = MetricId(metric_data.metric_id());
      auto &data_float_vec = metric_data.data();
      met.data.reserve(data_float_vec.size());
      for (auto d : data_float_vec)
        met.data.push_back(d);

      m_callback->onMetrics(met, eid, sid);
    }
  }

 private:
  const SelectionId * const m_sel_count;
  std::mutex *m_protect;
  RetraceRequest m_proto_msg;
  OnFrameRetrace *m_callback;
};

class ReplaceShadersRequest : public IRetraceRequest {
 public:
  ReplaceShadersRequest(RenderId renderId,
                        ExperimentId experimentCount,
                        const std::string &vs,
                        const std::string &fs,
                        const std::string &tessControl,
                        const std::string &tessEval,
                        const std::string &geom,
                        const std::string &comp,
                        OnFrameRetrace *cb)
      : m_callback(cb) {
    auto shaderRequest = m_proto_msg.mutable_shaders();
    shaderRequest->set_render_id(renderId());
    shaderRequest->set_experiment_count(experimentCount());
    shaderRequest->set_vs(vs);
    shaderRequest->set_fs(fs);
    shaderRequest->set_tess_control(tessControl);
    shaderRequest->set_tess_eval(tessEval);
    shaderRequest->set_geom(geom);
    shaderRequest->set_comp(comp);
    m_proto_msg.set_requesttype(ApiTrace::REPLACE_SHADERS_REQUEST);
  }
  virtual void retrace(RetraceSocket *s) {
    RetraceResponse response;
    s->retrace(m_proto_msg, &response);
    assert(response.has_shadersdata());
    auto shaders_response = response.shadersdata();

    const ExperimentId eid(shaders_response.experiment_count());
    const RenderId rid(shaders_response.render_id());
    m_callback->onShaderCompile(rid, eid, shaders_response.status(),
                                shaders_response.message());
  }

 private:
  RetraceRequest m_proto_msg;
  OnFrameRetrace *m_callback;
};

class ApiRequest : public IRetraceRequest {
 public:
  ApiRequest(SelectionId *current_selection,
             std::mutex *protect,
             const RenderSelection &selection,
             OnFrameRetrace *cb)
      : m_sel_count(current_selection),
        m_protect(protect),
        m_callback(cb) {
    auto apiRequest = m_proto_msg.mutable_api();
    auto selectionRequest = apiRequest->mutable_selection();
    makeRenderSelection(selection, selectionRequest);
    m_proto_msg.set_requesttype(ApiTrace::API_REQUEST);
  }
  virtual void retrace(RetraceSocket *s) {
    {
      std::lock_guard<std::mutex> l(*m_protect);
      const auto &sel = m_proto_msg.api().selection();
      const SelectionId id(sel.selection_count());
      if (*m_sel_count != id)
        // more recent selection was made while this was enqueued
        return;
    }
    RetraceResponse response;
    // sends single request, read multiple responses
    s->request(m_proto_msg);
    while (true) {
      response.Clear();
      s->response(&response);
      assert(response.has_api());
      const auto &api_response = response.api();
      if (api_response.render_id() == -1)
        // all responses sent
        break;

      const auto selection = api_response.selection_count();
      {
        std::lock_guard<std::mutex> l(*m_protect);
        if (*m_sel_count != SelectionId(selection))
          // more recent selection was made while retrace was being
          // executed.
          continue;
      }

      const RenderId rid(api_response.render_id());
      std::vector<std::string> apis;
      auto &api_str_vec = api_response.apis();
      apis.reserve(api_str_vec.size());
      for (auto a : api_str_vec)
        apis.push_back(a);

      m_callback->onApi(SelectionId(selection),
                        rid, apis);
    }
  }

 private:
  const SelectionId * const m_sel_count;
  std::mutex *m_protect;
  RetraceRequest m_proto_msg;
  OnFrameRetrace *m_callback;
};

class NullRequest : public IRetraceRequest {
 public:
  // to pump the thread, and force it to stop
  virtual void retrace(RetraceSocket *sock) {}
};

class FlushRequest : public IRetraceRequest {
 public:
  explicit FlushRequest(Semaphore *sem) : m_sem(sem) {}
  // to block until the queue executes all outstanding requests
  virtual void retrace(RetraceSocket *sock) {
    m_sem->post();
  }
 private:
  Semaphore *m_sem;
};

class BufferQueue {
 public:
  void push(IRetraceRequest *r) {
    m_mut.lock();
    m_requests.push_back(r);
    m_mut.unlock();
    m_sem.post();
  }

  IRetraceRequest *pop() {
    m_sem.wait();
    m_mut.lock();
    IRetraceRequest *r = m_requests.front();
    m_requests.pop_front();
    m_mut.unlock();
    return r;
  }

 private:
  std::deque<IRetraceRequest *> m_requests;
  Semaphore m_sem;
  std::mutex m_mut;
};

}  // namespace

namespace glretrace {

class ThreadedRetrace : public Thread {
 public:
  explicit ThreadedRetrace(const char *host,
                           int port) : Thread("retrace_stub"),
                                       m_running(true),
                                       m_sock(host, port) {}
  void push(IRetraceRequest *r) { m_queue.push(r); }
  void stop() {
    m_running = false;
    m_queue.push(new NullRequest());
    Join();
  }
  virtual void Run() {
    while (m_running) {
      IRetraceRequest *r = m_queue.pop();
      r->retrace(&m_sock);
      delete r;
    }
  }
 private:
  BufferQueue m_queue;
  bool m_running;
  RetraceSocket m_sock;
};

}  // namespace glretrace

using glretrace::ThreadedRetrace;

void
FrameRetraceStub::Init(const char *host, int port) {
  assert(m_thread == NULL);
  m_thread = new ThreadedRetrace(host, port);
  m_thread->Start();
}

void
FrameRetraceStub::Shutdown() {
  assert(m_thread != NULL);
  m_thread->stop();
  delete m_thread;
}

void
FrameRetraceStub::openFile(const std::string &filename,
                           const std::vector<unsigned char> &md5,
                           uint64_t fileSize,
                           uint32_t frameNumber,
                           OnFrameRetrace *callback) {
  m_thread->push(new RetraceOpenFileRequest(filename, md5, fileSize,
                                            frameNumber, callback));
}

void
FrameRetraceStub::retraceRenderTarget(ExperimentId experimentCount,
                                      const RenderSelection &selection,
                                      RenderTargetType type,
                                      RenderOptions options,
                                      OnFrameRetrace *callback) const {
  {
    std::lock_guard<std::mutex> l(m_mutex);
    m_current_rt_selection = selection.id;
    m_current_rt_experiment = experimentCount;
  }
  m_thread->push(new RetraceRenderTargetRequest(&m_current_rt_selection,
                                                &m_current_rt_experiment,
                                                &m_mutex,
                                                experimentCount,
                                                selection,
                                                type, options, callback));
}

void
FrameRetraceStub::retraceShaderAssembly(const RenderSelection &selection,
                                        OnFrameRetrace *callback) {
  {
    std::lock_guard<std::mutex> l(m_mutex);
    m_current_render_selection = selection.id;
  }
  m_thread->push(new RetraceShaderAssemblyRequest(&m_current_render_selection,
                                                  &m_mutex,
                                                  selection, callback));
}

void
FrameRetraceStub::retraceMetrics(const std::vector<MetricId> &ids,
                                 ExperimentId experimentCount,
                                 OnFrameRetrace *callback) const {
  m_thread->push(new RetraceMetricsRequest(ids, experimentCount, callback));
}

void
FrameRetraceStub::retraceAllMetrics(const RenderSelection &selection,
                                    ExperimentId experimentCount,
                                    OnFrameRetrace *callback) const {
  {
    std::lock_guard<std::mutex> l(m_mutex);
    m_current_met_selection = selection.id;
  }
  m_thread->push(new RetraceAllMetricsRequest(&m_current_met_selection,
                                              &m_mutex,
                                              selection,
                                              experimentCount,
                                              callback));
}

void
FrameRetraceStub::replaceShaders(RenderId renderId,
                                 ExperimentId experimentCount,
                                 const std::string &vs,
                                 const std::string &fs,
                                 const std::string &tessControl,
                                 const std::string &tessEval,
                                 const std::string &geom,
                                 const std::string &comp,
                                 OnFrameRetrace *callback) {
  m_thread->push(new ReplaceShadersRequest(renderId, experimentCount,
                                           vs, fs, tessControl, tessEval,
                                           geom, comp, callback));
}

void
FrameRetraceStub::retraceApi(const RenderSelection &selection,
                             OnFrameRetrace *callback) {
  {
    std::lock_guard<std::mutex> l(m_mutex);
    m_current_render_selection = selection.id;
  }
  m_thread->push(new ApiRequest(&m_current_render_selection,
                                &m_mutex,
                                selection, callback));
}

void
FrameRetraceStub::Flush() {
  Semaphore sem;
  m_thread->push(new FlushRequest(&sem));
  sem.wait();
}
