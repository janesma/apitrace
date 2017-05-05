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

#include "glframe_rendertarget_model.hpp"

#include <sstream>
#include <vector>

#include "glframe_retrace_images.hpp"
#include "glframe_retrace_model.hpp"

using glretrace::ExperimentId;
using glretrace::FrameRetraceModel;
using glretrace::QRenderTargetModel;
using glretrace::RenderOptions;
using glretrace::RenderTargetType;
using glretrace::SelectionId;

QRenderTargetModel::QRenderTargetModel() {}

QRenderTargetModel::QRenderTargetModel(FrameRetraceModel *retrace)
    : m_retrace(retrace),
      m_clear_before_render(false),
      m_stop_at_render(false),
      m_highlight_render(false)
{}

void
QRenderTargetModel::onRenderTarget(SelectionId selectionCount,
                                  ExperimentId experimentCount,
                                  const std::vector<unsigned char> &data) {
  glretrace::FrameImages::instance()->SetImage(data);
  emit onRenderTarget();
}


RenderOptions
QRenderTargetModel::options() {
  RenderOptions opt = DEFAULT_RENDER;
  if (m_clear_before_render)
    opt = (RenderOptions) (opt | CLEAR_BEFORE_RENDER);
  if (m_stop_at_render)
    opt = (RenderOptions) (opt | STOP_AT_RENDER);
  return opt;
}


RenderTargetType
QRenderTargetModel::type() {
  if (m_highlight_render)
    return HIGHLIGHT_RENDER;
  return NORMAL_RENDER;
}

QString
QRenderTargetModel::renderTargetImage() const {
  static int i = 0;
  std::stringstream ss;
  ss << "image://myimageprovider/image" << ++i << ".png";
  return ss.str().c_str();
}

bool
QRenderTargetModel::clearBeforeRender() const {
  return m_clear_before_render;
}

void
QRenderTargetModel::setClearBeforeRender(bool v) {
  m_clear_before_render = v;
  m_retrace->retraceRendertarget();
}

bool
QRenderTargetModel::stopAtRender() const {
  return m_stop_at_render;
}

void
QRenderTargetModel::setStopAtRender(bool v) {
  m_stop_at_render = v;
  m_retrace->retraceRendertarget();
}

bool
QRenderTargetModel::highlightRender() const {
  return m_highlight_render;
}

void
QRenderTargetModel::setHighlightRender(bool v) {
  m_highlight_render = v;
  m_retrace->retraceRendertarget();
}


