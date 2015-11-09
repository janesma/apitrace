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

QBarGraphRenderer::QBarGraphRenderer() : m_graph(true),
                                         selection(NULL),
                                         model(NULL) {
  m_graph.subscribe(this);
  std::vector<BarMetrics> metrics(4);
  metrics[0].metric1 = 1;
  metrics[0].metric2 = 1;
  metrics[1].metric1 = 2;
  metrics[1].metric2 = 2;
  metrics[2].metric1 = 1;
  metrics[2].metric2 = 1;
  metrics[3].metric1 = 2;
  metrics[3].metric2 = 2;
  m_graph.setBars(metrics);
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

  if (!model) {
    FrameRetraceModel *m = v->getModel();
    if (m) {
      model = m;
      m->subscribe(this);
    }
  }

  m_graph.setMouseArea(v->mouse_area[0],
                       v->mouse_area[1],
                       v->mouse_area[2],
                       v->mouse_area[3]);
  if (v->clicked) {
    m_graph.selectMouseArea();
    m_graph.setMouseArea(0, 0, 0, 0);
    v->clicked = false;
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
QBarGraphRenderer::onSelect(QList<int> selection) {
  std::set<int> s;
  for (auto i : selection)
    s.insert(i);
  m_graph.setSelection(s);
}


QOpenGLFramebufferObject *
QBarGraphRenderer::createFramebufferObject(const QSize & size) {
  QOpenGLFramebufferObjectFormat format;
  format.setSamples(20);
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

BarGraphView::BarGraphView() : mouse_area(4), clicked(false),
                               selection(NULL), model(NULL) {
}

void
BarGraphView::mouseRelease() {
  clicked = true;
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
