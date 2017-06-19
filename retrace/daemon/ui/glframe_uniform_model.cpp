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

#include "glframe_uniform_model.hpp"

#include <string>
#include <vector>

#include "glframe_os.hpp"

using glretrace::ExperimentId;
using glretrace::IFrameRetrace;
using glretrace::QUniformsModel;
using glretrace::QUniformValue;
using glretrace::RenderId;
using glretrace::ScopedLock;
using glretrace::SelectionId;
using glretrace::UniformType;
using glretrace::UniformDimension;

QUniformsModel::QUniformsModel() {
}

QUniformsModel::QUniformsModel(IFrameRetrace *retrace)
    : m_retrace(retrace) {
}

QUniformsModel::~QUniformsModel() {
}

void
QUniformsModel::onUniform(SelectionId selectionCount,
                 ExperimentId experimentCount,
                 RenderId renderId,
                 const std::string &name,
                 UniformType type,
                 UniformDimension dimension,
                          const std::vector<unsigned char> &data) {
  if (selectionCount == SelectionId(SelectionId::INVALID_SELECTION)) {
    {
      ScopedLock s(m_protect);
      m_renders.clear();
      m_uniforms.clear();
      // collate programs
      for (auto i : m_uniforms_by_renderid) {
        m_renders.push_back(QString("%1").arg(i.first.index()));
        m_uniforms.push_back(i.second);
      }
    }
    emit rendersChanged();
    setIndex(0);
    return;
  }
  ScopedLock s(m_protect);
  QUniformValue *u = new QUniformValue(name, type, dimension, data);
  m_uniforms_by_renderid[renderId].push_back(u);
  m_sel_count = selectionCount;
}

void
QUniformsModel::setIndex(int index) {
  {
    ScopedLock s(m_protect);
    m_index = index;
  }
  emit uniformsChanged();
}

QUniformValue::QUniformValue() : m_dimension(K1x1) {
}

QUniformValue::QUniformValue(const std::string &name,
                             UniformType t, UniformDimension d,
                             const std::vector<unsigned char> &data)
    : m_name(name.c_str()) {
  switch (d) {
    case k1x1:
      m_dimension = QUniformValue::K1x1;
      break;
    case k2x1:
      m_dimension = QUniformValue::K2x1;
      break;
    case k3x1:
      m_dimension = QUniformValue::K3x1;
      break;
    case k4x1:
      m_dimension = QUniformValue::K4x1;
      break;
    case k2x2:
      m_dimension = QUniformValue::K2x2;
      break;
    case k3x2:
      m_dimension = QUniformValue::K3x2;
      break;
    case k4x2:
      m_dimension = QUniformValue::K4x2;
      break;
    case k2x3:
      m_dimension = QUniformValue::K2x3;
      break;
    case k3x3:
      m_dimension = QUniformValue::K3x3;
      break;
    case k4x3:
      m_dimension = QUniformValue::K4x3;
      break;
    case k2x4:
      m_dimension = QUniformValue::K2x4;
      break;
    case k3x4:
      m_dimension = QUniformValue::K3x4;
      break;
    case k4x4:
      m_dimension = QUniformValue::K4x4;
      break;
  }
  switch (t) {
    case kFloatUniform: {
      const float* p = reinterpret_cast<const float *>(data.data());
      int count = data.size() / sizeof(float);
      while (count > 0) {
        m_data.push_back(*p);
        --count;
        ++p;
      }
      break;
    }
    case kIntUniform: {
      const int32_t* p = reinterpret_cast<const int32_t *>(data.data());
      int count = data.size() / sizeof(int32_t);
      while (count > 0) {
        m_data.push_back(*p);
        --count;
        ++p;
      }
      break;
    }
    case kUIntUniform: {
      const uint32_t* p = reinterpret_cast<const uint32_t *>(data.data());
      int count = data.size() / sizeof(uint32_t);
      while (count > 0) {
        m_data.push_back(*p);
        --count;
        ++p;
      }
      break;
    }
    case kBoolUniform: {
      const int32_t* p = reinterpret_cast<const int32_t*>(data.data());
      int count = data.size() / sizeof(int32_t);
      while (count > 0) {
        m_data.push_back(*p == 0 ? false : true);
        --count;
        ++p;
      }
      break;
    }
  }
}

void
QUniformValue::print() {
  printf("\t%s\n", m_name.toLatin1().data());
  printf("\t\t");
  for (auto i : m_data) {
    QString s = i.toString();
    printf("%s\t", s.toLatin1().data());
  }
  printf("\n");
}

QStringList
QUniformsModel::renders() const {
  ScopedLock s(m_protect);
  return m_renders;
}

QQmlListProperty<QUniformValue>
QUniformsModel::uniforms() {
  ScopedLock s(m_protect);
  if (m_index >= m_uniforms.size())
    return QQmlListProperty<QUniformValue>();
  return QQmlListProperty<QUniformValue>(this, m_uniforms[m_index]);
}

void
QUniformsModel::clear()  {
  {
    ScopedLock s(m_protect);
    m_renders.clear();
    m_uniforms.clear();
    m_index = 0;
  }
  emit rendersChanged();
  emit uniformsChanged();

  {
    ScopedLock s(m_protect);
    for (auto i : m_uniforms_by_renderid) {
      for (auto j : i.second) {
        delete j;
      }
    }
    m_uniforms_by_renderid.clear();
  }
}

void
QUniformsModel::setUniform(const QString &name,
                           const int index,
                           const QString &value) {
  RenderSelection selection;
  selection.id = m_sel_count;
  auto render = m_uniforms_by_renderid.begin();
  for (int i = 0; i < m_index; ++i)
    ++render;

  selection.series.push_back(RenderSequence(render->first,
                                            RenderId(render->first() + 1)));
  m_retrace->setUniform(selection,
                        name.toStdString(),
                        index,
                        value.toStdString());
  emit uniformExperiment();
}

