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

using glretrace::PerfMetricGroup;
using glretrace::GlFunctions;
using glretrace::PerfMetrics;
using glretrace::MetricId;
using glretrace::NoCopy;
using glretrace::NoAssign;

namespace {

struct MetricDescription {
  MetricId id;
  std::string name;
  MetricDescription() {}
  MetricDescription(MetricId i, const std::string &n)
      : id(i), name(n) {}
};

class PerfMetric : public NoCopy, NoAssign {
 public:
  PerfMetric(int query_id, int counter_num);
  ~PerfMetric() { }
  MetricId id() const;
  const std::string &name() const;
  float getMetric(const std::vector<unsigned char> &data);
 private:
  const int m_query_id, m_counter_num;
  GLuint m_offset, m_data_size, m_type,
    m_data_type;
  std::string m_name, m_description;
  std::vector<float> m_data;
};

}  // namespace

class PerfMetricGroup : public NoCopy, NoAssign {
 public:
  explicit PerfMetricGroup(int query_id);
  ~PerfMetricGroup();
  void metrics(std::vector<MetricDescription> *m) const;
  void begin(RenderId render);
  void end(RenderId render);
  void publish(MetricId metric, ExperimentId experimentCount,
               OnFrameRetrace *callback);

 private:
  const std::string m_query_name;
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

PerfMetrics::PerfMetrics(OnFrameRetrace *cb) {
  GLuint query_id;
  GlFunctions::GetFirstPerfQueryIdINTEL(&query_id);
  if (query_id == -1)
    return;

  assert(query_id < 256);
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

  std::map<std::string, MetricId> known_metrics;

  std::vector<MetricDescription> metrics;
  for (auto i : query_ids) {
    PerfMetricGroup *g = new PerfMetricGroup(i);
    groups.push_back(g);
    metrics.clear();
    g->metrics(&metrics);
    for (auto &d : metrics) {
      if (known_metrics.find(d.name) == known_metrics.end())
        known_metrics[d.name] = d.id;
    }
  }
  std::vector<MetricId> ids;
  std::vector<std::string> names;
  for (auto &i : known_metrics) {
    names.push_back(i.first);
    ids.push_back(i.second);
  }
  cb->onMetricList(ids, names);
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
  for (unsigned int counter_num = 1; counter_num <= number_counters;
       ++counter_num) {
    PerfMetric *p = new PerfMetric(m_query_id, counter_num);
    m_metrics[p->id()] = p;
  }
}

void
PerfMetricGroup::metrics(std::vector<MetricDescription> *m) const {
  for (auto &i : m_metrics) {
    m->push_back(MetricDescription(i.first, i.second->name()));
  }
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
  return MetricId(m_query_id << 16 | m_counter_num);
}

const std::string &
PerfMetric::name() const {
  return m_name;
}
