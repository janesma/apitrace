/**************************************************************************
 *
 * Copyright 2016 Intel Corporation
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


#ifndef _GLFRAME_SHADER_MODEL_HPP_
#define _GLFRAME_SHADER_MODEL_HPP_

#include <QObject>
#include <QList>
#include <QString>

#include <mutex>
#include <vector>

#include "glframe_retrace_interface.hpp"
#include "glframe_traits.hpp"

namespace glretrace {

class QShader : public QObject,
                NoCopy, NoAssign, NoMove {
  Q_OBJECT

 public:
  QShader() : m_source(""), m_ir(""), m_nir_ssa(""),
              m_nir_final(""), m_simd8(""), m_simd16(""),
              m_simd32("") {}
  virtual ~QShader() {}
  Q_PROPERTY(QString source READ source NOTIFY shadersChanged)
  Q_PROPERTY(QString ir READ ir NOTIFY shadersChanged)
  Q_PROPERTY(QString nirSsa READ nirSsa NOTIFY shadersChanged)
  Q_PROPERTY(QString nirFinal READ nirFinal NOTIFY shadersChanged)
  Q_PROPERTY(QString simd8 READ simd8 NOTIFY shadersChanged)
  Q_PROPERTY(QString simd16 READ simd16 NOTIFY shadersChanged)
  Q_PROPERTY(QString simd32 READ simd32 NOTIFY shadersChanged)
  Q_PROPERTY(QString beforeUnification READ beforeUnification
             NOTIFY shadersChanged)
  Q_PROPERTY(QString afterUnification READ afterUnification
             NOTIFY shadersChanged)
  Q_PROPERTY(QString beforeOptimization READ beforeOptimization
             NOTIFY shadersChanged)
  Q_PROPERTY(QString constCoalescing READ constCoalescing NOTIFY shadersChanged)
  Q_PROPERTY(QString genIrLowering READ genIrLowering NOTIFY shadersChanged)
  Q_PROPERTY(QString layout READ layout NOTIFY shadersChanged)
  Q_PROPERTY(QString optimized READ optimized NOTIFY shadersChanged)
  Q_PROPERTY(QString pushAnalysis READ pushAnalysis NOTIFY shadersChanged)
  Q_PROPERTY(QString codeHoisting READ codeHoisting NOTIFY shadersChanged)
  Q_PROPERTY(QString codeSinking READ codeSinking NOTIFY shadersChanged)

  void onShaderAssembly(const ShaderAssembly &a) {
    m_source = a.shader.c_str();
    m_ir = a.ir.c_str();
    m_nir_ssa = a.ssa.c_str();
    m_nir_final = a.nir.c_str();
    m_simd8 = a.simd8.c_str();
    m_simd16 = a.simd16.c_str();
    m_simd32 = a.simd32.c_str();
    m_beforeUnification = a.beforeUnification.c_str();
    m_afterUnification = a.afterUnification.c_str();
    m_beforeOptimization = a.beforeOptimization.c_str();
    m_constCoalescing = a.constCoalescing.c_str();
    m_genIrLowering = a.genIrLowering.c_str();
    m_layout = a.layout.c_str();
    m_optimized = a.optimized.c_str();
    m_pushAnalysis = a.pushAnalysis.c_str();
    m_codeHoisting = a.codeHoisting.c_str();
    m_codeSinking = a.codeSinking.c_str();
    emit shadersChanged();
  }

  QString source() const { return m_source; }
  QString ir() const { return m_ir; }
  QString nirSsa() const { return m_nir_ssa; }
  QString nirFinal() const { return m_nir_final; }
  QString simd8() const { return m_simd8; }
  QString simd16() const { return m_simd16; }
  QString simd32() const { return m_simd32; }
  QString beforeUnification() const { return m_beforeUnification; }
  QString afterUnification() const { return m_afterUnification; }
  QString beforeOptimization() const { return m_beforeOptimization; }
  QString constCoalescing() const { return m_constCoalescing; }
  QString genIrLowering() const { return m_genIrLowering; }
  QString layout() const { return m_layout; }
  QString optimized() const { return m_optimized; }
  QString pushAnalysis() const { return m_pushAnalysis; }
  QString codeHoisting() const { return m_codeHoisting; }
  QString codeSinking() const { return m_codeSinking; }

 signals:
  void shadersChanged();

 private:
  QString m_source, m_ir, m_nir_ssa, m_nir_final, m_simd8, m_simd16,
    m_simd32, m_beforeUnification, m_afterUnification, m_beforeOptimization,
    m_constCoalescing, m_genIrLowering, m_layout, m_optimized,
    m_pushAnalysis, m_codeHoisting, m_codeSinking;
};

// Encapsulates all shader data for a render
class QRenderShaders : public QObject,
                       NoCopy, NoAssign, NoMove {
  Q_OBJECT

  Q_PROPERTY(glretrace::QShader* vsShader READ vsShader CONSTANT)
  Q_PROPERTY(glretrace::QShader* fsShader READ fsShader CONSTANT)
  Q_PROPERTY(glretrace::QShader* tessControlShader READ tessControlShader
             CONSTANT)
  Q_PROPERTY(glretrace::QShader* tessEvalShader READ tessEvalShader
             CONSTANT)
  Q_PROPERTY(glretrace::QShader* geomShader READ geomShader
             CONSTANT)
  Q_PROPERTY(glretrace::QShader* compShader READ compShader
             CONSTANT)
  Q_PROPERTY(QString renders READ renders NOTIFY onRenders)
 public:
  QRenderShaders() : m_currentSelectionId(0) {}
  ~QRenderShaders() {}
  QShader *vsShader() { return &m_vs; }
  QShader *fsShader() { return &m_fs; }
  QShader *tessControlShader() { return &m_tess_control; }
  QShader *tessEvalShader() { return &m_tess_eval; }
  QShader *geomShader() { return &m_geom; }
  QShader *compShader() { return &m_comp; }

  void onShaderAssembly(const ShaderAssembly &vertex,
                        const ShaderAssembly &fragment,
                        const ShaderAssembly &tess_control,
                        const ShaderAssembly &tess_eval,
                        const ShaderAssembly &geom,
                        const ShaderAssembly &comp);
  QString renders() { return m_renders; }
 signals:
  void onRenders();
 private:
  QShader m_vs, m_fs, m_tess_control, m_tess_eval, m_geom, m_comp;
  SelectionId m_currentSelectionId;
  QString m_renders;
};

class FrameRetraceModel;
// collates renders that share programs
class QRenderShadersList : public QObject,
                           NoCopy, NoAssign, NoMove {
  Q_OBJECT
  Q_PROPERTY(glretrace::QRenderShaders* shaders READ shaders
             CONSTANT)
  Q_PROPERTY(QStringList renders
             READ renders NOTIFY onRendersChanged)
 public:
  QRenderShadersList() : m_retracer(NULL), m_retraceModel(NULL) {}
  ~QRenderShadersList() {}
  void setRetrace(IFrameRetrace *retracer,
                  FrameRetraceModel *model);
  QRenderShaders *shaders() { return &m_shaders; }
  void onShaderAssembly(RenderId renderId,
                        SelectionId selectionCount,
                        const ShaderAssembly &vertex,
                        const ShaderAssembly &fragment,
                        const ShaderAssembly &tess_control,
                        const ShaderAssembly &tess_eval,
                        const ShaderAssembly &geom,
                        const ShaderAssembly &comp);
  QStringList renders();
  Q_INVOKABLE void setIndex(int index);
  Q_INVOKABLE void overrideShaders(int index,
                                   const QString &vs, const QString &fs,
                                   const QString &tess_control,
                                   const QString &tess_eval,
                                   const QString &geom, const QString &comp);
 signals:
  void onRendersChanged();
 private:
  void setIndexDirect(int index);

  mutable std::mutex m_protect;
  QRenderShaders m_shaders;
  QStringList m_render_strings;
  std::vector<std::vector<RenderId>> m_renders;
  std::vector<std::vector<ShaderAssembly>> m_shader_assemblies;
  SelectionId m_current_selection;
  IFrameRetrace *m_retracer;
  FrameRetraceModel *m_retraceModel;
};

}  // namespace glretrace

#endif /* _GLFRAME_SHADER_MODEL_HPP_ */
