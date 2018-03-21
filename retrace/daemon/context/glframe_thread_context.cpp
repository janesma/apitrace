/**************************************************************************
 *
 * Copyright 2018 Intel Corporation
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

#include "glframe_thread_context.hpp"
#include "trace_model.hpp"
#include "glframe_retrace_render.hpp"

using glretrace::ThreadContext;

extern retrace::Retracer retracer;

ThreadContext::~ThreadContext() {
  for (auto i : m_thread_context_switch)
    delete i.second;
  m_thread_context_switch.clear();
}

void
ThreadContext::track(trace::Call *c,
                     bool *is_owned_by_thread_tracker) {
  if (c->thread_id != m_current_thread) {
    m_current_thread = c->thread_id;
    auto context_switch = m_thread_context_switch.find(m_current_thread);
    if (context_switch != m_thread_context_switch.end())
      retracer.retrace(*(context_switch->second));
  }

  *is_owned_by_thread_tracker = changesContext(*c);
  if (!*is_owned_by_thread_tracker) {
    return;
  }

  auto last_context_cmd = m_thread_context_switch.find(m_current_thread);
  if (last_context_cmd != m_thread_context_switch.end())
    delete last_context_cmd->second;
  m_thread_context_switch[m_current_thread] = c;
}

bool
ThreadContext::changesContext(const trace::Call &call) {
  if (strncmp(call.name(), "glXMakeCurrent", strlen("glXMakeCurrent")) == 0)
    return true;
  if (strncmp(call.name(), "glXMakeContextCurrent",
              strlen("glXMakeContextCurrent")) == 0)
    return true;
  return false;
}
