/**************************************************************************
 *
 * Copyright 2015 Intel Corporation
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
 **************************************************************************/


#include "glframe_qbargraph.hpp"

#include <QtOpenGL>
#include <QList>
#include <qtextstream.h>

#include <algorithm>
#include <set>
#include <vector>

#include "glframe_retrace_model.hpp"
#include "glframe_os.hpp"

using glretrace::BarGraphView;
using glretrace::BarMetrics;
using glretrace::FrameRetraceModel;
using glretrace::QBarGraphRenderer;
using glretrace::QSelection;
using glretrace::SelectionId;

QBarGraphRenderer::QBarGraphRenderer() : m_graph(true),
                                         selection(NULL),
                                         subscribed(false) {
  m_graph.subscribe(this);
}

void
QBarGraphRenderer::render() {
  m_graph.render();
}

void
QBarGraphRenderer::synchronize(QQuickFramebufferObject * item) {
  BarGraphView *v = reinterpret_cast<BarGraphView*>(item);
  if (!selection) {
    QSelection *s = v->getSelection();
    if (s) {
      selection = s;
      connect(s, &QSelection::onSelect,
              this, &QBarGraphRenderer::onSelect);
      connect(this, &QBarGraphRenderer::barSelect,
              s, &QSelection::select);
    }
  }

  if (!subscribed) {
    FrameRetraceModel *m = v->getModel();
    if (m) {
      m->subscribe(this);
      subscribed = true;
    } else {
      // for test
      subscribed = v->subscribeRandom(this);
    }
  }

  m_graph.setMouseArea(v->mouse_area[0],
                       v->mouse_area[1],
                       v->mouse_area[2],
                       v->mouse_area[3]);
  float zoom, zoom_translate;
  v->zoom(&zoom, &zoom_translate);
  m_graph.setZoom(zoom, zoom_translate);

  if (v->clicked) {
    m_graph.selectMouseArea(v->shift);
    m_graph.setMouseArea(0, 0, 0, 0);
    v->mouse_area[0] = -1.0;
    v->mouse_area[1] = -1.0;
    v->mouse_area[2] = -1.0;
    v->mouse_area[3] = -1.0;
    v->clicked = false;
    v->shift = false;
  }
}

void
QBarGraphRenderer::onBarSelect(const std::vector<int> selection) {
  QList<int> s;
  for (auto i : selection)
    s.append(i);
  emit barSelect(s);
}

void
QBarGraphRenderer::onSelect(SelectionId, QList<int> selection) {
  std::set<int> s;
  for (auto i : selection)
    s.insert(i);
  m_graph.setSelection(s);
}


QOpenGLFramebufferObject *
QBarGraphRenderer::createFramebufferObject(const QSize & size) {
  QOpenGLFramebufferObjectFormat format;
  format.setSamples(10);
  return new QOpenGLFramebufferObject(size, format);
}

void
QBarGraphRenderer::onMetrics(QList<BarMetrics> metrics) {
  std::vector<BarMetrics> m;
  for (auto metric : metrics) {
    m.push_back(metric);
  }
  m_graph.setBars(m);
  update();
}

QQuickFramebufferObject::Renderer *
BarGraphView::createRenderer() const {
  return new QBarGraphRenderer();
}

BarGraphView::BarGraphView() : mouse_area(4),
                               clicked(false), shift(false),
                               selection(NULL), model(NULL),
                               m_randomBars(0),
                               m_zoom(1.0),
                               m_translate(0.0) {
}

void
BarGraphView::mouseRelease(bool _shift) {
  clicked = true;
  shift = _shift;
  update();
}

void
BarGraphView::mouseDrag(float x1, float y1, float x2, float y2) {
  mouse_area[0] = std::min(x1, x2);
  mouse_area[1] = std::min(y1, y2);
  mouse_area[2] = std::max(x1, x2);
  mouse_area[3] = std::max(y1, y2);
  update();
}


QSelection *
BarGraphView::getSelection() {
  ScopedLock s(m_protect);
  return selection;
}
void
BarGraphView::setSelection(QSelection *s) {
  ScopedLock sl(m_protect);
  selection = s;
  update();
}

FrameRetraceModel *
BarGraphView::getModel() {
  ScopedLock s(m_protect);
  return model;
}

void
BarGraphView::setModel(FrameRetraceModel *m) {
  ScopedLock s(m_protect);
  model = m;
  update();
  emit onModel();
}

void
BarGraphView::setRandomBarCount(int count) {
  m_randomBars = count;
  update();
  emit onRandomBarCount();
}

bool
BarGraphView::subscribeRandom(QBarGraphRenderer *graph) {
  if (m_randomBars == 0)
    return false;
  QList<BarMetrics> metrics;
  for (int i = 0; i < m_randomBars; ++i) {
    metrics.append(BarMetrics());
    metrics.back().metric1 = qrand() % 100;
    metrics.back().metric2 = qrand() % 100;
  }
  graph->onMetrics(metrics);
  return true;
}

void
BarGraphView::mouseWheel(int degrees, float zoom_point_x) {
  float new_zoom = m_zoom + degrees / 360.0;
  if (new_zoom < 1.0) {
    new_zoom = 1.0;
  }

  float unzoomed_point = (zoom_point_x - m_translate) / m_zoom;
  float new_translate = zoom_point_x - unzoomed_point * new_zoom;
  // float new_translate = zoom_point_x * (1 - new_zoom);

  if (new_translate < 1 - new_zoom)
    new_translate = 1 - new_zoom;
  else if (new_translate > 0)
    new_translate = 0;

  m_translate = new_translate;
  m_zoom = new_zoom;
  emit zoomChanged();
  emit translateChanged();
  update();
}

void
BarGraphView::zoom(float *zoom, float *translate) const {
  *zoom = m_zoom;
  *translate = m_translate;
}

void
BarGraphView::setZoom(float z) {
  m_zoom = z;
  emit zoomChanged();
  update();
}

void
BarGraphView::setTranslate(float z) {
  m_translate = z;
  emit translateChanged();
  update();
}

