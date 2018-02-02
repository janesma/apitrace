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

#ifndef _GLFRAME_METRICS_AMD_GPA_HPP_
#define _GLFRAME_METRICS_AMD_GPA_HPP_

#include <map>
#include <vector>

#include "glframe_metrics.hpp"
#include "glframe_traits.hpp"
#include "glframe_retrace.hpp"

namespace glretrace {

class PerfContext;
class PerfMetricsContextAMDGPA;
struct Context;

class PerfMetricsAMDGPA : public PerfMetrics, NoCopy, NoAssign {
 public:
  explicit PerfMetricsAMDGPA(OnFrameRetrace *cb);
  ~PerfMetricsAMDGPA();
  int groupCount() const;
  void selectMetric(MetricId metric);
  void selectGroup(int index);
  void begin(RenderId render);
  void end();
  void publish(ExperimentId experimentCount,
               SelectionId selectionCount,
               OnFrameRetrace *callback);
  void endContext();
  void beginContext();
  typedef std::map<MetricId, std::map<RenderId, float>> MetricMap;

 private:
  std::map<Context *, PerfContext*> m_contexts;
  PerfContext *m_current_context;
  // PerfMetricsContextAMDGPA* m_current_context;
  // std::map<Context*, PerfMetricsContextAMDGPA*> m_contexts;
  MetricMap m_data;
  int m_current_group;
  MetricId m_current_metric;
  uint32_t m_session_id;
};

}  // namespace glretrace

#endif /* _GLFRAME_METRICS_AMD_GPA_HPP_ */
