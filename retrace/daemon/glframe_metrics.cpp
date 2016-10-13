// Copyright (C) Intel Corp.  2015.  All Rights Reserved.

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

#include "glframe_metrics.hpp"

#include <GL/gl.h>
#include <GL/glext.h>

#include <string>
#include <vector>
#include <map>

#include "glframe_glhelper.hpp"

using glretrace::ExperimentId;
using glretrace::GlFunctions;
using glretrace::MetricId;
using glretrace::NoAssign;
using glretrace::NoCopy;
using glretrace::OnFrameRetrace;
using glretrace::PerfMetricGroup;
using glretrace::PerfMetrics;
using glretrace::RenderId;
using glretrace::SelectionId;

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
  PerfMetric(int query_id, int counter_num);
  MetricId id() const;
  const std::string &name() const;
  const std::string &description() const;
  float getMetric(const std::vector<unsigned char> &data) const;
 private:
  const int m_query_id, m_counter_num;
  GLuint m_offset, m_data_size, m_type,
    m_data_type;
  std::string m_name, m_description;
};

}  // namespace

class PerfMetricGroup : public NoCopy, NoAssign {
 public:
  explicit PerfMetricGroup(int query_id);
  ~PerfMetricGroup();
  const std::string &name() const { return m_query_name; }
  void metrics(std::vector<MetricDescription> *m) const;
  void begin(RenderId render);
  void end(RenderId render);
  void publish(MetricId metric,
               ExperimentId experimentCount,
               SelectionId selectionCount,
               OnFrameRetrace *callback);

 private:
  std::string m_query_name;
  const int m_query_id;
  unsigned int m_data_size;
  unsigned int m_number_counters;
  unsigned int m_capabilities_mask;
  std::vector<unsigned char> m_data_buf;

  std::map<MetricId, PerfMetric *> m_metrics;

  // represent queries that have not produced results
  std::map<RenderId, int> m_extant_query_handles;

  // represent query handles that can be reused
  std::vector<unsigned int> m_free_query_handles;
};

PerfMetrics::PerfMetrics(OnFrameRetrace *cb) : current_group(NULL) {
  GLuint query_id;
  GLint count;
  bool has_metrics = false;
  GlFunctions::GetIntegerv(GL_NUM_EXTENSIONS, &count);
  for (int i = 0; i < count; ++i) {
    const GLubyte *name = GlFunctions::GetStringi(GL_EXTENSIONS, i);
    if (strcmp((const char*)name, "GL_INTEL_performance_query") == 0)
      has_metrics = true;
  }
  if (!has_metrics)
    return;

  GlFunctions::GetFirstPerfQueryIdINTEL(&query_id);
  if (query_id == -1)
    return;

  if (query_id == 0)
    return;
  std::vector<unsigned int> query_ids;
  query_ids.push_back(query_id);

  while (true) {
    GlFunctions::GetNextPerfQueryIdINTEL(query_id, &query_id);
    if (!query_id)
      break;
    query_ids.push_back(query_id);
  }

  std::map<std::string, MetricDescription> known_metrics;

  std::vector<MetricDescription> metrics;
  int group_index = 0;
  for (auto i : query_ids) {
    PerfMetricGroup *g = new PerfMetricGroup(i);
    if (g->name() == "Compute Metrics Extended Gen9") {
      // SKL metrics bug.  Queries on this group crash.
      delete g;
      continue;
    }

    groups.push_back(g);
    metrics.clear();
    g->metrics(&metrics);
    for (auto &d : metrics) {
      if (known_metrics.find(d.name) == known_metrics.end()) {
        known_metrics[d.name] = d;
        metric_map[d.id] = group_index;
      }
    }
    ++group_index;
  }
  std::vector<MetricId> ids;
  std::vector<std::string> names;
  std::vector<std::string> descriptions;
  for (auto &i : known_metrics) {
    names.push_back(i.second.name);
    ids.push_back(i.second.id);
    descriptions.push_back(i.second.description);
  }
  cb->onMetricList(ids, names, descriptions);
}

PerfMetrics::~PerfMetrics() {
  for (auto g : groups)
    delete g;
  groups.clear();
}

PerfMetricGroup::PerfMetricGroup(int query_id) : m_query_id(query_id) {
  static GLint max_name_len = 0;
  if (max_name_len == 0)
    GlFunctions::GetIntegerv(GL_PERFQUERY_QUERY_NAME_LENGTH_MAX_INTEL,
                             &max_name_len);

  std::vector<GLchar> query_name(max_name_len);
  unsigned int number_instances, capabilities_mask, number_counters;
  GlFunctions::GetPerfQueryInfoINTEL(m_query_id,
                                     query_name.size(), query_name.data(),
                                     &m_data_size, &number_counters,
                                     &number_instances, &capabilities_mask);
  m_data_buf.resize(m_data_size);
  m_query_name = query_name.data();
  for (unsigned int counter_num = 1; counter_num <= number_counters;
       ++counter_num) {
    PerfMetric *p = new PerfMetric(m_query_id, counter_num);
    m_metrics[p->id()] = p;
  }
}

