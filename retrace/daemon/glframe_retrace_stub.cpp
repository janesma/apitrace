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

using glretrace::FrameRetraceStub;
using glretrace::MetricId;
using glretrace::Thread;
using glretrace::RenderId;
using glretrace::RenderTargetType;
using glretrace::RenderOptions;
using glretrace::OnFrameRetrace;
using glretrace::Semaphore;
using glretrace::Socket;
using glretrace::ScopedLock;
using ApiTrace::RetraceRequest;
using ApiTrace::RetraceResponse;
using google::protobuf::io::ArrayInputStream;
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::CodedOutputStream;
using google::protobuf::io::ArrayOutputStream;

namespace {


class RetraceSocket {
 public:
  explicit RetraceSocket(int port) : m_sock("localhost", port) {}
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
    m_sock.Read(&read_size);
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
class RetraceRenderTargetRequest : public IRetraceRequest {
 public:
  RetraceRenderTargetRequest(RenderId renderId,
                             int render_target_number,
                             RenderTargetType type,
                             RenderOptions options,
                             OnFrameRetrace *callback)
      : m_callback(callback) {
    // make the proto msg
    auto rtRequest = m_proto_msg.mutable_rendertarget();
    rtRequest->set_renderid(renderId());
    rtRequest->set_type((ApiTrace::RenderTargetType)type);
    rtRequest->set_options(options);
    m_proto_msg.set_requesttype(ApiTrace::RENDER_TARGET_REQUEST);
  }

  virtual void retrace(RetraceSocket *s) {
    RetraceResponse response;
    s->retrace(m_proto_msg, &response);
    assert(response.has_rendertarget());
    auto rt = response.rendertarget();
    assert(rt.has_image());
    auto imageStr = rt.image();
    std::vector<unsigned char> image(imageStr.size());
    memcpy(image.data(), imageStr.c_str(), imageStr.size());
    m_callback->onRenderTarget(RenderId(m_proto_msg.rendertarget().renderid()),
                               glretrace::NORMAL_RENDER,
                               image);
  }

 private:
  RetraceRequest m_proto_msg;
  OnFrameRetrace *m_callback;
};
class RetraceShaderAssemblyRequest : public IRetraceRequest {
 public:
  RetraceShaderAssemblyRequest(RenderId id, OnFrameRetrace *cb)
      : m_callback(cb) {
    auto shaderRequest = m_proto_msg.mutable_shaderassembly();
    shaderRequest->set_renderid(id());
    m_proto_msg.set_requesttype(ApiTrace::SHADER_ASSEMBLY_REQUEST);
  }
  virtual void retrace(RetraceSocket *s) {
    RetraceResponse response;
    s->retrace(m_proto_msg, &response);
    assert(response.has_shaderassembly());
    auto shader = response.shaderassembly();
    m_callback->onShaderAssembly(
        RenderId(m_proto_msg.rendertarget().renderid()),
        shader.vertex_shader(),
        shader.vertex_ir(),
        shader.vertex_vec4(),
        shader.fragment_shader(),
        shader.fragment_ir(),
        shader.fragment_simd8(),
        shader.fragment_simd16());
  }

 private:
  RetraceRequest m_proto_msg;
  OnFrameRetrace *m_callback;
};

class RetraceOpenFileRequest: public IRetraceRequest {
 public:
  RetraceOpenFileRequest(const std::string &fn, uint32_t frame,
                         OnFrameRetrace *cb)
      : m_callback(cb) {
    m_proto_msg.set_requesttype(ApiTrace::OPEN_FILE_REQUEST);
    auto file_open = m_proto_msg.mutable_fileopen();
    file_open->set_filename(fn);
    file_open->set_framenumber(frame);
  }
  virtual void retrace(RetraceSocket *s) {
    s->request(m_proto_msg);
    while (true) {
      RetraceResponse response;
      s->response(&response);
      if (response.has_filestatus()) {
        auto status = response.filestatus();
        if (m_callback)
          m_callback->onFileOpening(status.finished(),
                                    status.percent_complete());
        if (status.finished())
          break;
      } else {
        assert(response.has_metricslist());
        std::vector<MetricId> ids;
        std::vector<std::string> names;
        auto metrics_list = response.metricslist();
        ids.reserve(metrics_list.metric_ids_size());
        for (int i = 0; i < metrics_list.metric_ids_size(); ++i )
          ids.push_back(MetricId(metrics_list.metric_ids(i)));
        names.reserve(metrics_list.metric_names_size());
        for (int i = 0; i < metrics_list.metric_names_size(); ++i )
          names.push_back(metrics_list.metric_names(i));
        m_callback->onMetricList(ids, names);
      }
    }
  }

 private:
  RetraceRequest m_proto_msg;
  OnFrameRetrace *m_callback;
};

class NullRequest : public IRetraceRequest {
 public:
  // to pump the thread, and force it to stop
  virtual void retrace(RetraceSocket *sock) {}
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

class ThreadedRetrace : public Thread {
 public:
  explicit ThreadedRetrace(int port) : Thread("retrace_stub"),
                                       m_running(true),
                                       m_sock(port) {}
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

static ThreadedRetrace *thread = NULL;
}  // namespace

void
FrameRetraceStub::Init(int port) {
  assert(thread == NULL);
  thread = new ThreadedRetrace(port);
  thread->Start();
}

void
FrameRetraceStub::Shutdown() {
  assert(thread != NULL);
  thread->stop();
  delete thread;
}

void
FrameRetraceStub::openFile(const std::string &filename,
                           uint32_t frameNumber,
                           OnFrameRetrace *callback) {
  thread->push(new RetraceOpenFileRequest(filename, frameNumber, callback));
}



void
FrameRetraceStub::retraceRenderTarget(RenderId renderId,
                                      int render_target_number,
                                      RenderTargetType type,
                                      RenderOptions options,
                                      OnFrameRetrace *callback) const {
  thread->push(new RetraceRenderTargetRequest(renderId, render_target_number,
                                              type, options, callback));
}
void
FrameRetraceStub::retraceShaderAssembly(RenderId renderId,
                                        OnFrameRetrace *callback) {
  thread->push(new RetraceShaderAssemblyRequest(renderId, callback));
}
