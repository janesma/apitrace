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

#include "glframe_retrace_model.hpp"

#include <QtConcurrentRun>
#include <QFileInfo>

#include <sstream>
#include <string>
#include <vector>

#include "glframe_qbargraph.hpp"
#include "glframe_retrace.hpp"
#include "glframe_logger.hpp"
#include "glframe_qutil.hpp"
#include "glframe_rendertarget_model.hpp"
#include "glframe_socket.hpp"
#include "glframe_uniform_model.hpp"

using glretrace::DEBUG;
using glretrace::ERR;
using glretrace::ErrorSeverity;
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
using glretrace::UniformType;
using glretrace::UniformDimension;

FrameRetraceModel::FrameRetraceModel()
    : m_experiment(&m_retrace),
      m_rendertarget(new QRenderTargetModel(this)),
      m_uniforms(new QUniformsModel(&m_retrace)),
      m_state(NULL),
      m_selection(NULL),
      m_selection_count(0),
      m_experiment_count(0),
      m_open_percent(0),
      m_frame_count(0),
      m_max_metric(0),
      m_severity(Warning) {
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
  m_retrace.Shutdown();
}

FrameState *frame_state_off_thread(std::string filename,
                                   int framenumber) {
  return new FrameState(filename, framenumber);
}

static QFuture<FrameState *> future;

