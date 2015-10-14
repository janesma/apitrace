/**************************************************************************
 *
 * Copyright 2015 Intel Corporation
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
 **************************************************************************/


#ifndef _GLFRAME_RETRACE_MODEL_HPP_
#define _GLFRAME_RETRACE_MODEL_HPP_

#include <QObject>
#include <QList>
#include <QString>
#include <QQmlListProperty>

#include "glframe_retrace.hpp"
#include "glframe_retrace_stub.hpp"
#include "glframe_os.hpp"

namespace glretrace {

class FrameRetrace;

class QRenderBookmark : public QObject
{
    Q_OBJECT
public:
    QRenderBookmark() : renderId(0) {}
    QRenderBookmark(const QRenderBookmark& o) : renderId(o.renderId) {}
    Q_PROPERTY(int index READ index NOTIFY onIndex);

    explicit QRenderBookmark(int id)
        : renderId(id) {}

    int index() { return renderId.index(); }
    glretrace::RenderId renderId;
signals:
    void onIndex();
};

class FrameRetraceModel : public QObject,
                          public OnFrameRetrace
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<glretrace::QRenderBookmark> renders
               READ renders NOTIFY onRenders)
    Q_PROPERTY(QString vsIR READ vsIR NOTIFY onShaders)
    Q_PROPERTY(QString fsIR READ fsIR NOTIFY onShaders)
    Q_PROPERTY(QString vsSource READ vsSource NOTIFY onShaders)
    Q_PROPERTY(QString fsSource READ fsSource NOTIFY onShaders)
    Q_PROPERTY(QString vsVec4 READ vsVec4 NOTIFY onShaders)
    Q_PROPERTY(QString fsSimd8 READ fsSimd8 NOTIFY onShaders)
    Q_PROPERTY(QString fsSimd16 READ fsSimd16 NOTIFY onShaders)
    Q_PROPERTY(QString renderTargetImage READ renderTargetImage NOTIFY onRenderTarget)
    Q_PROPERTY(int openPercent READ openPercent NOTIFY onOpenPercent)
public:
    FrameRetraceModel();
    ~FrameRetraceModel();

    Q_INVOKABLE void setFrame(const QString &filename, int framenumber);
    Q_INVOKABLE void retrace(int start);
    QQmlListProperty<QRenderBookmark> renders();

    void onFileOpening(bool finished,
                       uint32_t percent_complete);
    void onShaderAssembly(RenderId renderId,
                          const std::string &vertex_shader,
                          const std::string &vertex_ir,
                          const std::string &vertex_vec4,
                          const std::string &fragment_shader,
                          const std::string &fragment_ir,
                          const std::string &fragment_simd8,
                          const std::string &fragment_simd16);
    void onRenderTarget(RenderId renderId, RenderTargetType type,
                        const std::vector<unsigned char> &data);
    void onShaderCompile(RenderId renderId, int status,
                         std::string errorString);
    void onMetricList(const std::vector<MetricId> &ids,
                      const std::vector<std::string> &names) {}
    void onMetrics(const MetricSeries &metricData,
                   ExperimentId experimentCount) {}
    QString vsIR() const { ScopedLock s(m_protect); return m_vs_ir; }
    QString fsIR() const { ScopedLock s(m_protect); return m_fs_ir; }
    QString vsSource() const { ScopedLock s(m_protect); return m_vs_shader; }
    QString fsSource() const { ScopedLock s(m_protect); return m_fs_shader; }
    QString vsVec4() const { ScopedLock s(m_protect); return m_vs_vec4; }
    QString fsSimd8() const { ScopedLock s(m_protect); return m_fs_simd8; }
    QString fsSimd16() const { ScopedLock s(m_protect); return m_fs_simd16; }
    QString renderTargetImage() const;
    int openPercent() const { ScopedLock s(m_protect); return m_open_percent; }
signals:
    void onShaders();
    void onRenders();
    void onRenderTarget();
    void onOpenPercent();
private:
    mutable std::mutex m_protect;
    FrameRetraceStub m_retrace;
    FrameState *m_state;
    QList<QRenderBookmark *> m_renders_model;
    QString m_vs_ir, m_fs_ir, m_vs_shader, m_fs_shader,
        m_vs_vec4, m_fs_simd8, m_fs_simd16 ;
    int m_open_percent;
};
}

#endif  // _GLFRAME_RETRACE_MODEL_HPP_
