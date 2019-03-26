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
  Q_OBJECT

 public:
  explicit QBoundTexture(QObject *parent = NULL);
  ~QBoundTexture() {}
  void onTexture(ExperimentId experimentCount,
                 RenderId render,
                 const TextureKey &binding,
                 const std::vector<TextureData> &images);
  Q_INVOKABLE void selectLevel(int index);
  Q_PROPERTY(QStringList details READ details NOTIFY detailsChanged)
  Q_PROPERTY(QStringList levels READ levels NOTIFY levelsChanged)
  Q_PROPERTY(QString image READ image NOTIFY imageChanged)

  QStringList details();
  QStringList levels() { return m_levels; }
  QString image();
 signals:
  void detailsChanged();
  void levelsChanged();
  void imageChanged();

 private:
  std::vector<QStringList> m_details;
  QStringList m_levels;
  QStringList m_image_urls;
  int m_index;
};

class RenderTextures {
 public:
  explicit RenderTextures(QObject *parent) : q_parent(parent) {}
  void onTexture(ExperimentId experimentCount,
                 RenderId render,
                 const TextureKey &binding,
                 const std::vector<TextureData> &images);
  QStringList bindings();
  QBoundTexture *texture(QString q);
 private:
  std::map<QString, QBoundTexture*> m_binding_desc_to_texture;
  QObject *q_parent;
};

class QTextureModel : public QObject,
                      NoCopy, NoAssign, NoMove {
  Q_OBJECT
  Q_PROPERTY(QStringList renders READ renders NOTIFY rendersChanged)
  Q_PROPERTY(QStringList bindings READ bindings NOTIFY bindingsChanged)
  Q_PROPERTY(glretrace::QBoundTexture* texture
             READ texture
             NOTIFY textureChanged)
 public:
  QTextureModel();
  ~QTextureModel();
  // QString apiCalls();
  QStringList renders() const;
  void onTexture(SelectionId selectionCount,
                 ExperimentId experimentCount,
                 RenderId renderId,
                 const TextureKey &binding,
                 const std::vector<TextureData> &images);
  Q_INVOKABLE void selectRender(int index);
  Q_INVOKABLE void selectBinding(QString b);
  void clear();
  QStringList bindings() { return m_bindings; }
  QBoundTexture *texture() { return m_currentTexture; }
  void retraceTextures(IFrameRetrace *retrace,
                       const QList<int> &renders,
                       SelectionId selection_count,
                       ExperimentId experiment_count,
                       OnFrameRetrace *callback);


 signals:
  void indexChanged();
  void rendersChanged();
  void bindingsChanged();
  void textureChanged();

 private:
  std::map<RenderId, RenderTextures*> m_texture_units;
  std::vector<RenderId> m_render_index;
  QStringList m_renders;
  QStringList m_bindings;
  SelectionId m_sel_count;
  ExperimentId m_exp_count;
  QBoundTexture* m_currentTexture;
  QBoundTexture m_defaultTexture;
  int m_index;
  QString m_cached_binding_selection;
  mutable std::mutex m_protect;
};

}  // namespace glretrace

#endif  // _GLFRAME_TEXTURE_MODEL_HPP_
