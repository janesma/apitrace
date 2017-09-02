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

#include "glframe_state_model.hpp"

#include <string>
#include <vector>

#include "glframe_os.hpp"

using glretrace::QStateModel;
using glretrace::QStateValue;
using glretrace::StateItem;

QStateValue::QStateValue(const std::string &_name,
                         const std::vector<std::string> &_choices)
    : m_name(_name.c_str()) {
  for (auto c : _choices)
    m_choices.append(QString::fromStdString(c));
}

void
QStateValue::insert(int index, const std::string &value) {
  while (m_values.size() < index)
    m_values.append(QString());
  if (m_values.size() == index)
    m_values.append(QString::fromStdString(value));
  else
    m_values[index] = QString::fromStdString(value);
}

QStateModel::QStateModel() {}

QStateModel::QStateModel(IFrameRetrace *retrace) {}

QStateModel::~QStateModel() {}

QQmlListProperty<QStateValue> QStateModel::state() {
  ScopedLock s(m_protect);
  QList<QStateValue*> l;
  for (auto i : m_state_by_name)
    l.push_back(i.second);

  return QQmlListProperty<glretrace::QStateValue>(this, l);
}

std::string
state_name_to_string(StateItem n) {
  switch (n) {
    case glretrace::CULL_FACE:
      return std::string("CULL_FACE");
    case glretrace::CULL_FACE_MODE:
      return std::string("CULL_FACE_MODE");
    default:
      assert(false);
  }
}

std::vector<std::string>
name_to_choines(StateItem n) {
  switch (n) {
    case glretrace::CULL_FACE:
      return {"true", "false"};
    case glretrace::CULL_FACE_MODE:
      return {"GL_FRONT", "GL_BACK", "GL_FRONT_AND_BACK"};
    default:
      assert(false);
  }
}

void QStateModel::onState(SelectionId selectionCount,
                          ExperimentId experimentCount,
                          RenderId renderId,
                          StateKey item,
                          const std::string &value) {
  ScopedLock s(m_protect);
  if ((selectionCount > m_sel_count) ||
      (experimentCount > m_experiment_count)) {
    m_sel_count = selectionCount;
    m_experiment_count = experimentCount;
  } else {
    assert(selectionCount == m_sel_count);
    assert(experimentCount == m_experiment_count);
  }
  if (selectionCount == SelectionId(SelectionId::INVALID_SELECTION)) {
    emit stateChanged();
    return;
  }
  const auto name = state_name_to_string(item.name);
  auto state_value = m_state_by_name.find(name);
  if (state_value == m_state_by_name.end()) {
    QStateValue *i = new QStateValue(name,
                                     name_to_choines(item.name));
    m_state_by_name[name] = i;
    state_value = m_state_by_name.find(name);
  }
  state_value->second->insert(item.index, value);
}

void
QStateModel::setState(const QString &name,
                      const int index,
                      const QString &value) {
  assert(false);
}

