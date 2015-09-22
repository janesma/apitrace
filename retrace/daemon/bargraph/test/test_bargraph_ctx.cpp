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

#include "test_bargraph_ctx.hpp"

#include <GL/gl.h>
#include <stdio.h>

#include "glframe_glhelper.hpp"

using glretrace::TestContext;

TestContext::TestContext() {
  const int32_t init_attrs[] = {
    WAFFLE_PLATFORM, WAFFLE_PLATFORM_GLX,
    0,
  };
  waffle_init(init_attrs);

  m_dpy = waffle_display_connect(NULL);
  const int32_t config_attrs[] = {
    WAFFLE_CONTEXT_API, WAFFLE_CONTEXT_OPENGL,
    0,
  };

  m_config = waffle_config_choose(m_dpy, config_attrs);
  m_window = waffle_window_create(m_config, 1000, 1000);
  m_ctx = waffle_context_create(m_config, NULL);
  GL_CHECK();
  waffle_make_current(m_dpy, m_window, m_ctx);
  GL_CHECK();

  waffle_window_show(m_window);
}

TestContext::~TestContext() {
  waffle_context_destroy(m_ctx);
  waffle_window_destroy(m_window);
  waffle_config_destroy(m_config);
  waffle_display_disconnect(m_dpy);
}


void
TestContext::swapBuffers() {
  waffle_window_swap_buffers(m_window);
}
