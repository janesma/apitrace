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

#include "glframe_retrace_images.hpp"
#include <assert.h>

#include <vector>

using glretrace::FrameImages;

FrameImages * FrameImages::m_instance = NULL;

FrameImages *FrameImages::instance() {
    return m_instance;
}

void
FrameImages::Create() {
    assert(m_instance == NULL);
    m_instance = new FrameImages();
}

void
FrameImages::Destroy() {
    assert(m_instance != NULL);
    delete m_instance;
    m_instance = NULL;
}

void
FrameImages::AddImage(const char *path,
                      const std::vector<unsigned char> &buf) {
  QString qs(path);
  m_rts[qs].loadFromData(buf.data(), buf.size(), "PNG");
}

void
FrameImages::Clear() {
  m_rts.clear();
}

QImage
FrameImages::requestImage(const QString &id,
                          QSize *size,
                          const QSize &requestedSize) {
  if (m_rts.find(id) == m_rts.end())
    return m_default;
  return m_rts[id];
}
