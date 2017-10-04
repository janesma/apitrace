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

#ifndef _GLFRAME_STATE_MODEL_HPP_
#define _GLFRAME_STATE_MODEL_HPP_

#include <QObject>
#include <QList>
#include <QQmlListProperty>
#include <QString>
#include <QVariant>

#include <mutex>
#include <string>
#include <map>
#include <vector>

#include "glframe_retrace_interface.hpp"
#include "glframe_traits.hpp"

namespace glretrace {

class QStateValue : public QObject, NoCopy, NoAssign, NoMove {
  Q_OBJECT

  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(QList<QVariant> values READ values CONSTANT)
  Q_PROPERTY(QList<QVariant> choices READ choices CONSTANT)

 public:
  QStateValue() {}
  QStateValue(const std::string &_name,
              const std::vector<std::string> &_choices);
  void insert(int index, const std::string &value);

  QString name() const { return m_name; }
  QList<QVariant> values() const { return m_values; }
  QList<QVariant> choices() const { return m_choices; }

 private:
  QString m_name;
  QList<QVariant> m_values, m_choices;
};

class QStateModel : public QObject,
                    NoCopy, NoAssign, NoMove {
  Q_OBJECT
  Q_PROPERTY(QQmlListProperty<glretrace::QStateValue> state READ state
             NOTIFY stateChanged)
 public:
  QStateModel();
  explicit QStateModel(IFrameRetrace *retrace);
  ~QStateModel();
  QQmlListProperty<QStateValue> state();
  void onState(SelectionId selectionCount,
               ExperimentId experimentCount,
               RenderId renderId,
               StateKey item,
               const std::string &value);
  void clear();
  Q_INVOKABLE void setState(const QString &name,
                            const int index,
                            const QString &value);

 signals:
  void stateExperiment();
  void stateChanged();

 private:
  IFrameRetrace *m_retrace;
  SelectionId m_sel_count;
  ExperimentId m_experiment_count;
  // typedef QList<QStateValue*> StateList;
  std::map<std::string, QStateValue*> m_state_by_name;
  QList<QStateValue*> m_states;
  std::vector<QStateValue*> m_for_deletion;
  std::vector<RenderId> m_renders;
  mutable std::mutex m_protect;
};

}  // namespace glretrace

#endif  // _GLFRAME_STATE_MODEL_HPP_
