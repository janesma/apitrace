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

#include "glframe_experiment_model.hpp"

#include "glframe_qutil.hpp"

using glretrace::IFrameRetrace;
using glretrace::QExperimentModel;
using glretrace::SelectionId;

QExperimentModel::QExperimentModel()
    : m_retrace(NULL), m_checkbox(Qt::Unchecked) {
  assert(false);
}

QExperimentModel::QExperimentModel(IFrameRetrace *retrace)
    : m_retrace(retrace),
      m_checkbox(Qt::Unchecked) {}

void
QExperimentModel::onSelect(SelectionId count, QList<int> selection) {
  m_selection = selection;
  m_count = count;
  Qt::CheckState new_value = Qt::CheckState(-1);
  for (auto sel : selection) {
    const bool disabled = m_disabled[RenderId(sel)];
    if (disabled) {
      if (new_value == Qt::Unchecked) {
        new_value = Qt::PartiallyChecked;
        break;
      } else {
        new_value = Qt::Checked;
      }
    } else {  // not disabled
      if (new_value == Qt::Checked) {
        new_value = Qt::PartiallyChecked;
        break;
      } else {
        new_value = Qt::Unchecked;
      }
    }
  }
  m_checkbox = new_value;
  emit onDisabled();
}

void
QExperimentModel::disableDraw(Qt::CheckState disable) {
  if (disable != Qt::Unchecked)
    m_checkbox = Qt::Checked;
  else
    m_checkbox = Qt::Unchecked;
  for (auto render : m_selection)
    m_disabled[RenderId(render)] = (disable == Qt::Checked);
  RenderSelection sel;
  glretrace::renderSelectionFromList(m_count,
                                     m_selection,
                                     &sel);
  m_retrace->disableDraw(sel, (m_checkbox == Qt::Checked));
  emit onDisabled();
  emit onExperiment();
}
