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

#include "glframe_metrics_model.hpp"

#include <string>
#include <vector>

#include "glframe_qselection.hpp"

using glretrace::ExperimentId;
using glretrace::IFrameRetrace;
using glretrace::MetricId;
using glretrace::MetricSeries;
using glretrace::QMetricValue;
using glretrace::QMetricsModel;
using glretrace::QSelection;
using glretrace::SelectionId;

QMetricValue::QMetricValue() : m_name(""), m_frame_value(0), m_value(0) {
  assert(false);
}

QMetricValue::QMetricValue(QObject *p) {
  moveToThread(p->thread());
}

void
QMetricValue::setName(const std::string &n) {
  m_name = n.c_str();
  emit onName();
}

void
QMetricValue::setValue(float v) {
  m_value = v;
  emit onValue();
}

void
QMetricValue::setFrameValue(float v) {
  m_frame_value = v;
  emit onFrameValue();
}

QMetricsModel::QMetricsModel()
    : m_retrace(NULL), m_current_selection_count(SelectionId(0)) {
}

void
QMetricsModel::init(IFrameRetrace *r,
                    QSelection *qs,
                    const std::vector<MetricId> &ids,
                    const std::vector<std::string> &names,
                    int render_count) {
  m_retrace = r;
  m_render_count = render_count;
  for (int i = 0; i < ids.size(); ++i) {
    QMetricValue *q = new QMetricValue(this);
    q->setName(names[i]);
    m_metric_list.append(q);
    m_metrics[ids[i]] = q;
  }
  RenderSelection s;
  // request frame and initial metrics
  s.id = SelectionId(0);
  s.series.push_back(RenderSequence(RenderId(0), RenderId(render_count)));
  m_retrace->retraceAllMetrics(s, ExperimentId(0), this);
  connect(qs, &QSelection::onSelect,
          this, &QMetricsModel::onSelect);
  emit onMetricsChanged();
}

QQmlListProperty<QMetricValue>
QMetricsModel::metrics() {
  return QQmlListProperty<QMetricValue>(this, m_metric_list);
}

void
QMetricsModel::onMetrics(const MetricSeries &metricData,
                         ExperimentId experimentCount,
                         SelectionId selectionCount) {
  if ((selectionCount != m_current_selection_count) &&
      (selectionCount != SelectionId(0)))
    // a subsequent selection was made when the asynchronous metrics
    // request was retracing
    return;

  if (m_metrics.find(metricData.metric) == m_metrics.end())
    // the metric groups have many duplicates.  The metrics prefers to
    // use the smallest group with a metric of the same name.
    return;

  float value = 0;
  for (auto v : metricData.data)
    value += v;
  if (selectionCount == SelectionId(0))
    m_metrics[metricData.metric]->setFrameValue(value);
  else
    m_metrics[metricData.metric]->setValue(value);
}

void
QMetricsModel::onSelect(QList<int> selection) {
  m_render_selection.clear();
  if (selection.empty()) {
    for (auto m : m_metric_list)
      m->setValue(0.0);
    return;
  }
  m_render_selection.id = ++m_current_selection_count;
  int last_render = -5;
  for (auto i : selection) {
    if (i != last_render + 1) {
      if (last_render >= 0) {
        m_render_selection.series.back().end = RenderId(last_render + 1);
      }
      m_render_selection.series.push_back(RenderSequence(RenderId(i),
                                                         RenderId(0)));
    }
    last_render = i;
  }
  m_render_selection.series.back().end = RenderId(last_render + 1);
  // TODO(majanes) track ExperimentId, request new metrics on
  // experiments
  m_retrace->retraceAllMetrics(m_render_selection, ExperimentId(0), this);
}

void
QMetricsModel::refresh() {
  // retrace the metrics for the full frame
  RenderSelection s;
  s.id = SelectionId(0);
  s.series.push_back(RenderSequence(RenderId(0), RenderId(m_render_count)));
  m_retrace->retraceAllMetrics(s, ExperimentId(0), this);

  // retrace the metrics for the current selection
  if (!m_render_selection.series.empty())
    m_retrace->retraceAllMetrics(m_render_selection,
                                 ExperimentId(0), this);
}

QMetricsModel::~QMetricsModel() {
  for (auto m : m_metric_list)
    delete m;
  m_metric_list.clear();
  m_metrics.clear();
}
