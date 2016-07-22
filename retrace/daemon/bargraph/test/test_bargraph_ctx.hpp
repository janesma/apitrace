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

#ifndef RETRACE_DAEMON_BARGRAPH_TEST_TEST_BARGRAPH_CTX_H_
#define RETRACE_DAEMON_BARGRAPH_TEST_TEST_BARGRAPH_CTX_H_

#ifndef WIN32
#include <waffle-1/waffle.h>
#endif
#include <GLES2/gl2.h>

namespace glretrace {
class TestContext {
 public:
  TestContext();
  ~TestContext();
  void swapBuffers();

 private:
#ifndef WIN32
  struct waffle_display *m_dpy;
  struct waffle_config *m_config;
  struct waffle_window *m_window;
  struct waffle_context *m_ctx;
#endif
  // GLuint vbo, prog, texture;
  // GLint attribute_coord2d, tex_uniform;
};
}  // namespace glretrace
#endif  // RETRACE_DAEMON_BARGRAPH_TEST_TEST_BARGRAPH_CTX_H_
