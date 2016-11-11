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

#include <QQuickImageProvider>   // NOLINT
#include <QtConcurrentRun>   // NOLINT

#include <QObject>
#include <QList>
#include <QString>
#include <QQmlListProperty>

#include <string>
#include <vector>

#include "glframe_retrace.hpp"
#include "glframe_retrace_stub.hpp"
#include "glframe_os.hpp"
#include "glframe_bargraph.hpp"
#include "glframe_qselection.hpp"
#include "glframe_shader_model.hpp"
#include "glframe_metrics_model.hpp"

namespace glretrace {

class FrameRetrace;
class QSelection;
class QShader;

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
  Q_PROPERTY(QString renderTargetImage READ renderTargetImage
             NOTIFY onRenderTarget)
  Q_PROPERTY(int openPercent READ openPercent NOTIFY onOpenPercent)
  Q_PROPERTY(float maxMetric READ maxMetric NOTIFY onMaxMetric)
  Q_PROPERTY(bool clearBeforeRender READ clearBeforeRender
             WRITE setClearBeforeRender)
  Q_PROPERTY(bool stopAtRender READ stopAtRender
             WRITE setStopAtRender)
  Q_PROPERTY(bool highlightRender READ highlightRender
             WRITE setHighlightRender)
  Q_PROPERTY(QString apiCalls
             READ apiCalls NOTIFY onApiCalls)
  Q_PROPERTY(glretrace::QShader* vsShader READ vsShader NOTIFY onShaders)
  Q_PROPERTY(glretrace::QShader* fsShader READ fsShader NOTIFY onShaders)
  Q_PROPERTY(glretrace::QShader* tessControlShader READ tessControlShader
             NOTIFY onShaders)
  Q_PROPERTY(glretrace::QShader* tessEvalShader READ tessEvalShader
             NOTIFY onShaders)
  Q_PROPERTY(glretrace::QShader* geomShader READ geomShader
             NOTIFY onShaders)
  Q_PROPERTY(glretrace::QShader* compShader READ compShader
             NOTIFY onShaders)
  Q_PROPERTY(QString shaderCompileError READ shaderCompileError
             NOTIFY onShaderCompileError)
  Q_PROPERTY(QString argvZero READ argvZero WRITE setArgvZero
             NOTIFY onArgvZero)
  Q_PROPERTY(glretrace::QMetricsModel* metricTab READ metricTab CONSTANT)

 public:
  FrameRetraceModel();
  ~FrameRetraceModel();

  virtual void subscribe(QBarGraphRenderer *graph);

  Q_INVOKABLE void setFrame(const QString &filename, int framenumber,
                            const QString &host);
  Q_INVOKABLE void setMetric(int index, int id);
  Q_INVOKABLE void overrideShaders(const QString &vs, const QString &fs,
                                   const QString &tess_control,
                                   const QString &tess_eval,
                                   const QString &geom, const QString &comp);
  Q_INVOKABLE void refreshMetrics();
  Q_INVOKABLE void filterMetrics(const QString &f);
  Q_INVOKABLE QString urlToFilePath(const QUrl &url);
  QQmlListProperty<QRenderBookmark> renders();
  QQmlListProperty<QMetric> metricList();
  QSelection *selection();
  void setSelection(QSelection *s);

  void onFileOpening(bool needUpload, bool finished,
                     uint32_t percent_complete);
  void onShaderAssembly(RenderId renderId,
                        const ShaderAssembly &vertex,
                        const ShaderAssembly &fragment,
                        const ShaderAssembly &tess_control,
                        const ShaderAssembly &tess_eval,
                        const ShaderAssembly &geom,
                        const ShaderAssembly &comp);
  void onRenderTarget(RenderId renderId, RenderTargetType type,
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
  void onApi(RenderId renderId, const std::vector<std::string> &api_calls);
  void onError(const std::string &message);
  QString renderTargetImage() const;
  int openPercent() const { ScopedLock s(m_protect); return m_open_percent; }
  float maxMetric() const { ScopedLock s(m_protect); return m_max_metric; }
  QString apiCalls();
  QShader *vsShader() { return &m_vs; }
  QShader *fsShader() { return &m_fs; }
  QShader *tessControlShader() { return &m_tess_control; }
  QShader *tessEvalShader() { return &m_tess_eval; }
  QShader *geomShader() { return &m_geom; }
  QShader *compShader() { return &m_comp; }
  QString shaderCompileError() { return m_shader_compile_error; }
  QString argvZero() { return main_exe; }
  void setArgvZero(const QString &a) { main_exe = a; emit onArgvZero(); }
  QMetricsModel *metricTab() { return &m_metrics_table; }

  bool clearBeforeRender() const;
  void setClearBeforeRender(bool v);
  bool stopAtRender() const;
  void setStopAtRender(bool v);
  bool highlightRender() const;
  void setHighlightRender(bool v);
 public slots:
  void onUpdateMetricList();
  void onSelect(QList<int> selection);

 signals:
  void onShaders();
  void onQMetricList();
  void onQMetricData(QList<glretrace::BarMetrics> metrics);
  void onRenders();
  void onRenderTarget();
  void onOpenPercent();
  void onMaxMetric();
  void onApiCalls();
  void onShaderCompileError();
  void onArgvZero();

  // this signal transfers onMetricList to be handled in the UI
  // thread.  The handler generates QObjects which are passed to qml
  void updateMetricList();
 private:
  void retrace_rendertarget();
  void retrace_shader_assemblies();
  void retrace_api();

  mutable std::mutex m_protect;
  FrameRetraceStub m_retrace;
  QMetricsModel m_metrics_table;
  FrameState *m_state;
  QSelection *m_selection;
  SelectionId m_selection_count;
  QList<int> m_cached_selection;
  QList<QRenderBookmark *> m_renders_model;
  QList<QMetric *> m_metrics_model, m_filtered_metric_list;
  QList<BarMetrics> m_metrics;

  QShader m_vs, m_fs, m_tess_control, m_tess_eval, m_geom, m_comp;
  QString m_shader_compile_error;
  QString main_exe;  // for path to frame_retrace_server

  QString m_api_calls;
  int m_open_percent;

  // thread-safe storage for member data updated from the retrace
  // socket thread.
  std::vector<MetricId> t_ids;
  std::vector<std::string> t_names;

  std::vector<MetricId> m_active_metrics;
  float m_max_metric;
  bool m_clear_before_render, m_stop_at_render, m_highlight_render;
};

}  // namespace glretrace

#endif  // _GLFRAME_RETRACE_MODEL_HPP_
