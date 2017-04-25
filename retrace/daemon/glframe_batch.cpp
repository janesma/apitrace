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

#include "glframe_batch.hpp"

#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>

#include <sstream>

using glretrace::MesaBatch;

// This class is a proof-of-concept.  If it sticks, the path
// resolution needs to be improved.
MesaBatch::MesaBatch() : m_debug(NULL) {
  // TODO(majanes) handle missing path
  // TODO(majanes) handle colon-separated path
  const char* driver_path = getenv("LIBGL_DRIVERS_PATH");
  if (!driver_path)
    return;
  std::stringstream p;
  p << driver_path << "/" << "i965_dri.so";
  void *lib = dlopen(p.str().c_str(),
                     RTLD_NOW);
  if (!lib)
    return;
  m_debug = reinterpret_cast<uint64_t*>(dlsym(lib, "INTEL_DEBUG"));
}

bool
MesaBatch::batchSupported() const {
  return (m_debug != NULL);
}

#define DEBUG_BATCH               (1ull <<  6)

void
MesaBatch::batchOn() {
  (*m_debug) |= DEBUG_BATCH;
}

void
MesaBatch::batchOff() {
  (*m_debug) &= ~DEBUG_BATCH;
}

