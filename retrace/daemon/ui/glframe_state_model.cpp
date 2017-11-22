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

#include <algorithm>
#include <string>
#include <vector>

#include "glframe_os.hpp"
#include "glframe_retrace_render.hpp"
#include "glframe_state_enums.hpp"

using glretrace::QStateModel;
using glretrace::QStateValue;
using glretrace::state_name_to_enum;

static const int kUninitializedValue = -2;
static const int kMixedValue = -1;

QStateValue::QStateValue(QObject *parent) {
  if (parent)
    moveToThread(parent->thread());
}

QStateValue::QStateValue(QObject *parent,
                         const std::string &_group,
                         const std::string &_path,
                         const std::string &_name,
                         const std::vector<std::string> &_choices)
    : m_group(_group.c_str()),
      m_path(_path.c_str()),
      m_name(_name.c_str()),
      m_value(kUninitializedValue),
      m_visible(true),
      m_type(QStateValue::KglDirectory) {
  moveToThread(parent->thread());
  if (!_choices.empty())
    m_type = QStateValue::KglEnum;

  for (auto c : _choices)
    m_choices.append(QVariant(c.c_str()));
  const int path_count = std::count(_path.begin(), _path.end(), '/');
  const int indent =  path_count + (_name.length() > 0 ? 1 : 0);
  m_indent = indent;
  if (_name.length() == 0)
    m_name = _path.substr(_path.find_last_of("/") + 1).c_str();
}

void
QStateValue::insert(const std::string &value) {
  int value_index = 0;
  QVariant qvalue(value.c_str());
  if (m_type == QStateValue::KglEnum) {
    for (auto c : m_choices) {
      if (qvalue == c)
        break;
      ++value_index;
    }
    // value must be found
    assert(value_index < m_choices.size());
    if (m_value == kUninitializedValue) {
      // first render, display the value
      m_value = value_index;
      return;
    }

    if (m_value != value_index)
      // selected renders have different values
      m_value = kMixedValue;

    return;
  }

  // else
  m_type = QStateValue::KglFloat;
  const QString new_val = QString::fromStdString(value);
  if (m_value == kUninitializedValue) {
    m_value = new_val;
    return;
  }
  if (m_value != new_val)
    m_value = "###";
}

void
QStateValue::insert(const std::string &red,
                    const std::string &blue,
                    const std::string &green,
                    const std::string &alpha) {
  m_type = QStateValue::KglColor;
  QStringList color;
  color.append(QString::fromStdString(red));
  color.append(QString::fromStdString(blue));
  color.append(QString::fromStdString(green));
  color.append(QString::fromStdString(alpha));

  if (m_value == kUninitializedValue) {
    // selected renders have different values
    m_value = color;
    return;
  }

  if (m_value != color) {
    // selected renders have different values
    QStringList tmp = m_value.value<QStringList>();
    for (int i = 0; i < 4; ++i) {
      if (tmp[i] != color[i])
        tmp[i] = "###";
    }
    m_value = tmp;
  }
}

QStateModel::QStateModel() {}

QStateModel::QStateModel(IFrameRetrace *retrace) : m_retrace(retrace) {}

QStateModel::~QStateModel() {}

QQmlListProperty<QStateValue> QStateModel::state() {
  ScopedLock s(m_protect);
  return QQmlListProperty<glretrace::QStateValue>(this, m_states);
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
    m_state_by_name.clear();
    m_renders.clear();
    m_known_paths.clear();
    for (auto i : m_for_deletion)
      delete i;
    m_for_deletion.clear();
  }
}

