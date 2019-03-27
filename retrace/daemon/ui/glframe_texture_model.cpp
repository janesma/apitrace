/**************************************************************************
 *
 * Copyright 2019 Intel Corporation
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

#include "glframe_texture_model.hpp"

#include <string>
#include <vector>

#include "glframe_os.hpp"
#include "glframe_retrace_images.hpp"
#include "glframe_qutil.hpp"
#include "glframe_state_enums.hpp"
#include "glframe_logger.hpp"

using glretrace::QTextureModel;
using glretrace::RenderId;
using glretrace::ScopedLock;
using glretrace::SelectionId;
using glretrace::FrameImages;
using glretrace::RenderTextures;
using glretrace::state_name_to_enum;
using glretrace::QBoundTexture;

QTextureModel::QTextureModel() : m_currentTexture(&m_defaultTexture),
                                 m_defaultTexture(this) {}

QTextureModel::~QTextureModel() {}

QStringList
QTextureModel::renders() const {
  ScopedLock s(m_protect);
  return m_renders;
}

void
QTextureModel::retraceTextures(IFrameRetrace *retrace,
                               const QList<int> &renders,
                               SelectionId selectionCount,
                               ExperimentId experimentCount,
                               OnFrameRetrace *callback) {
  assert(m_sel_count != selectionCount || m_exp_count != experimentCount);
  clear();
  if (experimentCount.count() > m_exp_count.count()) {
    // existing textures are out of date
    FrameImages *fi = FrameImages::instance();
    fi->ClearTextures();
    {
      ScopedLock s(m_protect);
      for (auto i : m_texture_units)
        delete i.second;
      m_texture_units.clear();
    }
  }

  {
    ScopedLock s(m_protect);
    for (auto i : renders) {
      m_render_index.push_back(RenderId(i));
      m_renders.push_back(QString("%1").arg(i));
    }
    m_sel_count = selectionCount;
    m_exp_count = experimentCount;
  }

  // only retrace the textures that are not cached
  QList<int> uncached_renders;
  for (auto r : renders) {
    if (m_texture_units.find(RenderId(r)) == m_texture_units.end())
      uncached_renders.push_back(r);
  }
  RenderSelection sel;
  glretrace::renderSelectionFromList(selectionCount,
                                     uncached_renders,
                                     &sel);
  retrace->retraceTextures(sel, experimentCount, callback);
}

void
QTextureModel::onTexture(SelectionId selectionCount,
                         ExperimentId experimentCount,
                         RenderId renderId,
                         const TextureKey &binding,
                         const std::vector<TextureData> &images) {
  if (selectionCount == SelectionId(SelectionId::INVALID_SELECTION)) {
    // all textures received
    emit rendersChanged();
    selectRender(0);
    return;
  }

  ScopedLock s(m_protect);
  if (m_texture_units.find(renderId) == m_texture_units.end()) {
    m_texture_units[renderId] = new RenderTextures(this);
  }
  m_texture_units[renderId]->onTexture(experimentCount, renderId,
                                       binding, images);
}

void
QTextureModel::onTextureData(ExperimentId experimentCount,
                             const std::string &md5sum,
                             const std::vector<unsigned char> &image) {
  const QString urlFmt = "texture/%1.png";
  const QString url(urlFmt.arg(QString(md5sum.c_str())));
  FrameImages::instance()->AddTexture(url.toStdString().c_str(), image);
}

void
QTextureModel::selectRender(int index) {
  {
    ScopedLock s(m_protect);
    m_index = index;
    if (m_render_index.size() <= index) {
      m_bindings = QStringList();
    } else {
      RenderId r = m_render_index[index];
      if (m_texture_units.find(r) == m_texture_units.end()) {
        m_bindings = QStringList();
      } else {
        m_bindings = m_texture_units[m_render_index[index]]->bindings();
      }
    }
  }
  emit bindingsChanged();
  selectBinding(m_cached_binding_selection);
}

class Emit {
 public:
  explicit Emit(QTextureModel *m) : model(m) {}
  ~Emit() {
    model->textureChanged();
  }
  QTextureModel *model;
};

void
QTextureModel::selectBinding(QString b) {
  Emit e(this);
  {
    ScopedLock s(m_protect);
    m_currentTexture = &m_defaultTexture;

    if (m_index >= m_render_index.size())
      return;

    const auto render = m_render_index[m_index];
    if (m_texture_units.find(render) == m_texture_units.end())
      return;

    auto unit = m_texture_units[render];
    auto tex = unit->texture(b);
    if (!tex)
      return;
    m_currentTexture = tex;
    m_cached_binding_selection = b;
  }
}

void
QTextureModel::clear() {
  {
    ScopedLock s(m_protect);
    m_renders.clear();
    m_render_index.clear();
    // for (auto i : m_texture_units)
    //   // TODO(majanes) possible use-after-delete here
    //   delete i.second;
    // m_texture_units.clear();
  }
}

void
RenderTextures::onTexture(ExperimentId experimentCount,
                          RenderId render,
                          const TextureKey &binding,
                          const std::vector<TextureData> &images) {
  assert(binding.unit >= 0);

  // generate binding descriptor
  QString desc = QString("%1 %2 offset %3").arg(
      state_enum_to_name(binding.unit).c_str(),
      state_enum_to_name(binding.target).c_str(),
      QString::number(binding.offset));

  if (m_binding_desc_to_texture.find(desc) ==
      m_binding_desc_to_texture.end()) {
    m_binding_desc_to_texture[desc] = new QBoundTexture(q_parent);
  }

  m_binding_desc_to_texture[desc]->onTexture(experimentCount,
                                             render, binding,
                                             images);
}

QBoundTexture *
RenderTextures::texture(QString q) {
  if (m_binding_desc_to_texture.find(q) == m_binding_desc_to_texture.end())
    return NULL;
  return m_binding_desc_to_texture[q];
}

QStringList
RenderTextures::bindings() {
  QStringList b;
  for (auto i : m_binding_desc_to_texture)
    b.push_back(i.first);
  return b;
}

QBoundTexture::QBoundTexture(QObject *parent) : m_index(0) {
  // these objects will be generated by onTexture callback in the
  // retrace thread, not in the UI thread.
  if (parent)
    moveToThread(parent->thread());
}

void
QBoundTexture::onTexture(ExperimentId experimentCount,
                         RenderId render,
                         const TextureKey &binding,
                         const std::vector<TextureData> &images) {
  for (auto i : images) {
    {
      QStringList level_details;
      level_details.append(QString("Width: %1").arg(i.width));
      level_details.append(QString("Height: %1").arg(i.height));
      level_details.append(QString("Type: %1").arg(i.type.c_str()));
      level_details.append(QString("Format: %1").arg(i.format.c_str()));
      level_details.append(QString("Internal Format: %1").arg(
          i.internalFormat.c_str()));
      // TODO(majanes): avoid copy here
      m_details.push_back(level_details);
      m_levels.push_back(QString("Level: %1").arg(i.level));

      QString urlFmt = "image://myimageprovider/texture/%1.png";
      QString url(urlFmt.arg(QString(i.md5sum.c_str())));
      m_image_urls.push_back(url);
    }
  }
  emit levelsChanged();
}

QStringList
QBoundTexture::details() {
  if (m_details.size() == 0)
    return QStringList();
  return m_details[m_index];
}

void
QBoundTexture::selectLevel(int index) {
  m_index = index;
  emit detailsChanged();
  emit imageChanged();
}

QString
QBoundTexture::image() {
  if (m_image_urls.size() == 0)
    return QString("image://myimageprovider/default.image.url");
  return m_image_urls[m_index];
}

