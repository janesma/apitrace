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
#include <GL/gl.h>

#include <string>
#include <vector>

#include "glframe_os.hpp"
#include "glframe_retrace_render.hpp"

using glretrace::QStateModel;
using glretrace::QStateValue;
using glretrace::state_name_to_enum;

static const int kUninitializedValue = -2;
static const int kMixedValue = -1;

QStateValue::QStateValue(const std::string &_name,
                         const std::vector<std::string> &_choices)
    : m_name(_name.c_str()),
      m_value(kUninitializedValue) {
  for (auto c : _choices)
    m_choices.append(QVariant(c.c_str()));
}

void
QStateValue::insert(const std::string &value) {
  int value_index = 0;
  QVariant qvalue(value.c_str());
  for (auto c : m_choices) {
    if (qvalue == c)
      break;
    ++value_index;
  }
  // value must be found
  assert(value_index < m_choices.size());

  if (m_value == kUninitializedValue) {
    // selected renders have different values
    m_value = value_index;
    return;
  }

  if (m_value != value_index)
    // selected renders have different values
    m_value = kMixedValue;
}

QStateModel::QStateModel() {}

QStateModel::QStateModel(IFrameRetrace *retrace) : m_retrace(retrace) {}

QStateModel::~QStateModel() {}

QQmlListProperty<QStateValue> QStateModel::state() {
  ScopedLock s(m_protect);
  return QQmlListProperty<glretrace::QStateValue>(this, m_states);
}

std::vector<std::string>
name_to_choices(const std::string &n) {
  switch (state_name_to_enum(n)) {
    case GL_CULL_FACE:
      return {"true", "false"};
    case GL_CULL_FACE_MODE:
      return {"GL_FRONT", "GL_BACK", "GL_FRONT_AND_BACK"};
    case GL_INVALID_ENUM:
    default:
      assert(false);
  }
}

void
QStateModel::clear() {
  // QObjects being displayed in the UI must be cleared from the UI
  // before being deleted.  This routine is invoked in the UI thread
  // (as opposed to the retrace thread).  The emit with the empty
  // states calls down into ::state() to retrieve the new empty list
  // within a single thread.  After emit, it is safe to delete the
  // objects.  Conversely, if these objects were cleaned up after an
  // emit in the retrace thread, the request for updated state would
  // be enqueued.  The result is an asynchronous crash.
  {
    ScopedLock s(m_protect);
    m_states.clear();
  }
  emit stateChanged();
  {
    ScopedLock s(m_protect);
    for (auto i : m_state_by_name)
      delete i.second;
    m_state_by_name.clear();
    m_renders.clear();
  }
}

void QStateModel::onState(SelectionId selectionCount,
                          ExperimentId experimentCount,
                          RenderId renderId,
                          StateKey item,
                          const std::string &value) {
  if (selectionCount == SelectionId(SelectionId::INVALID_SELECTION)) {
    {
      ScopedLock s(m_protect);
      m_states.clear();
      for (auto i : m_state_by_name)
        m_states.push_back(i.second);
    }
    emit stateChanged();
    return;
  }

  if ((selectionCount > m_sel_count) ||
      (experimentCount > m_experiment_count))
    clear();

  ScopedLock s(m_protect);
  if ((selectionCount > m_sel_count) ||
      (experimentCount > m_experiment_count)) {
    m_sel_count = selectionCount;
    m_experiment_count = experimentCount;
  } else {
    assert(selectionCount == m_sel_count);
    assert(experimentCount == m_experiment_count);
  }
  if (m_renders.empty() || renderId != m_renders.back())
    m_renders.push_back(renderId);
  auto &name = item.name;
  auto state_value = m_state_by_name.find(name);
  if (state_value == m_state_by_name.end()) {
    QStateValue *i = new QStateValue(name,
                                     name_to_choices(name));
    m_state_by_name[name] = i;
    state_value = m_state_by_name.find(name);
  }
  state_value->second->insert(value);
}

void
QStateModel::setState(const QString &name,
                      const QString &value) {
  RenderSelection sel;
  sel.id = m_sel_count;
  auto r = m_renders.begin();
  sel.series.push_back(RenderSequence(*r, RenderId(r->index() + 1)));
  ++r;
  while (r != m_renders.end()) {
    if (*r == sel.series.back().end)
      ++sel.series.back().end;
    else
      sel.series.push_back(RenderSequence(*r, RenderId(r->index() + 1)));
    ++r;
  }

  StateKey key("", "", name.toStdString());
  m_retrace->setState(sel, key, value.toStdString());
  emit stateExperiment();
}

