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

#include <qtextstream.h>
#include "glframe_retrace_model.hpp"

// this has to be first, yuck
#include <QQuickImageProvider>   // NOLINT
#include <QtConcurrentRun>   // NOLINT

#include <sstream>  // NOLINT
#include <string> // NOLINT
#include <vector> // NOLINT

#include "glframe_qbargraph.hpp"
#include "glframe_retrace.hpp"
#include "glframe_retrace_images.hpp"
#include "glframe_logger.hpp"
#include "glframe_socket.hpp"
#include "md5.h"  // NOLINT

using glretrace::DEBUG;
using glretrace::ERR;
using glretrace::ExperimentId;
using glretrace::FrameRetraceModel;
using glretrace::FrameState;
using glretrace::MetricId;
using glretrace::MetricSeries;
using glretrace::QBarGraphRenderer;
using glretrace::QMetric;
using glretrace::QRenderBookmark;
using glretrace::QSelection;
using glretrace::RenderId;
using glretrace::RenderTargetType;
using glretrace::SelectionId;
using glretrace::ServerSocket;
using glretrace::ShaderAssembly;

FrameRetraceModel::FrameRetraceModel() : m_state(NULL),
                                         m_selection(NULL),
                                         m_selection_count(0),
                                         m_open_percent(0),
                                         m_max_metric(0),
                                         m_clear_before_render(false),
                                         m_stop_at_render(false),
                                         m_highlight_render(false) {
  m_metrics_model.push_back(new QMetric(MetricId(0), "No metric"));
  filterMetrics("");
  connect(this, &glretrace::FrameRetraceModel::updateMetricList,
          this, &glretrace::FrameRetraceModel::onUpdateMetricList);
  m_shaders.setRetrace(&m_retrace, this);
}

FrameRetraceModel::~FrameRetraceModel() {
  ScopedLock s(m_protect);
  if (m_state) {
    delete m_state;
    m_state = NULL;
  }
  m_api_calls.clear();
  m_retrace.Shutdown();
}

FrameState *frame_state_off_thread(std::string filename,
                                   int framenumber) {
  return new FrameState(filename, framenumber);
}

static QFuture<FrameState *> future;

void
exec_retracer(const char *main_exe, int port) {
  // frame_retrace_server should be at the same path as frame_retrace
  std::string server_exe(main_exe);

  char sep;
  std::string server_exe_name;

#ifdef WIN32
  sep = '\\';
  server_exe_name = "frame_retrace_server.exe";
#else
  sep = '/';
  server_exe_name = "frame_retrace_server";
#endif

  size_t last_sep = server_exe.rfind(sep);

  if (last_sep != std::string::npos)
    server_exe.resize(last_sep + 1);
  else
    server_exe = std::string("");

  server_exe += server_exe_name;

  std::stringstream port_ss;
  port_ss << port;

  std::string port_str(port_ss.str());

  const char *const args[] = {server_exe.c_str(),
                              "-p",
                              port_str.c_str(),
                              NULL};
  glretrace::fork_execv(server_exe.c_str(), args);
}

void
FrameRetraceModel::setFrame(const QString &filename, int framenumber,
                            const QString &host) {
  // m_retrace = new FrameRetrace(filename.toStdString(), framenumber);
  future = QtConcurrent::run(frame_state_off_thread,
                             filename.toStdString(), framenumber);
  m_state = future.result();
  int port = 24642;
  if (host == "localhost") {
    {
      ServerSocket sock(0);
      port = sock.GetPort();
    }
    GRLOGF(glretrace::WARN, "using port: %d", port);
    exec_retracer(main_exe.toStdString().c_str(), port);
  }

  m_retrace.Init(host.toStdString().c_str(), port);
  struct MD5Context md5c;
  MD5Init(&md5c);
  std::vector<unsigned char> buf(1024 * 1024);
  FILE * fh = fopen(filename.toStdString().c_str(), "r");
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
  std::vector<unsigned char> md5(16);
  MD5Final(md5.data(), &md5c);
  m_retrace.openFile(filename.toStdString(), md5, total_bytes,
                     framenumber, this);

  RenderSelection sel;
  glretrace::renderSelectionFromList(m_selection_count,
                                     m_cached_selection,
                                     &sel);
  m_retrace.retraceApi(sel, this);
}

QQmlListProperty<QRenderBookmark>
FrameRetraceModel::renders() {
  return QQmlListProperty<QRenderBookmark>(this, m_renders_model);
}

QQmlListProperty<QMetric>
FrameRetraceModel::metricList() {
  return QQmlListProperty<QMetric>(this, m_filtered_metric_list);
}


