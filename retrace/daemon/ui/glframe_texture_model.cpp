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
QTextureModel::onTexture(SelectionId selectionCount,
                         ExperimentId experimentCount,
                         RenderId renderId,
                         const TextureKey &binding,
                         const std::vector<TextureData> &images) {
  if (selectionCount == SelectionId(SelectionId::INVALID_SELECTION)) {
    {
      ScopedLock s(m_protect);
      for (auto i : m_texture_units) {
        m_render_index.push_back(i.first);
        m_renders.push_back(QString("%1").arg(i.first.index()));
      }
    }
    // all textures received
    emit rendersChanged();
    selectRender(0);
    return;
  }

  if (m_sel_count != selectionCount || m_exp_count != experimentCount) {
    clear();
    {
      ScopedLock s(m_protect);
      m_sel_count = selectionCount;
      m_exp_count = experimentCount;
    }
  }

  ScopedLock s(m_protect);
  if (m_texture_units.find(renderId) == m_texture_units.end()) {
    m_texture_units[renderId] = new RenderTextures(this);
  }
  m_texture_units[renderId]->onTexture(experimentCount, renderId,
                                       binding, images);
}

void
QTextureModel::selectRender(int index) {
  {
    ScopedLock s(m_protect);
    m_index = index;
    if (m_render_index.size() <= index)
      m_bindings = QStringList();
    else
      m_bindings = m_texture_units[m_render_index[index]]->bindings();
  }
  emit bindingsChanged();
}

void
QTextureModel::selectBinding(QString b) {
  {
    ScopedLock s(m_protect);
    if (m_index > m_render_index.size())
      m_currentTexture = &m_defaultTexture;
    else
      m_currentTexture = m_texture_units[m_render_index[m_index]]->texture(b);
  }
  emit textureChanged();
}

void
QTextureModel::clear() {
  {
    ScopedLock s(m_protect);
    m_renders.clear();
    m_render_index.clear();
    for (auto i : m_texture_units)
      // TODO(majanes) possible use-after-delete here
      delete i.second;
    m_texture_units.clear();
  }
}

void
RenderTextures::onTexture(ExperimentId experimentCount,
                          RenderId render,
                          const TextureKey &binding,
                          const std::vector<TextureData> &images) {
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
  FrameImages *fi = FrameImages::instance();
  assert(m_details.empty());
  int expected_level = 0;
  for (auto i : images) {
    {
      assert(expected_level == i.level);
      ++expected_level;
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

      QString imageProviderFmt = "texture/exp_%1/ren_%2/%3_%4_%5/level_%6.png";
      QString imageProvider(imageProviderFmt.arg(
          QString::number(experimentCount.count()),
          QString::number(render.index()),
          state_enum_to_name(binding.unit).c_str(),
          state_enum_to_name(binding.target).c_str(),
          QString::number(binding.offset),
          QString::number(i.level)));

      QString urlFmt = "image://myimageprovider/%1";
      QString url(urlFmt.arg(imageProvider));
      m_image_urls.push_back(url);

      assert(!i.image_data.empty());
      fi->AddTexture(imageProvider.toStdString().c_str(), i.image_data);
    }
  }
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
