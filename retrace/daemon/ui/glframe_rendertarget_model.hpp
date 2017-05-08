/**************************************************************************
 *
 * Copyright 2017 Intel Corporation
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


#ifndef _GLFRAME_RENDERTARGET_MODEL_HPP_
#define _GLFRAME_RENDERTARGET_MODEL_HPP_

#include <QObject>

#include <vector>

#include "glframe_retrace_interface.hpp"

namespace glretrace {

class FrameRetraceModel;

class QRenderTargetModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(QStringList renderTargetImages READ renderTargetImages
             NOTIFY renderTargetsChanged)
  Q_PROPERTY(bool clearBeforeRender READ clearBeforeRender
             WRITE setClearBeforeRender)
  Q_PROPERTY(bool stopAtRender READ stopAtRender
             WRITE setStopAtRender)
  Q_PROPERTY(bool highlightRender READ highlightRender
             WRITE setHighlightRender)

 public:
  QRenderTargetModel();
  explicit QRenderTargetModel(FrameRetraceModel *retrace);
  void onRenderTarget(SelectionId selectionCount,
                      ExperimentId experimentCount,
                      const std::vector<unsigned char> &data);
  QStringList renderTargetImages() const;
  bool clearBeforeRender() const;
  void setClearBeforeRender(bool v);
  bool stopAtRender() const;
  void setStopAtRender(bool v);
  bool highlightRender() const;
  void setHighlightRender(bool v);
  RenderTargetType type();
  RenderOptions options();

 signals:
  void renderTargetsChanged();

 private:
  FrameRetraceModel *m_retrace;
  bool m_clear_before_render, m_stop_at_render, m_highlight_render;
  SelectionId m_sel;
  ExperimentId m_exp;
  int m_option_count;
  int m_index;
  QStringList m_rts;
};

}  // namespace glretrace

#endif  // _GLFRAME_RENDERTARGET_MODEL_HPP_

