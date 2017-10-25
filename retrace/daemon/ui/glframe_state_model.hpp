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

  Q_PROPERTY(QString group READ group CONSTANT)
  Q_PROPERTY(QString path READ path CONSTANT)
  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(QVariant value READ value CONSTANT)
  Q_PROPERTY(QVariant indent READ indent CONSTANT)
  Q_PROPERTY(QStateType valueType READ valueType CONSTANT)
  Q_PROPERTY(QVariant visible
             READ visible
             WRITE setVisible
             NOTIFY visibleChanged)
  Q_PROPERTY(QList<QVariant> choices READ choices CONSTANT)

 public:
  enum QStateType {
    KglDirectory,
    KglEnum,
    KglFloat,
    KglColor
  };
  Q_ENUMS(QStateType);

  explicit QStateValue(QObject *parent = 0);
  QStateValue(QObject *parent,
              const std::string &_group,
              const std::string &_path,
              const std::string &_name,
              const std::vector<std::string> &_choices);
  void insert(const std::string &value);
  void insert(const std::string &red, const std::string &blue,
              const std::string &green, const std::string &alpha);

  QString group() const { return m_group; }
  QString path() const { return m_path; }
  QString name() const { return m_name; }
  QStateType valueType() const { return m_type; }
  QVariant value() const { return m_value; }
  QVariant indent() const { return m_indent; }
  QVariant visible() const { return m_visible; }
  void setVisible(QVariant v) { m_visible = v; emit visibleChanged(); }
  QList<QVariant> choices() const { return m_choices; }

 signals:
  void visibleChanged();

 private:
  QString m_group, m_path, m_name;
  QVariant m_value, m_indent, m_visible;
  QStateType m_type;
  QList<QVariant> m_choices;
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
               const std::vector<std::string> &value);
  void clear();
  Q_INVOKABLE void setState(const QString &group,
                            const QString &path,
                            const QString &name,
                            const QString &value);
  Q_INVOKABLE void collapse(const QString &path);
  Q_INVOKABLE void expand(const QString &path);

 signals:
  void stateExperiment();
  void stateChanged();

 private:
  void refresh();

  IFrameRetrace *m_retrace;
  SelectionId m_sel_count;
  ExperimentId m_experiment_count;
  // typedef QList<QStateValue*> StateList;
  std::map<StateKey, QStateValue*> m_state_by_name;
  std::map<std::string, bool> m_filter_paths;
  std::map<std::string, bool> m_known_paths;
  QList<QStateValue*> m_states;
  std::vector<QStateValue*> m_for_deletion;
  std::vector<RenderId> m_renders;
  mutable std::mutex m_protect;
};

}  // namespace glretrace

#endif  // _GLFRAME_STATE_MODEL_HPP_
