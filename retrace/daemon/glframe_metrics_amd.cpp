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

#include "glframe_metrics_amd.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include <string>
#include <vector>
#include <map>

#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"
#include "glretrace.hpp"

using glretrace::ExperimentId;
using glretrace::GlFunctions;
using glretrace::MetricId;
using glretrace::NoAssign;
using glretrace::NoCopy;
using glretrace::OnFrameRetrace;
using glretrace::PerfMetrics;
using glretrace::PerfMetricsAMD;
using glretrace::PerfMetricsContextAMD;
using glretrace::RenderId;
using glretrace::SelectionId;
using glretrace::GL;
using glretrace::glretrace_delay;
using glretrace::ID_PREFIX_MASK;

namespace {

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

class PerfMetric : public NoCopy, NoAssign {
 public:
  PerfMetric(int group_id, int counter_num);
  MetricId id() const;
  const std::string &name() const;
  const std::string &description() const;
  void getMetric(const unsigned char *p_value,
                 float *val,
                 int *bytes_read) const;
 private:
  const int m_group_id, m_counter_num;
  std::string m_name, m_description;

  enum CounterType {
    kInt64Counter = GL_UNSIGNED_INT64_AMD,
    kPercentCounter =  GL_PERCENTAGE_AMD,
    kUnsignedCounter = GL_UNSIGNED_INT,
    kFloatCounter = GL_FLOAT
  } m_counter_type;
};


class PerfMetricGroup : public NoCopy, NoAssign {
 public:
  explicit PerfMetricGroup(int group_id, int offset);
  ~PerfMetricGroup();
  const std::string &name() const { return m_group_name; }
  void metrics(std::vector<MetricDescription> *m) const;
  void begin(RenderId render);
  void end(RenderId render);
  void publish(MetricId metric, PerfMetricsAMD::MetricMap *m);
  void selectMetric(MetricId metric);

 private:
  void selectMetric(MetricId metric, bool enabled);
  std::string m_group_name;
  const int m_group_id, m_offset;
  std::vector<unsigned char> m_data_buf;
  MetricId m_metric;

  std::map<MetricId, PerfMetric *> m_metrics;

  // represent monitors that have not produced results
  std::map<RenderId, int> m_extant_monitors;

  // represent monitors that can be reused
  std::vector<unsigned int> m_free_monitors;
};

}  // namespace


namespace glretrace {

class PerfMetricsContextAMD : public NoCopy, NoAssign {
 public:
  explicit PerfMetricsContextAMD(OnFrameRetrace *cb);
  ~PerfMetricsContextAMD();
  int groupCount() const;
  void selectMetric(MetricId metric);
  void selectGroup(int index);
  void begin(RenderId render);
  void end();
  void publish(PerfMetricsAMD::MetricMap *metrics);
 private:
  std::vector<PerfMetricGroup *> m_groups;
  // indicates offset in groups of PerfMetricGroupAMD reporting MetricId
  std::map<MetricId, int> metric_map;
  // indicates the group that will handle subsequent begin/end calls
  PerfMetricGroup *current_group;
  MetricId current_metric;
  RenderId current_render;
};

}  // namespace glretrace