PerfMetricGroup::~PerfMetricGroup() {
  for (auto free_query : m_free_query_handles) {
    GlFunctions::DeletePerfQueryINTEL(free_query);
  }
  m_free_query_handles.clear();
  assert(m_extant_query_handles.empty());
  for (auto m : m_metrics)
    delete m.second;
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
PerfMetricGroup::begin(RenderId render) {
  if (m_free_query_handles.empty()) {
    GLuint query_handle;
    GlFunctions::CreatePerfQueryINTEL(m_query_id, &query_handle);
    m_free_query_handles.push_back(query_handle);
  }
  GLuint query_handle = m_free_query_handles.back();
  m_free_query_handles.pop_back();
  GlFunctions::BeginPerfQueryINTEL(query_handle);
  m_extant_query_handles[render] = query_handle;
}

void
PerfMetricGroup::publish(MetricId metric,
                         ExperimentId experimentCount,
                         SelectionId selectionCount,
                         OnFrameRetrace *callback) {
  const bool publish_all = ((metric() | ID_PREFIX_MASK) == -1);
  const int publish_count = m_extant_query_handles.size();
  std::map<MetricId, MetricSeries> out_data;
  if (publish_all) {
    for (auto i : m_metrics) {
      out_data[i.first].data.reserve(publish_count);
      out_data[i.first].metric = i.first;
    }
  } else {
    assert(m_metrics.find(metric) != m_metrics.end());
    out_data[metric].data.reserve(publish_count);
    out_data[metric].metric = metric;
  }

  for (auto extant_query : m_extant_query_handles) {
    memset(m_data_buf.data(), 0, m_data_buf.size());
    GLuint bytes_written;
    GlFunctions::GetPerfQueryDataINTEL(extant_query.second,
                                       GL_PERFQUERY_WAIT_INTEL,
                                       m_data_size, m_data_buf.data(),
                                       &bytes_written);
    assert(bytes_written == m_data_size);

    // TODO(majanes) verify order of m_extant_query_handles is by
    // RenderId
    for (auto desired_metric : out_data) {
      MetricId m = desired_metric.first;
      out_data[m].data.push_back(m_metrics[m]->getMetric(m_data_buf));
    }
    m_free_query_handles.push_back(extant_query.second);
  }
  m_extant_query_handles.clear();
  if (callback) {
    for (auto desired_metric : out_data) {
      callback->onMetrics(desired_metric.second,
                          experimentCount,
                          selectionCount);
    }
  }
  for (auto free_query : m_free_query_handles) {
    GlFunctions::DeletePerfQueryINTEL(free_query);
  }
  m_free_query_handles.clear();
}

void
PerfMetricGroup::end(RenderId render) {
  GlFunctions::EndPerfQueryINTEL(m_extant_query_handles[render]);
}


PerfMetric::PerfMetric(int query_id,
                       int counter_num) : m_query_id(query_id),
                                          m_counter_num(counter_num) {
  static GLint max_name_len = 0, max_desc_len = 0;
  if (max_name_len == 0)
    GlFunctions::GetIntegerv(GL_PERFQUERY_COUNTER_NAME_LENGTH_MAX_INTEL,
                             &max_name_len);
  if (max_desc_len == 0)
    GlFunctions::GetIntegerv(GL_PERFQUERY_COUNTER_DESC_LENGTH_MAX_INTEL,
                             &max_desc_len);
  std::vector<GLchar> counter_name(max_name_len);
  std::vector<GLchar> counter_description(max_desc_len);
  GLuint64 max_value;
  GlFunctions::GetPerfCounterInfoINTEL(m_query_id, m_counter_num,
                                       counter_name.size(), counter_name.data(),
                                       counter_description.size(),
                                       counter_description.data(),
                                       &m_offset, &m_data_size, &m_type,
                                       &m_data_type, &max_value);
  m_name = counter_name.data();
  m_description = counter_description.data();
}

MetricId
PerfMetric::id() const {
  return MetricId(m_query_id, m_counter_num);
}

const std::string &
PerfMetric::name() const {
  return m_name;
}

const std::string &
PerfMetric::description() const {
  return m_description;
}

float
PerfMetric::getMetric(const std::vector<unsigned char> &data) const {
  const unsigned char *p_value = data.data() + m_offset;
  float fval;
  switch (m_data_type) {
    case GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL: {
      assert(m_data_size == 4);
      const uint32_t val = *reinterpret_cast<const uint32_t *>(p_value);
      fval = static_cast<float>(val);
      break;
    }
    case GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL: {
      assert(m_data_size == 8);
      const uint64_t val = *reinterpret_cast<const uint64_t *>(p_value);
      fval = static_cast<float>(val);
      break;
    }
    case GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL: {
      assert(m_data_size == 4);
      fval = *reinterpret_cast<const float *>(p_value);
      break;
    }
    case GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL: {
      assert(m_data_size == 8);
      const double val = *reinterpret_cast<const double *>(p_value);
      fval = static_cast<float>(val);
      break;
    }
    case GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL: {
      assert(m_data_size == 4);
      const bool val = *reinterpret_cast<const bool*>(p_value);
      fval = val ? 1.0 : 0.0;
      break;
    }
    default:
      assert(false);
  }
  return fval;
}

void
PerfMetrics::selectMetric(MetricId metric) {
  assert(metric_map.find(metric) != metric_map.end());
  current_metric = metric;
  current_group = groups[metric_map[metric]];
}

void
PerfMetrics::publish(ExperimentId experimentCount,
                     SelectionId selectionCount,
                     OnFrameRetrace *callback) {
  current_group->publish(current_metric, experimentCount,
                         selectionCount, callback);
}

void
PerfMetrics::begin(RenderId render) {
  current_group->begin(render);
  current_render = render;
}

void
PerfMetrics::end() {
  current_group->end(current_render);
}

int
PerfMetrics::groupCount() const {
  return groups.size();
}

void
PerfMetrics::selectGroup(int index) {
  current_group = groups[index];
  // choose an invalid metric to represent "all metrics"
  current_metric = MetricId(~ID_PREFIX_MASK);
}