void
FrameRetraceModel::onShaderAssembly(RenderId renderId,
                                    SelectionId selectionCount,
                                    const ShaderAssembly &vertex,
                                    const ShaderAssembly &fragment,
                                    const ShaderAssembly &tess_control,
                                    const ShaderAssembly &tess_eval,
                                    const ShaderAssembly &geom,
                                    const ShaderAssembly &comp) {
  ScopedLock s(m_protect);
  if (m_selection_count != selectionCount)
    // retrace is out of date
    return;
  // do not emit onShaders().  The QShader model (reference) is
  // unchanged, even if it's contents have.  The ShaderControl binds
  // to the contents of the QShader model.
  m_shaders.onShaderAssembly(renderId, selectionCount,
                             vertex, fragment,
                             tess_control, tess_eval,
                             geom, comp);
}


void
FrameRetraceModel::onRenderTarget(SelectionId selectionCount,
                                  ExperimentId experimentCount,
                                  const std::vector<unsigned char> &data) {
  ScopedLock s(m_protect);
  glretrace::FrameImages::instance()->SetImage(data);
  emit onRenderTarget();
}

void
FrameRetraceModel::onShaderCompile(RenderId renderId,
                                   ExperimentId experimentCount,
                                   bool status,
                                   const std::string &errorString) {
  if (errorString.size())
    GRLOGF(WARN, "Compilation error: %s", errorString.c_str());
  m_shader_compile_error = errorString.c_str();
  emit onShaderCompileError();
}

void
FrameRetraceModel::retrace_rendertarget() {
  RenderOptions opt = DEFAULT_RENDER;
  if (m_clear_before_render)
    opt = (RenderOptions) (opt | CLEAR_BEFORE_RENDER);
  if (m_stop_at_render)
    opt = (RenderOptions) (opt | STOP_AT_RENDER);
  RenderTargetType rt_type = glretrace::NORMAL_RENDER;
  if (m_highlight_render)
    rt_type = HIGHLIGHT_RENDER;
  if (m_cached_selection.empty())
    return;
  RenderSelection rs;
  glretrace::renderSelectionFromList(m_selection_count,
                                     m_cached_selection,
                                     &rs);
  m_retrace.retraceRenderTarget(ExperimentId(0),
                                rs,
                                rt_type,
                                opt, this);
}

void
FrameRetraceModel::retrace_shader_assemblies() {
  if (m_cached_selection.empty())
    return;

  RenderSelection rs;
  glretrace::renderSelectionFromList(m_selection_count,
                                     m_cached_selection,
                                     &rs);
  m_retrace.retraceShaderAssembly(rs, this);
}


QString
FrameRetraceModel::renderTargetImage() const {
  ScopedLock s(m_protect);
  static int i = 0;
  std::stringstream ss;
  ss << "image://myimageprovider/image" << ++i << ".png";
  return ss.str().c_str();
}

void
FrameRetraceModel::retrace_api() {
  RenderSelection sel;
  glretrace::renderSelectionFromList(m_selection_count,
                                     m_cached_selection,
                                     &sel);
  m_retrace.retraceApi(sel, this);
}

void
FrameRetraceModel::onFileOpening(bool needUpload,
                                 bool finished,
                                 uint32_t percent_complete) {
  ScopedLock s(m_protect);
  if (finished) {
    const int rcount = m_state->getRenderCount();
    for (int i = 0; i < rcount; ++i) {
      m_renders_model.append(new QRenderBookmark(i));
    }
    emit onRenders();

    m_open_percent = 101;

    // trace initial metrics (GPU Time Elapsed, if available)
    std::vector<MetricId> t_metrics(1);
    t_metrics[0] = m_active_metrics[0];
    m_retrace.retraceMetrics(t_metrics, ExperimentId(0),
                             this);
  }
  if (m_open_percent == percent_complete)
    return;
  m_open_percent = percent_complete;
  emit onOpenPercent();
}

void
FrameRetraceModel::onMetricList(const std::vector<MetricId> &ids,
                                const std::vector<std::string> &names,
                                const std::vector<std::string> &desc) {
  ScopedLock s(m_protect);
  t_ids = ids;
  t_names = names;
  m_metrics_table.init(&m_retrace, m_selection, ids, names, desc,
                       m_state->getRenderCount());
  emit updateMetricList();
}

void
FrameRetraceModel::onMetrics(const MetricSeries &metricData,
                             ExperimentId experimentCount,
                             SelectionId selectionCount) {
  ScopedLock s(m_protect);
  bool vertical_metric = false;
  if (metricData.metric == m_active_metrics[0])
    vertical_metric = true;
  if (vertical_metric)
    m_max_metric = 0.0;
  assert(m_metrics.size() <= metricData.data.size());
  while (m_metrics.size() < metricData.data.size())
    m_metrics.append(BarMetrics());
  auto bar = m_metrics.begin();
  for (auto i : metricData.data) {
    vertical_metric ? bar->metric1 = i : bar->metric2 = i;
    if (vertical_metric && i > m_max_metric)
      m_max_metric = i;
    ++bar;
  }
  emit onMaxMetric();
  emit onQMetricData(m_metrics);
}

