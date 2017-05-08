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
#ifndef GLFRAME_RETRACE_IMAGES_HPP__
#define GLFRAME_RETRACE_IMAGES_HPP__


#include <QObject>
#include <QImage>
#include <QQuickImageProvider>
#include <map>
#include <vector>

namespace glretrace {
class FrameImages : public QQuickImageProvider {
 public:
  static FrameImages *instance();
  static void Create();
  static void Destroy();
  virtual QImage requestImage(const QString &id,
                              QSize *size,
                              const QSize &requestedSize);
  void Clear();
  void AddImage(const char *path, const std::vector<unsigned char> &buf);
 private:
  FrameImages() : QQuickImageProvider(QQmlImageProviderBase::Image),
                  m_default(":/qml/images/no_render_target.png") {}
  QImage m_default;
  std::map<QString, QImage> m_rts;
  static FrameImages *m_instance;
};

}  // namespace glretrace

#endif
