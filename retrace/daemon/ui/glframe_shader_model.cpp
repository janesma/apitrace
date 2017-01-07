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

#include "glframe_shader_model.hpp"

#include <sstream>
#include <string>
#include <vector>

#include "glframe_logger.hpp"
#include "glframe_os.hpp"
#include "glframe_retrace_model.hpp"

using glretrace::DEBUG;
using glretrace::FrameRetraceModel;
using glretrace::QRenderShaders;
using glretrace::QRenderShadersList;
using glretrace::ScopedLock;

void
QRenderShaders::onShaderAssembly(const ShaderAssembly &vertex,
                                 const ShaderAssembly &fragment,
                                 const ShaderAssembly &tess_control,
                                 const ShaderAssembly &tess_eval,
                                 const ShaderAssembly &geom,
                                 const ShaderAssembly &comp) {
  m_vs.onShaderAssembly(vertex);
  m_fs.onShaderAssembly(fragment);
  m_tess_control.onShaderAssembly(tess_control);
  m_tess_eval.onShaderAssembly(tess_eval);
  m_geom.onShaderAssembly(geom);
  m_comp.onShaderAssembly(comp);
}

void
QRenderShadersList::onShaderAssembly(RenderId renderId,
                                     SelectionId selectionCount,
                                     const ShaderAssembly &vertex,
                                     const ShaderAssembly &fragment,
                                     const ShaderAssembly &tess_control,
                                     const ShaderAssembly &tess_eval,
                                     const ShaderAssembly &geom,
                                     const ShaderAssembly &comp) {
  ScopedLock s(m_protect);
  if (selectionCount != m_current_selection) {
    m_renders.clear();
    m_shader_assemblies.clear();
    m_render_strings.clear();
    m_current_selection = selectionCount;
  }
  assert(m_render_strings.size() == m_shader_assemblies.size());
  for (int i = 0; i < m_shader_assemblies.size(); ++i) {
    if ((vertex.shader == m_shader_assemblies[i][0].shader) &&
        (fragment.shader == m_shader_assemblies[i][1].shader) &&
        (tess_control.shader == m_shader_assemblies[i][2].shader) &&
        (tess_eval.shader == m_shader_assemblies[i][3].shader) &&
        (geom.shader == m_shader_assemblies[i][4].shader) &&
        (comp.shader == m_shader_assemblies[i][5].shader)) {
      m_renders[i].push_back(renderId);
      m_render_strings[i].append(QString(",%1").arg(renderId.index()));
      emit onRendersChanged();
      return;
    }
  }

  // program not found
  m_renders.push_back(std::vector<RenderId>());
  m_renders.back().push_back(renderId);
  m_render_strings.push_back(QString("%1").arg(renderId.index()));
  m_shader_assemblies.push_back(std::vector<ShaderAssembly>());
  m_shader_assemblies.back().push_back(ShaderAssembly());
  m_shader_assemblies.back().back() = vertex;
  m_shader_assemblies.back().push_back(ShaderAssembly());
  m_shader_assemblies.back().back() = fragment;
  m_shader_assemblies.back().push_back(ShaderAssembly());
  m_shader_assemblies.back().back() = tess_control;
  m_shader_assemblies.back().push_back(ShaderAssembly());
  m_shader_assemblies.back().back() = tess_eval;
  m_shader_assemblies.back().push_back(ShaderAssembly());
  m_shader_assemblies.back().back() = geom;
  m_shader_assemblies.back().push_back(ShaderAssembly());
  m_shader_assemblies.back().back() = comp;

  if (m_render_strings.size() == 1)
    // first shader assembly for a new selection.  update shader display
    setIndexDirect(0);

  emit onRendersChanged();
}

QStringList
QRenderShadersList::renders() {
  ScopedLock s(m_protect);
  return m_render_strings;
}

void
QRenderShadersList::setIndexDirect(int index) {
  m_shaders.onShaderAssembly(m_shader_assemblies[index][0],
                             m_shader_assemblies[index][1],
                             m_shader_assemblies[index][2],
                             m_shader_assemblies[index][3],
                             m_shader_assemblies[index][4],
                             m_shader_assemblies[index][5]);
}

void
QRenderShadersList::setIndex(int index) {
  ScopedLock s(m_protect);
  setIndexDirect(index);
}

void
QRenderShadersList::setRetrace(IFrameRetrace *retracer,
                               FrameRetraceModel *model) {
  m_retracer = retracer;
  m_retraceModel = model;
}

void
QRenderShadersList::overrideShaders(int index,
                                    const QString &vs, const QString &fs,
                                    const QString &tess_control,
                                    const QString &tess_eval,
                                    const QString &geom,
                                    const QString &comp) {
  ScopedLock s(m_protect);
  const std::string &vss = vs.toStdString(),
                    &fss = fs.toStdString(),
         &tess_control_s = tess_control.toStdString(),
            &tess_eval_s = tess_eval.toStdString(),
                 &geom_s = geom.toStdString(),
                 &comp_s = comp.toStdString();

  GRLOGF(DEBUG, "vs: %s\nfs: %s\ntess_control: %s\n"
         "tess_eval: %s\ngeom: %s\ncomp: %s",
         vss.c_str(), fss.c_str(),
         tess_control_s.c_str(), tess_eval_s.c_str(), geom_s.c_str(),
         comp_s.c_str());
  for (auto i : m_renders[index]) {
    m_retracer->replaceShaders(i, ExperimentId(0), vss, fss,
                               tess_control_s, tess_eval_s, geom_s, comp_s,
                               m_retraceModel);
  }
  m_renders.clear();
  m_shader_assemblies.clear();
  m_render_strings.clear();
  m_retraceModel->onShadersChanged();
}