void
FrameRetraceModel::onUpdateMetricList() {
  ScopedLock s(m_protect);
  for (auto i : m_metrics_model)
    delete i;
  m_metrics_model.clear();
  m_metrics_model.push_back(new QMetric(MetricId(0), "No metric"));
  assert(t_ids.size() == t_names.size());
  for (int i = 0; i < t_ids.size(); ++i)
    m_metrics_model.append(new QMetric(t_ids[i], t_names[i]));
  filterMetrics("");
  emit onQMetricList();
}

void
FrameRetraceModel::setMetric(int index, int id) {
  if (index >= m_active_metrics.size())
    m_active_metrics.resize(index+1);

  if (m_active_metrics[index] == MetricId(id))
    return;
  m_active_metrics[index] = MetricId(id);

  if (m_open_percent < 100)
    // file not open yet
    return;

  // clear bar data, so stale data will not be displayed.  It may be
  // than one of the metrics we are requesting is "No metric"
  for (auto &bar : m_metrics) {
    bar.metric1 = 0;
    bar.metric2 = 0;
  }

  std::vector<MetricId> t_metrics = m_active_metrics;
  if (t_metrics[1] == MetricId(0))
    // don't replay a null metric for the widths, it makes the bars contiguous.
    t_metrics.resize(1);

  m_retrace.retraceMetrics(t_metrics, ExperimentId(0),
                           this);
}

void
FrameRetraceModel::subscribe(QBarGraphRenderer *graph) {
  ScopedLock s(m_protect);
  connect(this, &FrameRetraceModel::onQMetricData,
          graph, &QBarGraphRenderer::onMetrics);
}

QSelection *
FrameRetraceModel::selection() {
  ScopedLock s(m_protect);
  return m_selection;
}

void
FrameRetraceModel::setSelection(QSelection *s) {
  ScopedLock st(m_protect);
  m_selection = s;
  connect(s, &QSelection::onSelect,
          this, &FrameRetraceModel::onSelect);
}

void
FrameRetraceModel::onSelect(QList<int> selection) {
  ScopedLock s(m_protect);
  m_cached_selection = selection;
  ++m_selection_count;
  retrace_rendertarget();
  retrace_shader_assemblies();
  retrace_api();
}

bool
FrameRetraceModel::clearBeforeRender() const {
  ScopedLock s(m_protect);
  return m_clear_before_render;
}

void
FrameRetraceModel::setClearBeforeRender(bool v) {
  ScopedLock s(m_protect);
  m_clear_before_render = v;
  retrace_rendertarget();
}

bool
FrameRetraceModel::stopAtRender() const {
  ScopedLock s(m_protect);
  return m_stop_at_render;
}

void
FrameRetraceModel::setStopAtRender(bool v) {
  ScopedLock s(m_protect);
  m_stop_at_render = v;
  retrace_rendertarget();
}

bool
FrameRetraceModel::highlightRender() const {
  ScopedLock s(m_protect);
  return m_highlight_render;
}

void
FrameRetraceModel::setHighlightRender(bool v) {
  ScopedLock s(m_protect);
  m_highlight_render = v;
  retrace_rendertarget();
}

void
FrameRetraceModel::refreshMetrics() {
  // sending a second null metric to be retraced will result in two
  // data axis being returned.  Instead, we want a single metric, and
  // the bar graph to show separated bars.
  std::vector<MetricId> drop_second_null_metric = m_active_metrics;
  if (drop_second_null_metric.back() == MetricId(0))
    drop_second_null_metric.pop_back();

  m_retrace.retraceMetrics(drop_second_null_metric, ExperimentId(0),
                           this);
  m_metrics_table.refresh();
}

QString
FrameRetraceModel::apiCalls() {
  ScopedLock s(m_protect);
  return m_api_calls;
}

void
FrameRetraceModel::onApi(SelectionId selectionCount,
                         RenderId renderId,
                         const std::vector<std::string> &api_calls) {
  m_api_calls.clear();

  {
    ScopedLock s(m_protect);
    if (m_selection_count != selectionCount)
      // retrace is out of date
      return;
  }

  // TODO(majanes): cache calls by render
  for (auto i : api_calls) {
    m_api_calls.append(QString::fromStdString(i));
  }
  emit onApiCalls();
}

void
FrameRetraceModel::onError(const std::string &message) {
  GRLOG(ERR, message.c_str());
}

void
FrameRetraceModel::filterMetrics(const QString &f) {
  m_metrics_table.filter(f);

  if (f.size() == 0) {
    m_filtered_metric_list = m_metrics_model;
    emit onQMetricList();
    return;
  }
  m_filtered_metric_list.clear();
  for (auto m : m_metrics_model) {
    if (m->name().contains(f, Qt::CaseInsensitive))
      m_filtered_metric_list.append(m);
  }

  emit onQMetricList();
}

Q_INVOKABLE QString
FrameRetraceModel::urlToFilePath(const QUrl &url) {
  return url.toLocalFile();
}

void
FrameRetraceModel::onShadersChanged() {
  retrace_rendertarget();
  retrace_shader_assemblies();
  m_retrace.retraceMetrics(m_active_metrics, ExperimentId(0),
                           this);
}
