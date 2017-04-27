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

#ifndef RETRACE_DAEMON_BARGRAPH_GLFRAME_QBARGRAPH_H_
#define RETRACE_DAEMON_BARGRAPH_GLFRAME_QBARGRAPH_H_

#include <QtQuick/QQuickFramebufferObject>

#include <mutex>
#include <vector>

#include "glframe_bargraph.hpp"
#include "glframe_qselection.hpp"
#include "glframe_traits.hpp"

namespace glretrace {

class FrameRetraceModel;
class QBarGraphRenderer : public QObject,
                          public QQuickFramebufferObject::Renderer,
                          public glretrace::BarGraphSubscriber,
                          NoCopy, NoAssign, NoMove {
  Q_OBJECT
 public:
  QBarGraphRenderer();
  void render();
  void synchronize(QQuickFramebufferObject * item);
  void onBarSelect(const std::vector<int> selection);
  // to ensure that we get a multisample fbo
  QOpenGLFramebufferObject * createFramebufferObject(const QSize & size);
 public slots:
  void onSelect(glretrace::SelectionId id, QList<int> selection);
  void onMetrics(QList<BarMetrics> metrics);
 signals:
  void barSelect(QList<int> selection);
 private:
  BarGraphRenderer m_graph;
  std::vector<int> current_selection;
  QSelection *selection;
  bool subscribed;
};

// exposes qml properties and signals to integrate the bar graph into
// the application.
class BarGraphView : public QQuickFramebufferObject,
                     NoCopy, NoAssign, NoMove {
  Q_OBJECT
  Q_PROPERTY(glretrace::QSelection* selection
             READ getSelection WRITE setSelection)
  Q_PROPERTY(glretrace::FrameRetraceModel* model
             READ getModel WRITE setModel NOTIFY onModel)
  Q_PROPERTY(int randomBarCount
             READ randomBarCount
             WRITE setRandomBarCount
             NOTIFY onRandomBarCount)
  Q_PROPERTY(float zoom
             READ getZoom
             WRITE setZoom
             NOTIFY zoomChanged)
  Q_PROPERTY(float translate
             READ getTranslate
             WRITE setTranslate
             NOTIFY translateChanged)

 public:
  BarGraphView();
  QQuickFramebufferObject::Renderer *createRenderer() const;
  Q_INVOKABLE void mouseRelease(bool shift);
  Q_INVOKABLE void mouseDrag(float x1, float y1, float x2, float y2);
  Q_INVOKABLE void mouseWheel(int degrees, float zoom_point_x);

  bool subscribeRandom(QBarGraphRenderer *graph);

  QSelection *getSelection();
  void setSelection(QSelection *s);

  FrameRetraceModel *getModel();
  void setModel(FrameRetraceModel *m);

  int randomBarCount() const {return m_randomBars;}
  void setRandomBarCount(int count);

  float getZoom() const { return m_zoom; }
  void setZoom(float z);

  float getTranslate() const { return m_translate; }
  void setTranslate(float z);

  void zoom(float *zoom, float *translate) const;

  std::vector<float> mouse_area;
  bool clicked;
  bool shift;
 signals:
  void onModel();
  void onRandomBarCount();
  void zoomChanged();
  void translateChanged();

 private:
  mutable std::mutex m_protect;
  QSelection *selection;
  FrameRetraceModel *model;
  int m_randomBars;
  float m_zoom;  // range [1.0..]
  float m_translate;  // after zoom, translate to keep zoom point in
                      // place [0..1.0] coordinate system
};

}  // namespace glretrace

#endif  // RETRACE_DAEMON_BARGRAPH_GLFRAME_QBARGRAPH_H_