void
exec_retracer(const char *main_exe, int port) {
  // frameretrace_server should be at the same path as frameretrace
  std::string server_exe(main_exe);

  char sep;
  std::string server_exe_name;

#ifdef WIN32
  sep = '\\';
  server_exe_name = "frameretrace_server.exe";
#else
  sep = '/';
  server_exe_name = "frameretrace_server";
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

bool
FrameRetraceModel::setFrame(const QString &filename, int framenumber,
                            const QString &host) {
  QFileInfo check_file(filename);
  if (!check_file.exists())
    return false;
  if (!check_file.isFile())
    return false;

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

  // let the stub calculate the md5 off-thread.  Doing it here
  // conforms better to the interfaces, but blocks the UI.
  std::vector<unsigned char> md5;

  m_target_frame_number = framenumber;
  m_retrace.openFile(filename.toStdString(), md5, 0,
                     framenumber, this);

  RenderSelection sel;
  glretrace::renderSelectionFromList(m_selection_count,
                                     m_cached_selection,
                                     &sel);
  m_retrace.retraceApi(sel, this);
  return true;
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
                                    ExperimentId experimentCount,
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
  assert(experimentCount <= m_experiment_count);
  if (m_experiment_count != experimentCount)
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
  m_rendertarget->onRenderTarget(selectionCount, experimentCount, data);
}

void
FrameRetraceModel::onShaderCompile(RenderId renderId,
                                   ExperimentId experimentCount,
                                   bool status,
                                   const std::string &errorString) {
  m_shaders.onShaderCompile(renderId, experimentCount, status, errorString);
}

void
FrameRetraceModel::retraceRendertarget() {
  if (m_cached_selection.empty())
    return;
  RenderOptions opt = m_rendertarget->options();
  RenderTargetType rt_type = m_rendertarget->type();
  RenderSelection rs;
  glretrace::renderSelectionFromList(m_selection_count,
                                     m_cached_selection,
                                     &rs);
  m_retrace.retraceRenderTarget(m_experiment_count,
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
  m_retrace.retraceShaderAssembly(rs, m_experiment_count, this);
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
FrameRetraceModel::retrace_batch() {
  RenderSelection sel;
  glretrace::renderSelectionFromList(m_selection_count,
                                     m_cached_selection,
                                     &sel);
  m_retrace.retraceBatch(sel, m_experiment_count, this);
}

void
FrameRetraceModel::onFileOpening(bool needUpload,
                                 bool finished,
                                 uint32_t frame_count) {
  ScopedLock s(m_protect);
  if (finished) {
    m_open_percent = 101;
    const int rcount = m_state->getRenderCount();
    for (int i = 0; i < rcount; ++i) {
      m_renders_model.append(new QRenderBookmark(i));
    }
    emit onRenders();

    // trace initial metrics (GPU Time Elapsed, if available)
    std::vector<MetricId> t_metrics(1);
    t_metrics[0] = m_active_metrics[0];
    m_retrace.retraceMetrics(t_metrics, m_experiment_count,
                             this);
  }
  int percent = frame_count * 100 / m_target_frame_number;
  if (m_open_percent == percent)
    // do not update the progress bar
    return;
  m_open_percent = percent;
  m_frame_count = frame_count;
  emit onFrameCount();
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

  m_retrace.retraceMetrics(t_metrics, m_experiment_count,
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
  connect(s, &QSelection::onExperiment,
          this, &FrameRetraceModel::onExperiment);
  connect(s, &QSelection::onSelect,
          &m_experiment, &QExperimentModel::onSelect);
  connect(&m_experiment, &QExperimentModel::onExperiment,
          s, &QSelection::experiment);
  connect(&m_shaders, &QRenderShadersList::shadersChanged,
          s, &QSelection::experiment);
  connect(s, &QSelection::onExperiment,
          &m_shaders, &QRenderShadersList::onExperiment);
  connect(m_uniforms, &QUniformsModel::uniformExperiment,
          s, &QSelection::experiment);
}

void
FrameRetraceModel::onSelect(SelectionId id, QList<int> selection) {
  ScopedLock s(m_protect);
  m_cached_selection = selection;
  m_selection_count = id;

  // refresh currently visible tab
  if (m_current_tab == kRenderTarget)
    retraceRendertarget();
  if (m_current_tab == kShaders)
    retrace_shader_assemblies();
  if (m_current_tab == kApiCalls)
    retrace_api();
  if (m_current_tab == kBatch)
    retrace_batch();
  if (m_current_tab == kUniforms)
    retrace_uniforms();
  if (m_current_tab == kMetrics)
    m_metrics_table.onSelect(id, selection);

  // refresh other tabs
  if (m_current_tab != kRenderTarget)
    retraceRendertarget();
  if (m_current_tab != kShaders)
    retrace_shader_assemblies();
  if (m_current_tab != kApiCalls)
    retrace_api();
  if (m_current_tab != kBatch)
    retrace_batch();
  if (m_current_tab != kUniforms)
    retrace_uniforms();
  if (m_current_tab != kMetrics)
    m_metrics_table.onSelect(id, selection);
}

void
FrameRetraceModel::refreshBarMetrics() {
  // sending a second null metric to be retraced will result in two
  // data axis being returned.  Instead, we want a single metric, and
  // the bar graph to show separated bars.
  std::vector<MetricId> drop_second_null_metric = m_active_metrics;
  if (drop_second_null_metric.back() == MetricId(0))
    drop_second_null_metric.pop_back();

  m_retrace.retraceMetrics(drop_second_null_metric, m_experiment_count,
                           this);
}

void
FrameRetraceModel::refreshMetrics() {
  // "Refresh" button invoke this.
  m_selection->experiment();
}

void
FrameRetraceModel::onApi(SelectionId selectionCount,
                         RenderId renderId,
                         const std::vector<std::string> &api_calls) {
  {
    ScopedLock s(m_protect);
    if (m_selection_count != selectionCount)
      // retrace is out of date
      return;
  }
  m_api.onApi(selectionCount, renderId, api_calls);
}

void
FrameRetraceModel::onError(ErrorSeverity s, const std::string &message) {
  GRLOG(ERR, message.c_str());

  // split on newline, to provide additional context behind the "show
  // details" button.
  QString m(message.c_str());
  QStringList m_l = m.split("\n");
  m_general_error = m_l[0];
  m_general_error_details = "";
  if (m_l.size() > 1)
    m_general_error_details = m_l[1];
  // qml will check severity and quit if necessary
  switch (s) {
    case RETRACE_WARN:
      m_severity = Warning;
      break;
    case RETRACE_FATAL:
      m_severity = Fatal;
      break;
  }
  emit onGeneralError();
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
FrameRetraceModel::onBatch(SelectionId selectionCount,
                           ExperimentId experimentCount,
                           RenderId renderId,
                           const std::string &batch) {
  assert(selectionCount <= m_selection_count);
  if (m_selection_count != selectionCount)
    // retrace is out of date
    return;
  assert(experimentCount <= m_experiment_count);
  if (m_experiment_count != experimentCount)
    // retrace is out of date
    return;
  m_batch.onBatch(selectionCount, experimentCount, renderId, batch);
}

void
FrameRetraceModel::onExperiment(ExperimentId experiment_count) {
  ScopedLock s(m_protect);
  m_experiment_count = experiment_count;

  refreshBarMetrics();

  // refresh the currently shown tab
  if (m_current_tab == kRenderTarget)
    retraceRendertarget();
  if (m_current_tab == kShaders)
    retrace_shader_assemblies();
  if (m_current_tab == kBatch)
    retrace_batch();
  if (m_current_tab == kUniforms)
    retrace_uniforms();
  if (m_current_tab == kMetrics)
    m_metrics_table.onExperiment(experiment_count);

  // refresh the rest of the tabs
  if (m_current_tab != kRenderTarget)
    retraceRendertarget();
  if (m_current_tab != kShaders)
    retrace_shader_assemblies();
  if (m_current_tab != kBatch)
    retrace_batch();
  if (m_current_tab != kUniforms)
    retrace_uniforms();
  if (m_current_tab != kMetrics)
    m_metrics_table.onExperiment(experiment_count);
}

void
FrameRetraceModel::onUniform(SelectionId selectionCount,
                             ExperimentId experimentCount,
                             RenderId renderId,
                             const std::string &name,
                             UniformType type,
                             UniformDimension dimension,
                             const std::vector<unsigned char> &data) {
  m_uniforms->onUniform(selectionCount, experimentCount, renderId,
                        name, type, dimension, data);
}

void
FrameRetraceModel::retrace_uniforms() {
  RenderSelection sel;
  glretrace::renderSelectionFromList(m_selection_count,
                                     m_cached_selection,
                                     &sel);
  m_uniforms->clear();
  m_retrace.retraceUniform(sel, m_experiment_count, this);
}

void
FrameRetraceModel::setTab(const int index) {
  m_current_tab = static_cast<TabIndex>(index);
}
