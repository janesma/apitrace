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

#include "glframe_batch_model.hpp"

#include <string>
#include <vector>

#include "glframe_os.hpp"
#include "glframe_qutil.hpp"

using glretrace::ExperimentId;
using glretrace::QBatchModel;
using glretrace::RenderId;
using glretrace::ScopedLock;
using glretrace::SelectionId;

QBatchModel::QBatchModel() : m_sel_count(0), m_index(-1) {
}

QBatchModel::~QBatchModel() {
    m_batch.clear();
}

QString
QBatchModel::batch() {
  ScopedLock s(m_protect);
  if (m_index < 0)
    return QString("Mesa has the ability to display batches dispatched "
                   "to the GPU."
                   "\nSee apitrace/retrace/daemon/doc/README.txt");
  if (m_index >= m_renders.size())
    return QString("");
  return m_batch[m_renders[m_index]];
}

void
QBatchModel::onBatch(SelectionId selectionCount,
                 RenderId renderId,
                 const std::string &batch) {
  {
    ScopedLock s(m_protect);
    if (m_sel_count != selectionCount) {
      m_batch.clear();
      m_renders.clear();
      m_sel_count = selectionCount;
    }

    m_renders.push_back(QString("%1").arg(renderId.index()));
    m_batch[m_renders.back()] = QString(batch.c_str());
  }
  setIndex(0);
  emit onRenders();
}

void
QBatchModel::setIndex(int index) {
  {
    ScopedLock s(m_protect);
    m_index = index;
  }
  emit onBatchChanged();
}

QStringList
QBatchModel::renders() const {
  ScopedLock s(m_protect);
  return m_renders;
}

