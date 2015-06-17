/**************************************************************************
 *
 * Copyright 2011 Jose Fonseca
 * Copyright (C) 2013 Intel Corporation. All rights reversed.
 * Author: Shuang He <shuang.he@intel.com>
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
 **************************************************************************/


// This file was culled from retrace_main.cpp, which defines a bunch
// of global variables accessed throughout tracing.  IMHO this type of
// state should be encapsulated, but I'm not going to refactor all of
// retrace unless Jose agrees that it is a good thing to do.  For the
// time being, we will just imitate the global state that the command
// line application uses.

#include "trace_parser.hpp"
#include "trace_profiler.hpp"
#include "retrace.hpp"
#include "trace_callset.hpp"

static trace::CallSet snapshotFrequency;
static trace::ParseBookmark lastFrameStart;

retrace::Retracer retracer;

namespace retrace {

trace::Parser _parser;
trace::AbstractParser *parser = &_parser;
trace::Profiler profiler;


int verbosity = 0;
unsigned debug = 1;
bool dumpingState = false;
bool dumpingSnapshots = false;

Driver driver = DRIVER_DEFAULT;
const char *driverModule = NULL;

bool doubleBuffer = true;
unsigned samples = 1;

bool profiling = false;
bool profilingGpuTimes = false;
bool profilingCpuTimes = false;
bool profilingPixelsDrawn = false;
bool profilingMemoryUsage = false;
bool useCallNos = true;
bool singleThread = false;
bool markers = false;
bool profilingWithBackends = false;
bool profilingListMetrics = false;
bool curPas = false;
bool profilingNumPasses = false;
char *profilingCallsMetricsString = NULL;
char *profilingFramesMetricsString = NULL;
char *profilingDrawCallsMetricsString = NULL;
unsigned frameNo = 0;
unsigned callNo = 0;
unsigned int numPasses = 0;
unsigned int curPass = 0;

void
frameComplete(trace::Call &call) { // NOLINT -- can't change ApiTrace badness
}

class DefaultDumper: public Dumper {
 public:
  int
  getSnapshotCount(void) {
    return 1;
  }

  image::Image *
  getSnapshot(int num) {
    return NULL;
  }

  bool
  canDump(void) {
    return false;
  }

  void
  dumpState(StateWriter &writer) {  // NOLINT -- can't change ApiTrace badness
    assert(0);
  }
};

static DefaultDumper defaultDumper;

Dumper *dumper = &defaultDumper;


}  // namespace retrace

