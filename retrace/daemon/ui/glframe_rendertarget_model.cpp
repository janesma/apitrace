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

#include "glframe_os.hpp"
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
      m_highlight_render(false),
      m_sel(0), m_exp(0),
      m_option_count(0),
      m_index(0) {
  m_rts.push_back("image://myimageprovider/default.image.url");
}

void
QRenderTargetModel::onRenderTarget(SelectionId selectionCount,
                                   ExperimentId experimentCount,
                                   const std::vector<unsigned char> &data) {
  if (selectionCount > m_sel || experimentCount > m_exp) {
    m_sel = selectionCount;
    m_exp = experimentCount;
    m_index = 0;
    glretrace::FrameImages::instance()->Clear();
    m_rts.clear();
  }

  ++m_index;
  if (data.empty()) {
    // error case
    m_rts.push_back("image://myimageprovider/default_image.png");
    emit renderTargetsChanged();
    return;
  }

  {
    std::stringstream ss;
    ss << "image://myimageprovider/image_"
       << m_sel.count() << "_"
       << m_exp.count() << "_"
       << m_option_count << "_"
       << m_index << ".png";
    m_rts.push_back(ss.str().c_str());
  }

  {
    // QML requests path from ImageProvider without the full url
    std::stringstream ss;
    ss << "image_" << m_sel.count() << "_"
       << m_exp.count() << "_"
       << m_option_count << "_"
       << m_index << ".png";
    glretrace::FrameImages::instance()->AddImage(ss.str().c_str(), data);
  }
  emit renderTargetsChanged();
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

QStringList
QRenderTargetModel::renderTargetImages() const {
  return m_rts;
}

bool
QRenderTargetModel::clearBeforeRender() const {
  return m_clear_before_render;
}

void
QRenderTargetModel::setClearBeforeRender(bool v) {
  ++m_option_count;
  glretrace::FrameImages::instance()->Clear();
  m_rts.clear();
  m_clear_before_render = v;
  m_retrace->retraceRendertarget();
}

bool
QRenderTargetModel::stopAtRender() const {
  return m_stop_at_render;
}

void
QRenderTargetModel::setStopAtRender(bool v) {
  ++m_option_count;
  glretrace::FrameImages::instance()->Clear();
  m_rts.clear();
  m_stop_at_render = v;
  m_retrace->retraceRendertarget();
}

bool
QRenderTargetModel::highlightRender() const {
  return m_highlight_render;
}

void
QRenderTargetModel::setHighlightRender(bool v) {
  ++m_option_count;
  glretrace::FrameImages::instance()->Clear();
  m_rts.clear();
  m_highlight_render = v;
  m_retrace->retraceRendertarget();
}


