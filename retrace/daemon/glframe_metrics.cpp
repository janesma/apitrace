// Copyright (C) Intel Corp.  2017.  All Rights Reserved.

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

#include <string.h>
#include <string>

#include "glframe_metrics_intel.hpp"
#include "glframe_metrics_amd.hpp"
#include "glframe_glhelper.hpp"
#include "glframe_metrics_amd_gpa.hpp"

using glretrace::PerfMetrics;
using glretrace::OnFrameRetrace;

PerfMetrics *PerfMetrics::Create(OnFrameRetrace *callback) {
  std::string extensions;

  const GLubyte *renderer = GlFunctions::GetString(GL_RENDERER);
  if (strstr((const char*)renderer, "AMD") != NULL)
      return new glretrace::PerfMetricsAMDGPA(callback);

  GlFunctions::GetGlExtensions(&extensions);
  if (extensions.find("GL_AMD_performance_monitor") != std::string::npos)
    return new PerfMetricsAMD(callback);
  if (extensions.find("GL_INTEL_performance_query") != std::string::npos)
    return new PerfMetricsIntel(callback);

  return new glretrace::DummyMetrics();
}
