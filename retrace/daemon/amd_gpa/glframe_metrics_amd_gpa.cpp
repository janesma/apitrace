// Copyright (C) Intel Corp.  2018.  All Rights Reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice (including the
// next paragraph) shall be included in all copies or substantial
// portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  **********************************************************************/
//  * Authors:
//  *   Mark Janes <mark.a.janes@intel.com>
//  **********************************************************************/

#include "glframe_metrics_amd_gpa.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include <map>
#include <string>
#include <vector>

#include "glretrace.hpp"

#include "GPUPerfAPI.h"
#include "GPUPerfAPIFunctionTypes.h"

#include "glframe_traits.hpp"

using glretrace::MetricId;
using glretrace::ExperimentId;
using glretrace::SelectionId;
using glretrace::OnFrameRetrace;
using glretrace::RenderId;
using glretrace::PerfMetricsAMDGPA;
using glretrace::PerfContext;
using glretrace::NoAssign;
using glretrace::NoCopy;
using glretrace::ID_PREFIX_MASK;

struct MetricDescription {
  MetricId id;
  std::string name;
  std::string description;
  MetricDescription() {}
  MetricDescription(MetricId i,
                    const std::string &n,
                    const std::string &d)
      : id(i), name(n), description(d) {}
};

class PerfMetric: public NoCopy, NoAssign {
 public:
  explicit PerfMetric(int group, int index);
  MetricId id() const;
  const std::string &name() const { return m_name; }
  const std::string &description() const { return m_description; }
  void enable();
  void publish(gpa_uint32 session,
               const std::vector<RenderId> &samples,
               PerfMetricsAMDGPA::MetricMap *data);
 private:
  const int m_group, m_index;
  std::string m_name, m_description;
  GPA_Type m_type;
};

class PerfGroup : public NoCopy, NoAssign {
 public:
  PerfGroup(int group_index, int metric_index);
  ~PerfGroup();
  int nextMetricIndex() const { return m_next_metric; }
  void metrics(std::vector<MetricDescription> *m) const;
  void begin(RenderId render);
  void end(RenderId render);
  void selectMetric(MetricId metric);
  void selectAll();
  void publish(gpa_uint32 session,
               const std::vector<RenderId> &samples,
               PerfMetricsAMDGPA::MetricMap *data);
 private:
  int m_next_metric;
  std::vector<PerfMetric *> m_metrics;
  std::map<MetricId, int> m_metric_index;
  MetricId m_active_metric;
};

namespace glretrace {

class PerfContext : public NoCopy, NoAssign {
 public:
  explicit PerfContext(OnFrameRetrace *cb);
  ~PerfContext();
  int groupCount() const { return m_groups.size(); }
  void selectMetric(MetricId metric);
  void selectGroup(int index);
  void begin(RenderId render);
  void end();
  void startContext();
  void endContext();
  void publish(PerfMetricsAMDGPA::MetricMap *data);

 private:
  std::vector<PerfGroup*> m_groups;
  std::vector<RenderId> m_open_samples;
  int m_active_group;
  gpa_uint32 m_current_session;
};
}  // namespace glretrace

PerfMetric::PerfMetric(int group, int index) : m_group(group), m_index(index) {
  const char *name, *description;
  GPA_Status ok = GPA_GetCounterName(index, &name);
  assert(ok == GPA_STATUS_OK);
  m_name = name;
  ok = GPA_GetCounterDescription(index, &description);
  assert(ok == GPA_STATUS_OK);
  m_description = description;
  ok = GPA_GetCounterDataType(index, &m_type);
}

MetricId
PerfMetric::id() const {
  // index 0 is used for null metric
  return MetricId(m_group, m_index + 1);
}

void PerfMetric::enable() {
  GPA_Status ok = GPA_EnableCounter(m_index);
  assert(ok == GPA_STATUS_OK);
}

void
PerfMetric::publish(gpa_uint32 session,
                    const std::vector<RenderId> &samples,
                    PerfMetricsAMDGPA::MetricMap *data) {
  GPA_Status ok;
  for (auto render : samples) {
    float val = 0;
    switch (m_type) {
      case GPA_TYPE_FLOAT32: {
        gpa_float32 data;
        ok = GPA_GetSampleFloat32(session, render(), m_index, &data);
        val = data;
        break;
      }
      case GPA_TYPE_FLOAT64: {
        gpa_float64 data;
        ok = GPA_GetSampleFloat64(session, render(), m_index, &data);
        val = data;
        break;
      }
      case GPA_TYPE_INT32:
      case GPA_TYPE_UINT32: {
        gpa_uint32 data;
        ok = GPA_GetSampleUInt32(session, render(), m_index, &data);
        val = data;
        break;
      }
      case GPA_TYPE_INT64:
      case GPA_TYPE_UINT64: {
        gpa_uint64 data;
        ok = GPA_GetSampleUInt64(session, render(), m_index, &data);
        val = data;
        break;
      }
      default:
        assert(false);
    }
    assert(ok == GPA_STATUS_OK);
    (*data)[id()][render] = val;
  }
}


