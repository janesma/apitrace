/**************************************************************************
 *
 * Copyright 2019 Intel Corporation
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

#include "glframe_texture_model.hpp"

#include <string>
#include <vector>

#include "glframe_os.hpp"
#include "glframe_qutil.hpp"

using glretrace::QTextureModel;
using glretrace::RenderId;
using glretrace::ScopedLock;
using glretrace::SelectionId;

QTextureModel::QTextureModel() {}
QTextureModel::~QTextureModel() {}

QStringList
QTextureModel::renders() const {
  ScopedLock s(m_protect);
  return m_renders;
}

void
QTextureModel::onTexture(SelectionId selectionCount,
                         ExperimentId experimentCount,
                         RenderId renderId,
                         TextureKey binding,
                         const std::vector<TextureData> &images) {
  if (selectionCount == SelectionId(SelectionId::INVALID_SELECTION)) {
    for (auto i : m_texture_units) {
      m_render_index.push_back(i.first);
      m_renders.push_back(QString("%1").arg(i.first.index()));
    }
    // all textures received
    emit rendersChanged();
    setIndex(0);
    return;
  }

  {
    ScopedLock s(m_protect);
    if (m_sel_count != selectionCount || m_exp_count != experimentCount) {
      clear();
      m_sel_count = selectionCount;
      m_exp_count = experimentCount;
    }
  }

  if (m_texture_units.find(renderId) == m_texture_units.end()) {
    m_texture_units[renderId] = new QTextureUnits();
  }
}

void
QTextureModel::setIndex(int index) {
  {
    ScopedLock s(m_protect);
    m_index = index;
  }
  emit indexChanged();
}

void
QTextureModel::clear() {
  m_renders.clear();
  m_render_index.clear();
  for (auto i : m_texture_units)
    // possible use-after-delete here
    delete i.second;
  m_texture_units.clear();
}
