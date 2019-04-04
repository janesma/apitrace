/**************************************************************************
 *
 * Copyright 2019 Intel Corporation
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
 * Authors:
 *   Mark Janes <mark.a.janes@intel.com>
 **************************************************************************/

#ifndef _GLFRAME_CANCELLATION_HPP_
#define _GLFRAME_CANCELLATION_HPP_

#include <mutex>

#include "glframe_retrace_interface.hpp"
#include "glframe_traits.hpp"

namespace glretrace {

class CancellationPolicy : NoCopy, NoAssign {
 public:
  bool isCancelled(SelectionId selectionCount,
                   ExperimentId experimentCount) const;
  void cancel(SelectionId selectionCount,
              ExperimentId experimentCount);

 private:
  mutable std::mutex m_protect;
  SelectionId m_current_sel;
  ExperimentId m_current_exp;
};

}  // end namespace glretrace

#endif  // _GLFRAME_CANCELLATION_HPP_
