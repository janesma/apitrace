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

#include "glframe_gpu_speed.hpp"

#include <stdio.h>
#include <stdlib.h>

#include <vector>
#include <sstream>

#include "glframe_retrace_interface.hpp"

using glretrace::check_gpu_speed;
using glretrace::OnFrameRetrace;

void
glretrace::check_gpu_speed(OnFrameRetrace *callback) {
  FILE * fh = fopen("/sys/class/drm/card0/gt_max_freq_mhz", "r");
  std::vector<unsigned char> buf(100);
  size_t bytes = fread(buf.data(), 1, 99, fh);
  buf[bytes] = '\0';
  const int max_rate = atoi(reinterpret_cast<char*>(buf.data()));
  fclose(fh);
  fh = fopen("/sys/class/drm/card0/gt_min_freq_mhz", "r");
  bytes = fread(buf.data(), 1, 99, fh);
  buf[bytes] = '\0';
  const int min_rate = atoi(reinterpret_cast<char*>(buf.data()));
  fclose(fh);

  if (min_rate == max_rate)
    return;

  std::stringstream msg;
  msg << "The target platform gpu clock rate is not fixed.\n" <<
      "To stabilize metrics, execute as root:" <<
      "  `cat /sys/class/drm/card0/gt_max_freq_mhz > " <<
      "/sys/class/drm/card0/gt_min_freq_mhz`";
  callback->onError(RETRACE_WARN, msg.str());
}