PerfMetricsContextAMD::PerfMetricsContextAMD(OnFrameRetrace *cb)
  : current_group(NULL) {
  std::string extensions;

  GlFunctions::GetGlExtensions(&extensions);
  if (extensions.find("GL_AMD_performance_monitor") == std::string::npos)
    return;

  int num_groups;
  GlFunctions::GetPerfMonitorGroupsAMD(&num_groups, 0, NULL);
  assert(num_groups > 0);
  if (num_groups == 0) {
    GRLOG(WARN, "AMD_performance_monitor supported, but no metrics "
          "provided by platform");
    return;
  }
  std::vector<uint> groups(num_groups);
  assert(!GL::GetError());
  GlFunctions::GetPerfMonitorGroupsAMD(&num_groups, num_groups, groups.data());
  assert(!GL::GetError());

  std::map<std::string, MetricDescription> known_metrics;

  std::vector<MetricDescription> metrics;

  for (int group_index = 0; group_index < num_groups; ++group_index) {
    // query max active counters, and make a subgroup
    int num_counters;
    int max_active_counters;
    GlFunctions::GetPerfMonitorCountersAMD(groups[group_index],
                                           &num_counters,
                                           &max_active_counters,
                                           0, NULL);
    int offset = 0;
    while (offset < num_counters) {
      PerfMetricGroup *g = new PerfMetricGroup(groups[group_index], offset);
      m_groups.push_back(g);
      metrics.clear();
      g->metrics(&metrics);
      for (auto &d : metrics) {
        assert(known_metrics.find(d.name) == known_metrics.end());
        known_metrics[d.name] = d;
        metric_map[d.id] = m_groups.size() - 1;
      }
      offset += max_active_counters;
    }
  }
  std::vector<MetricId> ids;
  std::vector<std::string> names;
  std::vector<std::string> descriptions;
  for (auto &i : known_metrics) {
    names.push_back(i.second.name);
    ids.push_back(i.second.id);
    descriptions.push_back(i.second.description);
  }
  if (cb)
    // only send metrics list on first context
    cb->onMetricList(ids, names, descriptions);
}

PerfMetricsContextAMD::~PerfMetricsContextAMD() {
  for (auto g : m_groups)
    delete g;
  m_groups.clear();
}

static const MetricId ALL_METRICS_IN_GROUP = MetricId(~ID_PREFIX_MASK);

PerfMetricGroup::PerfMetricGroup(int group_id, int offset)
    : m_group_id(group_id),
      m_offset(offset),
      m_metric(ALL_METRICS_IN_GROUP) {
  static GLint max_name_len = 0;
  assert(!GL::GetError());
  if (max_name_len == 0)
    GlFunctions::GetPerfMonitorGroupStringAMD(m_group_id, 0,
                                              &max_name_len, NULL);
  assert(!GL::GetError());
  std::vector<GLchar> group_name(max_name_len + 1);
  GLsizei name_len;
  GlFunctions::GetPerfMonitorGroupStringAMD(m_group_id, max_name_len + 1,
                                            &name_len, group_name.data());
  assert(!GL::GetError());
  m_group_name = group_name.data();

  int num_counters;
  int max_active_counters;
  GlFunctions::GetPerfMonitorCountersAMD(m_group_id,
                                         &num_counters,
                                         &max_active_counters,
                                         0, NULL);
  assert(offset < num_counters);
  assert(!GL::GetError());
  std::vector<uint> counters(num_counters);
  GlFunctions::GetPerfMonitorCountersAMD(m_group_id,
                                         &num_counters,
                                         &max_active_counters,
                                         num_counters, counters.data());
  assert(!GL::GetError());
  for (int i = 0;
       (i < max_active_counters) && (i + offset < num_counters);
       i++) {
    PerfMetric *p = new PerfMetric(m_group_id, counters[offset + i]);
    m_metrics[p->id()] = p;
  }
}

PerfMetricGroup::~PerfMetricGroup() {
  for (auto i : m_extant_monitors)
    m_free_monitors.push_back(i.second);
  m_extant_monitors.clear();
  GlFunctions::DeletePerfMonitorsAMD(m_free_monitors.size(),
                                     m_free_monitors.data());
  assert(!GL::GetError());
  m_free_monitors.clear();
  for (auto i : m_metrics)
    delete i.second;
  m_metrics.clear();
}

void
PerfMetricGroup::metrics(std::vector<MetricDescription> *m) const {
  for (auto &i : m_metrics) {
    m->push_back(MetricDescription(i.first,
                                   i.second->name(),
                                   i.second->description()));
  }
}

void
PerfMetricGroup::selectMetric(MetricId metric) {
  selectMetric(m_metric, false);
  m_metric = metric;
  assert(m_extant_monitors.empty());
  selectMetric(m_metric, true);
}

void
PerfMetricGroup::selectMetric(MetricId metric, bool enabled) {
  std::vector<uint> counters;
  if (metric == ALL_METRICS_IN_GROUP) {
    for (auto metric : m_metrics) {
      counters.push_back(metric.first.counter());
    }
  } else {
    counters.push_back(metric.counter());
  }
  for (auto i : m_free_monitors) {
    GlFunctions::SelectPerfMonitorCountersAMD(
        i, enabled,
        m_group_id, counters.size(),
        counters.data());
  }
  assert(!GL::GetError());
}


