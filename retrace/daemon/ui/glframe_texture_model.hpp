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

#ifndef _GLFRAME_TEXTURE_MODEL_HPP_
#define _GLFRAME_TEXTURE_MODEL_HPP_

#include <QObject>
#include <QString>

#include <mutex>
#include <string>
#include <map>
#include <vector>

#include "glframe_retrace_interface.hpp"
#include "glframe_traits.hpp"

namespace glretrace {

class QBoundTexture : public QObject, NoCopy, NoAssign, NoMove {
};

class QTextureUnits : public QObject, NoCopy, NoAssign, NoMove {
  Q_OBJECT
 public:
  QTextureUnits() {}
  ~QTextureUnits() {}
};

class QTextureModel : public QObject,
                      NoCopy, NoAssign, NoMove {
  Q_OBJECT
  // Q_PROPERTY(QString apiCalls READ apiCalls NOTIFY onTextureCalls)
  Q_PROPERTY(QStringList renders READ renders NOTIFY rendersChanged)
 public:
  QTextureModel();
  ~QTextureModel();
  // QString apiCalls();
  QStringList renders() const;
  void onTexture(SelectionId selectionCount,
                 ExperimentId experimentCount,
                 RenderId renderId,
                 TextureKey binding,
                 const std::vector<TextureData> &images);
  Q_INVOKABLE void setIndex(int index);
  void clear();

 signals:
  void indexChanged();
  void rendersChanged();

 private:
  std::map<RenderId, QTextureUnits*> m_texture_units;
  std::vector<RenderId> m_render_index;
  QStringList m_renders;
  SelectionId m_sel_count;
  ExperimentId m_exp_count;
  int m_index;
  mutable std::mutex m_protect;
};

}  // namespace glretrace

#endif  // _GLFRAME_TEXTURE_MODEL_HPP_