PerfGroup::PerfGroup(int group_index, int metric_index) {
  m_next_metric = metric_index;
  gpa_uint32 max_count;
  GPA_Status ok =  GPA_GetNumCounters(&max_count);
  assert(ok == GPA_STATUS_OK);
  while (m_next_metric < max_count) {
    ok = GPA_EnableCounter(m_next_metric);
    assert(ok == GPA_STATUS_OK);
    gpa_uint32 passes;
    ok = GPA_GetPassCount(&passes);
    assert(ok == GPA_STATUS_OK);
    assert(passes > 0);
    if (passes > 1) {
      if (m_metrics.empty()) {
        // single metric requires 2 passes.  Skip it.
        ok = GPA_DisableAllCounters();
        assert(ok == GPA_STATUS_OK);
        ++m_next_metric;
        continue;
      }
      break;
    }
    m_metrics.push_back(new PerfMetric(group_index, m_next_metric));
    m_metric_index[m_metrics.back()->id()] = m_metrics.size() - 1;
    ++m_next_metric;
  }
  ok = GPA_DisableAllCounters();
  assert(ok == GPA_STATUS_OK);
}

void
PerfGroup::metrics(std::vector<MetricDescription> *out) const {
  for (auto m : m_metrics)
    out->push_back(MetricDescription(m->id(),
                                     m->name(),
                                     m->description()));
}

void
PerfGroup::selectMetric(MetricId metric) {
  GPA_Status ok = GPA_DisableAllCounters();
  assert(ok == GPA_STATUS_OK);
  m_metrics[m_metric_index[metric]]->enable();
  m_active_metric = metric;
}

static const MetricId ALL_METRICS_IN_GROUP = MetricId(~ID_PREFIX_MASK);

void PerfGroup::selectAll() {
  GPA_Status ok = GPA_DisableAllCounters();
  assert(ok == GPA_STATUS_OK);
  for (auto m : m_metrics)
    m->enable();
  m_active_metric = ALL_METRICS_IN_GROUP;
}

void
PerfGroup::publish(gpa_uint32 session,
                   const std::vector<RenderId> &samples,
                   PerfMetricsAMDGPA::MetricMap *data) {
  if (m_active_metric != ALL_METRICS_IN_GROUP)
    return m_metrics[m_metric_index[m_active_metric]]->publish(
        session, samples, data);
  for (auto p : m_metrics)
    p->publish(session, samples, data);
}

PerfContext::PerfContext(OnFrameRetrace *cb) : m_active_group(-1),
                                               m_current_session(-1) {
  gpa_uint32 count;
  GPA_Status ok = GPA_GetNumCounters(&count);
  assert(ok == GPA_STATUS_OK);
  ok = GPA_DisableAllCounters();
  assert(ok == GPA_STATUS_OK);

  int index = 0;
  std::vector<MetricDescription> metrics;
  while (index < count) {
    PerfGroup *pg = new PerfGroup(m_groups.size(), index);
    m_groups.push_back(pg);
    index = m_groups.back()->nextMetricIndex();
    pg->metrics(&metrics);
  }
  std::vector<MetricId> ids;
  std::vector<std::string> names;
  std::vector<std::string> descriptions;
  for (auto m : metrics) {
    ids.push_back(m.id);
    names.push_back(m.name);
    descriptions.push_back(m.description);
  }
  if (cb)
    cb->onMetricList(ids, names, descriptions);
}

void
PerfContext::selectMetric(MetricId metric) {
  m_active_group = metric.group();
  m_groups[metric.group()]->selectMetric(metric);
}

void
PerfContext::selectGroup(int index) {
  m_active_group = index;
  m_groups[index]->selectAll();
}

void PerfContext::startContext() {
  GPA_Status ok = GPA_BeginSession(&m_current_session);
  // assert(ok == GPA_STATUS_OK);
  ok = GPA_BeginPass();
  // assert(ok == GPA_STATUS_OK);
}