void
PerfMetricGroup::begin(RenderId render) {
  if (m_free_monitors.empty()) {
    assert(!GL::GetError());
    m_free_monitors.resize(m_extant_monitors.empty() ?
                           8 : m_extant_monitors.size());
    GlFunctions::GenPerfMonitorsAMD(m_free_monitors.size(),
                                    m_free_monitors.data());
    assert(!GL::GetError());
    selectMetric(m_metric, true);
  }
  assert(!m_free_monitors.empty());
  GlFunctions::BeginPerfMonitorAMD(m_free_monitors.back());
  m_extant_monitors[render] = m_free_monitors.back();
  m_free_monitors.pop_back();
}

void
PerfMetricGroup::publish(MetricId metric,
                         PerfMetricsAMD::MetricMap *out_metrics) {
  for (auto extant_monitor : m_extant_monitors) {
    GLuint ready_for_read = 0, data_size = 0;
    GLsizei bytes_written = 0;
    while (!ready_for_read) {
      GlFunctions::GetPerfMonitorCounterDataAMD(
          extant_monitor.second, GL_PERFMON_RESULT_AVAILABLE_AMD,
          sizeof(GLuint), &ready_for_read, &bytes_written);
      assert(bytes_written == sizeof(GLuint));
      assert(!GL::GetError());
      if (!ready_for_read)
        GlFunctions::Finish();
    }
    assert(ready_for_read);
    GlFunctions::GetPerfMonitorCounterDataAMD(extant_monitor.second,
                                              GL_PERFMON_RESULT_SIZE_AMD,
                                              sizeof(GLuint), &data_size,
                                              &bytes_written);
    assert(!GL::GetError());
    assert(bytes_written == sizeof(GLuint));
    std::vector<unsigned char> buf(data_size);
    GlFunctions::GetPerfMonitorCounterDataAMD(
            extant_monitor.second, GL_PERFMON_RESULT_AMD, data_size,
            reinterpret_cast<uint *>(buf.data()), &bytes_written);
    const unsigned char *buf_ptr = buf.data();
    const unsigned char *buf_end = buf_ptr + bytes_written;
    while (buf_ptr < buf_end) {
      const GLuint *group = reinterpret_cast<const GLuint *>(buf_ptr);
      const GLuint *counter = group + 1;
      buf_ptr += 2*sizeof(GLuint);
      assert(*group == m_group_id);
      if (metric != ALL_METRICS_IN_GROUP)
        assert(metric.counter() == *counter);
      MetricId parsed_metric(*group, *counter);
      assert(m_metrics.find(parsed_metric) != m_metrics.end());

      float value;
      int bytes_read;
      m_metrics[parsed_metric]->getMetric(buf_ptr, &value, &bytes_read);
      (*out_metrics)[parsed_metric][extant_monitor.first] = value;
      buf_ptr += bytes_read;
    }
    m_free_monitors.push_back(extant_monitor.second);
  }
  m_extant_monitors.clear();
}

void
PerfMetricGroup::end(RenderId render) {
  auto i = m_extant_monitors.find(render);
  if (i == m_extant_monitors.end())
    return;
  GlFunctions::EndPerfMonitorAMD(i->second);
}


PerfMetric::PerfMetric(int group_id,
                       int counter_num) : m_group_id(group_id),
                                          m_counter_num(counter_num) {
  GLsizei length;
  GlFunctions::GetPerfMonitorCounterStringAMD(
      m_group_id, m_counter_num, 0, &length, NULL);
  assert(!GL::GetError());
  std::vector<char> name(length + 1);
  GlFunctions::GetPerfMonitorCounterStringAMD(
    m_group_id, m_counter_num, length + 1, &length,
    name.data());
  assert(!GL::GetError());
  m_name = name.data();

  GLuint counter_type;
  GlFunctions::GetPerfMonitorCounterInfoAMD(
      m_group_id, m_counter_num, GL_COUNTER_TYPE_AMD, &counter_type);
  m_counter_type = static_cast<PerfMetric::CounterType>(counter_type);
}

