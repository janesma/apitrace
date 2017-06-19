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

#ifndef _GLFRAME_UNIFORM_MODEL_HPP_
#define _GLFRAME_UNIFORM_MODEL_HPP_

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

class QUniformValue : public QObject, NoCopy, NoAssign, NoMove {
  Q_OBJECT

  Q_PROPERTY(QList<QVariant> data READ data CONSTANT)
  Q_PROPERTY(QUniformDimension dimension READ dimension CONSTANT)
  Q_PROPERTY(QString name READ name CONSTANT)

 public:
  QUniformValue();
  QUniformValue(const std::string &name,
                UniformType t, UniformDimension d,
                const std::vector<unsigned char> &data);
  void print();
  enum QUniformDimension {
    K1x1,
    K2x1,
    K3x1,
    K4x1,
    K2x2,
    K3x2,
    K4x2,
    K2x3,
    K3x3,
    K4x3,
    K2x4,
    K3x4,
    K4x4
  };
  Q_ENUMS(QUniformDimension);

  QString name() const { return m_name; }
  QUniformDimension dimension() const { return m_dimension; }
  QList<QVariant> data() const { return m_data; }

 private:
  QString m_name;
  QList<QVariant> m_data;
  QUniformDimension m_dimension;
};

class QUniformsModel : public QObject,
                       NoCopy, NoAssign, NoMove {
  Q_OBJECT
  Q_PROPERTY(QStringList renders READ renders NOTIFY rendersChanged)
  Q_PROPERTY(QQmlListProperty<glretrace::QUniformValue> uniforms READ uniforms
             NOTIFY uniformsChanged)
 public:
  QUniformsModel();
  explicit QUniformsModel(IFrameRetrace *retrace);
  ~QUniformsModel();
  QStringList renders() const;
  QQmlListProperty<QUniformValue> uniforms();
  void onUniform(SelectionId selectionCount,
                 ExperimentId experimentCount,
                 RenderId renderId,
                 const std::string &name,
                 UniformType type,
                 UniformDimension dimension,
                 const std::vector<unsigned char> &data);
  void clear();
  Q_INVOKABLE void setIndex(int index);
  Q_INVOKABLE void setUniform(const QString &name,
                              const int index,
                              const QString &value);

 signals:
  // new list of collated programs available
  void rendersChanged();
  // after index is selected, uniform data is available
  void uniformsChanged();
  void uniformExperiment();

 private:
  // stores a list of render sets, collated by uniform interface.  If
  // two renders have identical uniforms, then they appear in the same
  // set.
  IFrameRetrace *m_retrace;
  QStringList m_renders;
  SelectionId m_sel_count;
  ExperimentId m_experiment_count;
  int m_index;
  typedef QList<QUniformValue *> uniform_list;
  std::map<RenderId, uniform_list> m_uniforms_by_renderid;
  QList<uniform_list> m_uniforms;
  mutable std::mutex m_protect;
};

}  // namespace glretrace

#endif  // _GLFRAME_UNIFORM_MODEL_HPP_