void
PerfContext::endContext() {
  if (m_active_group == -1)
    return;
  GPA_Status ok = GPA_EndPass();
  assert(ok == GPA_STATUS_OK);
  ok = GPA_EndSession();
  assert(ok == GPA_STATUS_OK);
}

void
PerfContext::begin(RenderId render) {
  GPA_Status ok = GPA_BeginSample(render());
  assert(ok == GPA_STATUS_OK);
  m_open_samples.push_back(render);
}
void
PerfContext::end() {
  GPA_Status ok = GPA_EndSample();
  assert(ok == GPA_STATUS_OK);
}

static const int INVALID_GROUP = -1;

void
PerfContext::publish(PerfMetricsAMDGPA::MetricMap *data) {
  if (m_active_group == INVALID_GROUP)
    return;
  m_groups[m_active_group]->publish(m_current_session, m_open_samples, data);
  m_open_samples.clear();
}

void gpa_log(GPA_Logging_Type messageType, const char* pMessage) {
  printf("%s\n", pMessage);
}

PerfMetricsAMDGPA::PerfMetricsAMDGPA(OnFrameRetrace *cb)
    : m_current_context(NULL),
      m_current_group(INVALID_GROUP) {
  GPA_Status ok;
  ok =  GPA_RegisterLoggingCallback(GPA_LOGGING_ERROR_AND_MESSAGE, gpa_log);
  assert(ok == GPA_STATUS_OK);

  ok = GPA_Initialize();
  assert(ok == GPA_STATUS_OK);
  Context *c = getCurrentContext();
  if (c == NULL)
    c = reinterpret_cast<Context*>(this);
  ok = GPA_OpenContext(c);
  // assert(ok == GPA_STATUS_OK);
  ok = GPA_SelectContext(c);
  assert(ok == GPA_STATUS_OK);

  m_current_context = new PerfContext(cb);
  m_contexts[c] = m_current_context;
}

PerfMetricsAMDGPA::~PerfMetricsAMDGPA() {
  for (auto i : m_contexts) {
    GPA_SelectContext(i.first);
    GPA_CloseContext();
  }
  GPA_Destroy();
}

int PerfMetricsAMDGPA::groupCount() const {
  return m_contexts.begin()->second->groupCount();
}

void PerfMetricsAMDGPA::selectMetric(MetricId metric) {
  m_current_group = INVALID_GROUP;
  m_current_metric = metric;
  for (auto c : m_contexts)
    c.second->selectMetric(metric);
}

void PerfMetricsAMDGPA::selectGroup(int index) {
  m_current_metric = MetricId();  // no metric
  m_current_group = index;
  for (auto c : m_contexts)
    c.second->selectGroup(index);
}

void PerfMetricsAMDGPA::begin(RenderId render) {
  m_current_context->begin(render);
}
void PerfMetricsAMDGPA::end() {
  m_current_context->end();
}
void PerfMetricsAMDGPA::publish(ExperimentId experimentCount,
                                SelectionId selectionCount,
                                OnFrameRetrace *callback) {
  for (auto i : m_contexts)
    i.second->publish(&m_data);

  for (auto i : m_data) {
    MetricSeries s;
    s.metric = i.first;
    s.data.resize(i.second.rbegin()->first.index() + 1);
    for (auto datapoint : i.second)
      s.data[datapoint.first.index()] = datapoint.second;
    callback->onMetrics(s, experimentCount, selectionCount);
  }
  m_data.clear();
}

void
PerfMetricsAMDGPA::beginContext() {
  Context *c = getCurrentContext();
  GPA_Status ok;
  if (m_contexts.find(c) == m_contexts.end()) {
    ok = GPA_OpenContext(c);
    assert(ok == GPA_STATUS_OK);

    PerfContext *pc = new PerfContext(NULL);
    m_contexts[c] = pc;
  }
  ok = GPA_SelectContext(c);
  assert(ok == GPA_STATUS_OK);
  m_current_context = m_contexts[c];
  if (m_current_group != INVALID_GROUP)
    m_current_context->selectGroup(m_current_group);
  else if (m_current_metric())
    m_current_context->selectMetric(m_current_metric);
  m_current_context->startContext();
}

void
PerfMetricsAMDGPA::endContext() {
  if (m_current_context) {
    m_current_context->endContext();
    // get data from the context before going on to the next one.
    // This is also triggered when iterating metric groups: one group
    // must be published before the next can be collected.
    m_current_context->publish(&m_data);
  }
  m_current_context = NULL;
}

