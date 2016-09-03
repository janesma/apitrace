/**************************************************************************
 *
 * Copyright 2016 Intel Corporation
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

#ifndef _GLFRAME_METRICS_MODEL_HPP_
#define _GLFRAME_METRICS_MODEL_HPP_

#include <QList>
#include <QObject>
#include <QQmlListProperty>
#include <QString>

#include <map>
#include <string>
#include <vector>

#include "glframe_retrace_interface.hpp"
#include "glframe_traits.hpp"

namespace glretrace {

class QSelection;
class QMetricValue : public QObject,
                NoCopy, NoAssign, NoMove {
  Q_OBJECT
  Q_PROPERTY(QString name READ name NOTIFY onName)
  Q_PROPERTY(float value READ value NOTIFY onValue)
  Q_PROPERTY(float frameValue READ frameValue NOTIFY onFrameValue)

 public:
  QMetricValue();
  explicit QMetricValue(QObject *p);
  void setName(const std::string &n);
  void setValue(float v);
  void setFrameValue(float v);
  QString name() const { return m_name; }
  float value() const { return m_value; }
  float frameValue() const { return m_frame_value; }
 signals:
  void onValue();
  void onFrameValue();
  void onName();
 private:
  QString m_name;
  float m_frame_value, m_value;
};

class QMetricsModel : public QObject, OnFrameRetrace,
                      NoCopy, NoAssign, NoMove {
  Q_OBJECT
  Q_PROPERTY(QQmlListProperty<glretrace::QMetricValue> metrics
             READ metrics NOTIFY onMetricsChanged)
 public:
  QMetricsModel();
  ~QMetricsModel();
  void init(IFrameRetrace *r,
            QSelection *s,
            const std::vector<MetricId> &ids,
            const std::vector<std::string> &names,
            int render_count);
  void onFileOpening(bool needUpload,
                     bool finished,
                     uint32_t percent_complete) { assert(false); }
  void onShaderAssembly(RenderId renderId,
                        const ShaderAssembly &vertex,
                        const ShaderAssembly &fragment,
                        const ShaderAssembly &tess_control,
                        const ShaderAssembly &tess_eval,
                        const ShaderAssembly &geon) { assert(false); }
  void onRenderTarget(RenderId renderId, RenderTargetType type,
                      const uvec & pngImageData) { assert(false); }
  void onMetricList(const std::vector<MetricId> &ids,
                    const std::vector<std::string> &names) { assert(false); }
  void onMetrics(const MetricSeries &metricData,
                 ExperimentId experimentCount,
                 SelectionId selectionCount);
  void onShaderCompile(RenderId renderId,
                       ExperimentId experimentCount,
                       bool status,
                       const std::string &errorString) { assert(false); }
  void onApi(RenderId renderId,
             const std::vector<std::string> &api_calls) { assert(false); }
  void onError(const std::string &message) { assert(false); }

  QQmlListProperty<QMetricValue> metrics();
  void refresh();

 public slots:
  void onSelect(QList<int> selection);

 signals:
  void onMetricsChanged();

 private:
  IFrameRetrace *m_retrace;
  int m_render_count;
  SelectionId m_current_selection_count;
  RenderSelection m_render_selection;
  std::map<MetricId, QMetricValue*> m_metrics;
  QList<QMetricValue *> m_metric_list;
};

}  // namespace glretrace

#endif  // _GLFRAME_METRICS_MODEL_HPP_