MetricId
PerfMetric::id() const {
  return MetricId(m_group_id, m_counter_num);
}

const std::string &
PerfMetric::name() const {
  return m_name;
}

const std::string &
PerfMetric::description() const {
  return m_description;
}

void
PerfMetric::getMetric(const unsigned char *p_value,
                      float *val,
                      int *bytes_read) const {
  switch (m_counter_type) {
  case kInt64Counter: {
    uint64_t uval = *reinterpret_cast<const uint64_t *>(p_value);
    *val = static_cast<float>(uval);
    *bytes_read = sizeof(uint64_t);
    break;
  }
  case kPercentCounter:
  case kFloatCounter: {
    *val = *reinterpret_cast<const float *>(p_value);
    *bytes_read = sizeof(float);
    break;
  }
  case kUnsignedCounter: {
    uint32_t uval = *reinterpret_cast<const uint32_t *>(p_value);
    *val = static_cast<float>(uval);
    *bytes_read = sizeof(uint32_t);
    break;
  }
  default:
    assert(false);
  }
}

void
PerfMetricsContextAMD::selectMetric(MetricId metric) {
  assert(metric_map.find(metric) != metric_map.end());
  current_metric = metric;
  current_group = m_groups[metric_map[metric]];
  current_group->selectMetric(metric);
}

void
PerfMetricsContextAMD::publish(PerfMetricsAMD::MetricMap *metrics)  {
  current_group->publish(current_metric, metrics);
}

void
PerfMetricsContextAMD::begin(RenderId render) {
  current_group->begin(render);
  current_render = render;
}

void
PerfMetricsContextAMD::end() {
  current_group->end(current_render);
}

int
PerfMetricsContextAMD::groupCount() const {
  return m_groups.size();
}

void
PerfMetricsContextAMD::selectGroup(int index) {
  current_group = m_groups[index];
  current_metric = ALL_METRICS_IN_GROUP;
}

PerfMetricsAMD::PerfMetricsAMD(OnFrameRetrace *cb)
  : m_current_group(0) {
  Context *c = getCurrentContext();
  m_current_context = new PerfMetricsContextAMD(cb);
  m_contexts[c] = m_current_context;
}

PerfMetricsAMD::~PerfMetricsAMD() {
  for (auto i : m_contexts) {
    delete i.second;
  }
  m_contexts.clear();
}

int
PerfMetricsAMD::groupCount() const {
  assert(!m_contexts.empty());
  return m_contexts.begin()->second->groupCount();
}

void
PerfMetricsAMD::selectMetric(MetricId metric) {
  m_data.clear();
  m_current_metric = metric;
  for (auto i : m_contexts)
    i.second->selectMetric(metric);
}

void
PerfMetricsAMD::selectGroup(int index) {
  // TODO(majanes): should we m_data.clear();
  m_current_group = index;
  m_current_metric = ALL_METRICS_IN_GROUP;
  for (auto i : m_contexts)
    i.second->selectGroup(index);
}

void
PerfMetricsAMD::begin(RenderId render) {
  if (!m_current_context) {
    beginContext();
  }
  m_current_context->begin(render);
}

void
PerfMetricsAMD::end() {
  if (m_current_context)
    m_current_context->end();
}

void
PerfMetricsAMD::publish(ExperimentId experimentCount,
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
PerfMetricsAMD::beginContext() {
  Context *c = getCurrentContext();
  auto entry = m_contexts.find(c);
  if (entry != m_contexts.end()) {
    m_current_context = entry->second;
  } else {
    // create a new metrics context
    GRLOG(glretrace::WARN, "new context in frame");
    m_current_context = new PerfMetricsContextAMD(NULL);
    m_contexts[c] = m_current_context;
  }
  m_current_context->selectGroup(m_current_group);
  if (m_current_metric() &&
      (m_current_metric != ALL_METRICS_IN_GROUP))
    m_current_context->selectMetric(m_current_metric);
}

void
PerfMetricsAMD::endContext() {
  if (m_current_context) {
    m_current_context->publish(&m_data);
  }
  m_current_context = NULL;
}
