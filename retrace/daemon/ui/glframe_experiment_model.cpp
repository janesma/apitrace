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

#include <map>

#include "glframe_qutil.hpp"

using glretrace::IFrameRetrace;
using glretrace::QExperimentModel;
using glretrace::RenderId;
using glretrace::SelectionId;
using Qt::Checked;
using Qt::Unchecked;
using Qt::PartiallyChecked;
using Qt::CheckState;

QExperimentModel::QExperimentModel()
    : m_retrace(NULL), m_disabled_checkbox(Qt::Unchecked),
      m_simple_checkbox(Unchecked) {
  assert(false);
}

QExperimentModel::QExperimentModel(IFrameRetrace *retrace)
    : m_retrace(retrace),
      m_disabled_checkbox(Unchecked),
      m_simple_checkbox(Unchecked) {}

CheckState
isChecked(std::map<RenderId, bool> *_renders,
          const QList<int> &selection) {
  // if selection has both checked and unchecked experiments, return
  // PartiallyChecked.
  auto renders = *_renders;
  CheckState new_value = CheckState(-1);
  for (auto sel : selection) {
    const bool checked = renders[RenderId(sel)];
    if (checked) {
      if (new_value == Unchecked) {
        return PartiallyChecked;
      } else {
        new_value = Checked;
      }
    } else {  // not checked
      if (new_value == Checked) {
        return PartiallyChecked;
      } else {
        new_value = Unchecked;
      }
    }
  }
  return new_value;
}

void
QExperimentModel::onSelect(SelectionId count, const QList<int> &selection) {
  m_selection = selection;
  m_count = count;
  m_disabled_checkbox = isChecked(&m_disabled, selection);
  m_simple_checkbox = isChecked(&m_simple, selection);
  emit onDisabled();
  emit onSimpleShader();
}

CheckState uncheckPartials(CheckState c) {
  assert(c != PartiallyChecked);
  // treat PartiallyChecked as Unchecked
  if (c == Checked)
    return Checked;
  return Unchecked;
}

void
QExperimentModel::disableDraw(CheckState disable) {
  m_disabled_checkbox = uncheckPartials(disable);
  for (auto render : m_selection)
    m_disabled[RenderId(render)] = m_disabled_checkbox;
  RenderSelection sel;
  glretrace::renderSelectionFromList(m_count,
                                     m_selection,
                                     &sel);
  m_retrace->disableDraw(sel, (m_disabled_checkbox == Checked));
  emit onDisabled();
  emit onExperiment();
}

void
QExperimentModel::simpleShader(CheckState simple) {
  m_simple_checkbox = uncheckPartials(simple);
  for (auto render : m_selection)
    m_simple[RenderId(render)] = m_simple_checkbox;
  RenderSelection sel;
  glretrace::renderSelectionFromList(m_count,
                                     m_selection,
                                     &sel);
  m_retrace->simpleShader(sel, (m_simple_checkbox == Checked));
  emit onSimpleShader();
  emit onExperiment();
}
