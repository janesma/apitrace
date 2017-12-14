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

using glretrace::ExperimentId;
using glretrace::IFrameRetrace;
using glretrace::QStateModel;
using glretrace::QStateValue;
using glretrace::RenderId;
using glretrace::SelectionId;
using glretrace::StateKey;
using glretrace::state_name_to_enum;

static const int kUninitializedValue = -2;
static const int kMixedValue = -1;

QStateValue::QStateValue(QObject *parent) {
  if (parent)
    moveToThread(parent->thread());
}

QStateValue::QStateValue(QObject *parent,
                         const std::string &_path,
                         const std::string &_name,
                         const std::vector<std::string> &_choices,
                         const std::vector<std::string> &_indices)
    : m_path(_path.c_str()),
      m_name(_name.c_str()),
      m_value({}),
      m_visible(true) {
  moveToThread(parent->thread());

  for (auto c : _choices)
    m_choices.append(QString(c.c_str()));
  for (auto i : _indices)
    m_indices.append(QString(i.c_str()));

  const int path_count = std::count(_path.begin(), _path.end(), '/');
  const int indent =  path_count + (_name.length() > 0 ? 1 : 0);
  m_indent = indent;
  if (_name.length() == 0)
    m_name = _path.substr(_path.find_last_of("/") + 1).c_str();
}

void
QStateValue::insert(const std::vector<std::string> &value) {
  QStringList new_value;
  for (auto i : value) {
    new_value.append(m_choices.length() ?
                     value_to_choice(i) :
                     QString::fromStdString(i));
  }

  if (m_value.length() == 0) {
    m_value = new_value;
    emit valueChanged();
    return;
  }

  // else we are appending values from a multiple-render selection to
  // an existing value.
  if (m_value == new_value)
    return;

  QStringList tmp = m_value;
  for (int i = 0; i < tmp.size(); ++i) {
    if (tmp[i] != new_value[i])
      tmp[i] = "###";
  }
  m_value = tmp;
  emit valueChanged();
}

QStateModel::QStateModel() {}

QStateModel::QStateModel(IFrameRetrace *retrace) : m_retrace(retrace) {}

QStateModel::~QStateModel() {
  for (auto i : m_states)
    delete i;
  m_states.clear();
  m_state_by_name.clear();
}

QQmlListProperty<QStateValue> QStateModel::state() {
  ScopedLock s(m_protect);
  return QQmlListProperty<glretrace::QStateValue>(this, m_states);
}

void
QStateModel::clear() {
  {
    ScopedLock s(m_protect);
    for (auto i : m_states)
      i->clear();
  }
}

void QStateModel::onState(SelectionId selectionCount,
                          ExperimentId experimentCount,
                          RenderId renderId,
                          StateKey item,
                          const std::vector<std::string> &value) {
  if (selectionCount == SelectionId(SelectionId::INVALID_SELECTION)) {
    return;
  }

  if ((selectionCount > m_sel_count) ||
      (experimentCount > m_experiment_count)) {
    // state values for a new selection/experiment.  Discard existing
    // values.
    clear();
    m_renders.clear();
  }

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

  auto state_value = m_state_by_name.find(item);
  if (state_value != m_state_by_name.end()) {
    // update existing value
    state_value->second->insert(value);
    return;
  }

  // else this is a state value that has not been seen before.  Create
  // QStateValue objects for the directory tree and the value itself.
  std::string path_comp = item.path;
  while (path_comp.length() > 0) {
    auto known = m_known_paths.find(path_comp);
    if (known == m_known_paths.end()) {
      // create an empty item to serve as the directory
      QStateValue *i = new QStateValue(this,
                                       path_comp,
                                       "",
                                       {}, {});
      StateKey k(path_comp, "");
      m_state_by_name[k] = i;
      m_known_paths[path_comp] = true;
    } else {
      break;
    }
    path_comp = path_comp.substr(0, path_comp.find_last_of("/"));
  }

  auto &name = item.name;
  QStateValue *i = new QStateValue(this,
                                   item.path,
                                   name,
                                   state_name_to_choices(name),
                                   state_name_to_indices(name));
  m_state_by_name[item] = i;
  state_value = m_state_by_name.find(item);
  i->insert(value);

  // the list of QStateValue objects has changed, resort and redraw
  // the ListView displaying them.
  m_states.clear();
  for (auto i : m_state_by_name)
    m_states.push_back(i.second);
  emit stateChanged();
}

void
QStateModel::setState(const QString &path,
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
  const StateKey key(path.toStdString(), name.toStdString());
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

QString
QStateValue::value_to_choice(const std::string &_value) {
  assert(m_choices.length() > 0);
  int c = 0;
  const QString v(_value.c_str());
  for (const auto &i : m_choices) {
    if (i == v)
      return QString().setNum(c);
    ++c;
  }
  assert(false);
  return QString();
}
