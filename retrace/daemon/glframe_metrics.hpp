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

#ifndef _GLFRAME_METRICS_HPP_
#define _GLFRAME_METRICS_HPP_

#include <map>
#include <vector>

#include "glframe_traits.hpp"
#include "glframe_retrace.hpp"

namespace glretrace {

class PerfMetricGroup;

class PerfMetrics : public NoCopy, NoAssign {
 public:
  explicit PerfMetrics(OnFrameRetrace *cb);
  ~PerfMetrics();
  int groupCount() const;
  void selectMetric(MetricId metric);
  void selectGroup(int index);
  void begin(RenderId render);
  void end();
  void publish(ExperimentId experimentCount,
               SelectionId selectionCount,
               OnFrameRetrace *callback);
 private:
  std::vector<PerfMetricGroup *> groups;
  // indicates offset in groups of PerfMetricGroup reporting MetricId
  std::map<MetricId, int> metric_map;
  // indicates the group that will handle subsequent begin/end calls
  PerfMetricGroup *current_group;
  MetricId current_metric;
  RenderId current_render;
};

}  // namespace glretrace

#endif /* _GLFRAME_METRICS_HPP__ */
