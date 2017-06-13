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


#ifndef _GLFRAME_RETRACE_MODEL_HPP_
#define _GLFRAME_RETRACE_MODEL_HPP_

#include <QtConcurrentRun>   // NOLINT

#include <QObject>
#include <QList>
#include <QString>
#include <QQmlListProperty>

#include <string>
#include <vector>

#include "glframe_retrace.hpp"
#include "glframe_retrace_stub.hpp"
#include "glframe_api_model.hpp"
#include "glframe_bargraph.hpp"
#include "glframe_batch_model.hpp"
#include "glframe_experiment_model.hpp"
#include "glframe_os.hpp"
#include "glframe_qselection.hpp"
#include "glframe_shader_model.hpp"
#include "glframe_metrics_model.hpp"

namespace glretrace {

class FrameRetrace;
class QSelection;
class QRenderTargetModel;
class QUniformsModel;

class QRenderBookmark : public QObject {
  Q_OBJECT
 public:
  QRenderBookmark() : renderId(0) {}
  QRenderBookmark(const QRenderBookmark& o) : renderId(o.renderId) {}
  Q_PROPERTY(int index READ index NOTIFY onIndex);

  explicit QRenderBookmark(int id)
      : renderId(id) {}

  int index() { return renderId.index(); }
  glretrace::RenderId renderId;
 signals:
  void onIndex();
};

class QMetric : public QObject {
  Q_OBJECT
 public:
  Q_PROPERTY(int id READ id NOTIFY onId);
  Q_PROPERTY(QString name READ name NOTIFY onName);
  QMetric() : m_id(-1), m_name("") {}
  QMetric(MetricId i, const std::string &n)
      : m_id(i()), m_name(QString::fromStdString(n)) {}
  int id() const { return m_id; }
  QString name() const { return m_name; }
 signals:
  void onName();
  void onId();
 private:
  int m_id;
  QString m_name;
};

class QBarGraphRenderer;
class FrameRetraceModel : public QObject,
                          public OnFrameRetrace {
  Q_OBJECT
  Q_PROPERTY(QQmlListProperty<glretrace::QRenderBookmark> renders
             READ renders NOTIFY onRenders)
  Q_PROPERTY(QQmlListProperty<glretrace::QMetric> metricList
             READ metricList NOTIFY onQMetricList);
  Q_PROPERTY(glretrace::QSelection* selection
             READ selection WRITE setSelection);
  Q_PROPERTY(int frameCount READ frameCount NOTIFY onFrameCount)
  Q_PROPERTY(float maxMetric READ maxMetric NOTIFY onMaxMetric)
  Q_PROPERTY(glretrace::QRenderShadersList* shaders READ shaders CONSTANT)
  Q_PROPERTY(glretrace::QApiModel* api READ api CONSTANT)
  Q_PROPERTY(glretrace::QBatchModel* batch READ batch CONSTANT)
  Q_PROPERTY(QString shaderCompileError READ shaderCompileError
             NOTIFY onShaderCompileError)
  Q_PROPERTY(QString argvZero READ argvZero WRITE setArgvZero
             NOTIFY onArgvZero)
  Q_PROPERTY(glretrace::QMetricsModel* metricTab READ metricTab CONSTANT)
  Q_PROPERTY(QString generalError READ generalError
             NOTIFY onGeneralError)
  Q_PROPERTY(QString generalErrorDetails READ generalErrorDetails
             NOTIFY onGeneralError)
  Q_PROPERTY(Severity errorSeverity READ errorSeverity
             NOTIFY onGeneralError)
  Q_PROPERTY(glretrace::QExperimentModel* experimentModel
             READ experiments CONSTANT)
  Q_PROPERTY(glretrace::QRenderTargetModel* rendertarget
             READ rendertarget CONSTANT)
  Q_PROPERTY(glretrace::QUniformsModel* uniformModel
             READ uniformModel CONSTANT)

 public:
  FrameRetraceModel();
  ~FrameRetraceModel();

  virtual void subscribe(QBarGraphRenderer *graph);

  Q_INVOKABLE bool setFrame(const QString &filename, int framenumber,
                            const QString &host);
  Q_INVOKABLE void setMetric(int index, int id);
  Q_INVOKABLE void refreshMetrics();
  Q_INVOKABLE void filterMetrics(const QString &f);
  Q_INVOKABLE QString urlToFilePath(const QUrl &url);
  QQmlListProperty<QRenderBookmark> renders();
  QQmlListProperty<QMetric> metricList();
  QSelection *selection();
  void setSelection(QSelection *s);

  void onFileOpening(bool needUpload, bool finished,
                     uint32_t frame_count);
  void onShaderAssembly(RenderId renderId,
                        SelectionId selectionCount,
                        ExperimentId experimentCount,
                        const ShaderAssembly &vertex,
                        const ShaderAssembly &fragment,
                        const ShaderAssembly &tess_control,
                        const ShaderAssembly &tess_eval,
                        const ShaderAssembly &geom,
                        const ShaderAssembly &comp);
  void onRenderTarget(SelectionId selectionCount,
                      ExperimentId experimentCount,
                      const std::vector<unsigned char> &data);
  void onShaderCompile(RenderId renderId,
                       ExperimentId experimentCount,
                       bool status,
                       const std::string &errorString);
  void onMetricList(const std::vector<MetricId> &ids,
                    const std::vector<std::string> &names,
                    const std::vector<std::string> &desc);
  void onMetrics(const MetricSeries &metricData,
                 ExperimentId experimentCount,
                 SelectionId selectionCount);
  void onApi(SelectionId selectionCount,
             RenderId renderId,
             const std::vector<std::string> &api_calls);
  void onError(ErrorSeverity s, const std::string &message);
  void onBatch(SelectionId selectionCount,
               ExperimentId experimentCount,
               RenderId renderId,
               const std::string &batch);
  void onUniform(SelectionId selectionCount,
                 ExperimentId experimentCount,
                 RenderId renderId,
                 const std::string &name,
                 UniformType type,
                 UniformDimension dimension,
                 const std::vector<unsigned char> &data);
  int frameCount() const { ScopedLock s(m_protect); return m_frame_count; }
  float maxMetric() const { ScopedLock s(m_protect); return m_max_metric; }
  QString apiCalls();
  QRenderShadersList *shaders() { return &m_shaders; }
  QExperimentModel *experiments() { return &m_experiment; }
  QUniformsModel *uniformModel() { return m_uniforms; }
  QApiModel *api() { return &m_api; }
  QBatchModel *batch() { return &m_batch; }
  QRenderTargetModel *rendertarget() { return m_rendertarget; }
  QString shaderCompileError() { return m_shader_compile_error; }
  QString argvZero() { return main_exe; }
  void setArgvZero(const QString &a) { main_exe = a; emit onArgvZero(); }
  QMetricsModel *metricTab() { return &m_metrics_table; }
  QString generalError() { return m_general_error; }
  QString generalErrorDetails() { return m_general_error_details; }

  enum Severity {
        Warning,
        Fatal
    };
  Q_ENUM(Severity);
  Severity errorSeverity() const { return m_severity; }
  void retraceRendertarget();
 public slots:
  void onUpdateMetricList();
  void onSelect(glretrace::SelectionId id, QList<int> selection);
  void onExperiment(glretrace::ExperimentId experiment_count);
 signals:
  void onQMetricList();
  void onQMetricData(QList<glretrace::BarMetrics> metrics);
  void onRenders();
  void onFrameCount();
  void onMaxMetric();
  void onShaderCompileError();
  void onArgvZero();
  void onGeneralError();

  // this signal transfers onMetricList to be handled in the UI
  // thread.  The handler generates QObjects which are passed to qml
  void updateMetricList();
 private:
  void retrace_shader_assemblies();
  void retrace_api();
  void retrace_batch();
  void retrace_uniforms();
  void refreshBarMetrics();

  mutable std::mutex m_protect;
  FrameRetraceStub m_retrace;
  QMetricsModel m_metrics_table;
  QApiModel m_api;
  QBatchModel m_batch;
  QExperimentModel m_experiment;
  QRenderTargetModel *m_rendertarget;
  QUniformsModel *m_uniforms;
  FrameState *m_state;
  QSelection *m_selection;
  SelectionId m_selection_count;
  ExperimentId m_experiment_count;
  QList<int> m_cached_selection;
  QList<QRenderBookmark *> m_renders_model;
  QList<QMetric *> m_metrics_model, m_filtered_metric_list;
  QList<BarMetrics> m_metrics;

  QRenderShadersList m_shaders;
  QString m_shader_compile_error;
  QString main_exe;  // for path to frame_retrace_server

  int m_target_frame_number, m_open_percent, m_frame_count;

  // thread-safe storage for member data updated from the retrace
  // socket thread.
  std::vector<MetricId> t_ids;
  std::vector<std::string> t_names;

  std::vector<MetricId> m_active_metrics;
  float m_max_metric;
  QString m_general_error, m_general_error_details;
  Severity m_severity;
};

}  // namespace glretrace

#endif  // _GLFRAME_RETRACE_MODEL_HPP_
