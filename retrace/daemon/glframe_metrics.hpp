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
#include "glframe_retrace_interface.hpp"

namespace glretrace {

class PerfMetricsContext;
struct Context;

class PerfMetrics {
 public:
  // Constructor accepts OnFrameRetrace callback pointer, and calls
  // onMetricList before returning.
  static PerfMetrics *Create(OnFrameRetrace *callback);
  virtual ~PerfMetrics() {}

  // Indicates the number of passes a metrics retrace must make to
  // collect the full metric set
  virtual int groupCount() const = 0;

  // Subsequent begin/end will collect data for all counters in the group
  virtual void selectGroup(int index) = 0;

  // Subsequent begin/end will collect data for the metric associated
  // with the id
  virtual void selectMetric(MetricId metric) = 0;

  // Begin collection for selected metrics.  When reported, the
  // counter values will be associated with the specified render.
  virtual void begin(RenderId render) = 0;

  // End collection for the selected metrics.
  virtual void end() = 0;

  // Flush and call onMetrics, providing all queried metric data.
  virtual void publish(ExperimentId experimentCount,
               SelectionId selectionCount,
               OnFrameRetrace *callback) = 0;

  // Call before changing to another context
  virtual void endContext() = 0;
  virtual void beginContext() = 0;
};

class DummyMetrics : public PerfMetrics {
 public:
  ~DummyMetrics() {}
  virtual int groupCount() const { return 0; }
  virtual void selectGroup(int index) {}
  virtual void selectMetric(MetricId metric) {}
  virtual void begin(RenderId render) {}
  virtual void end() {}
  virtual void publish(ExperimentId experimentCount,
               SelectionId selectionCount,
               OnFrameRetrace *callback) {}
  virtual void endContext() {}
  virtual void beginContext() {}
};

}  // namespace glretrace

#endif /* _GLFRAME_METRICS_HPP__ */