void QStateModel::onState(SelectionId selectionCount,
                          ExperimentId experimentCount,
                          RenderId renderId,
                          StateKey item,
                          const std::vector<std::string> &value) {
  if (selectionCount == SelectionId(SelectionId::INVALID_SELECTION)) {
    refresh();
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
  std::string path_comp = item.path;
  while (path_comp.length() > 0) {
    auto known = m_known_paths.find(path_comp);
    if (known == m_known_paths.end()) {
      // create an empty item to serve as the directory
      QStateValue *i = new QStateValue(this,
                                       item.group,
                                       path_comp,
                                       "",
                                       std::vector<std::string>());
      StateKey k(item.group, path_comp, "");
      m_state_by_name[k] = i;
      m_known_paths[path_comp] = true;
    } else {
      break;
    }
    path_comp = path_comp.substr(0, path_comp.find_last_of("/"));
  }

  auto &name = item.name;
  auto state_value = m_state_by_name.find(item);
  if (state_value == m_state_by_name.end()) {
    QStateValue *i = new QStateValue(this,
                                     item.group,
                                     item.path,
                                     name,
                                     state_name_to_choices(name));
    m_state_by_name[item] = i;
    state_value = m_state_by_name.find(item);
    m_for_deletion.push_back(i);
  }
  if (value.size() == 1) {
    state_value->second->insert(value[0]);
    return;
  }
  if (value.size() == 4) {
    // color value
    state_value->second->insert(value[0], value[1], value[2], value[3]);
    return;
  }
  assert(false);
}

void
QStateModel::setState(const QString &group,
                      const QString &path,
                      const QString &name,
                      int offset,
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

  StateKey key(group.toStdString(), path.toStdString(), name.toStdString());
  m_retrace->setState(sel, key, offset, value.toStdString());
  emit stateExperiment();
}

void
QStateModel::collapse(const QString &path) {
  const std::string path_str = path.toStdString();
  m_filter_paths[path_str] = true;
  for (auto i : m_state_by_name) {
    if (!i.second->visible().toBool())
      continue;
    if (strncmp(path_str.c_str(), i.first.path.c_str(),
                path_str.length()) == 0) {
      // do not filter the collapsed directories themselves
      if ((i.first.path == path_str) && (i.first.name.length() == 0))
        continue;
      i.second->setVisible(false);
    }
  }
}

void
QStateModel::expand(const QString &path) {
  const std::string path_str = path.toStdString();
  auto i = m_filter_paths.find(path_str);
  assert(i != m_filter_paths.end());
  m_filter_paths.erase(i);
  for (auto i : m_state_by_name) {
    if (i.second->visible().toBool())
      continue;
    if (strncmp(path_str.c_str(), i.first.path.c_str(),
                path_str.length()) == 0) {
      // possibly expanded
      bool visible = true;
      for (auto f : m_filter_paths) {
        if (strncmp(f.first.c_str(), i.first.path.c_str(),
                    f.first.length()) == 0) {
          // do not filter the collapsed directories themselves
          if ((i.first.path == f.first) && (i.first.name.length() == 0))
            visible = true;
          else
            visible = false;
        }
      }
      i.second->setVisible(visible);
    }
  }
}

void
QStateModel::refresh() {
  {
    ScopedLock s(m_protect);
    m_states.clear();
    for (auto i : m_state_by_name) {
      m_states.push_back(i.second);
    }
    set_visible();
  }
  emit stateChanged();
}

void
QStateModel::set_visible() {
  for (auto i : m_state_by_name) {
    bool visible = true;
    for (auto f : m_filter_paths) {
      if (strncmp(f.first.c_str(), i.first.path.c_str(),
                  f.first.length()) == 0) {
        // do not filter the collapsed directories themselves
        if ((i.first.path == f.first) && (i.first.name.length() == 0)) {
          visible = true;
        } else {
          visible = false;
          break;
        }
      }
    }
    if (visible && m_search.length() > 0) {
      if ((!i.second->path().contains(m_search, Qt::CaseInsensitive)) &&
          (!i.second->name().contains(m_search, Qt::CaseInsensitive)))
        visible = false;
    }
    i.second->setVisible(visible);
  }
}

void
QStateModel::search(const QString &_search) {
  m_search = _search;
  set_visible();
}
