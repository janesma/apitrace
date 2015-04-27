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

#include "glframe_retrace_model.hpp"

#include "glframe_retrace.hpp"

using glretrace::FrameRetraceModel;
using glretrace::QRenderBookmark;

FrameRetraceModel::FrameRetraceModel() : m_retrace(NULL)
{}

void
FrameRetraceModel::setFrame(const QString &filename, int framenumber) {
    m_retrace = new FrameRetrace(filename.toStdString(), framenumber);
    for (auto i : m_retrace->getRenders()) {
        std::cout << "got: " << i.start.next_call_no << std::endl;
        m_renders_model.append(new QRenderBookmark(i));
    }
    emit onRenders();
}

QQmlListProperty<QRenderBookmark>
FrameRetraceModel::renders() {
    return QQmlListProperty<QRenderBookmark>(this, m_renders_model);
}


void
FrameRetraceModel::onShaderAssembly(const RenderBookmark &render,
                                    const std::string &vertex_shader,
                                    const std::string &vertex_ir,
                                    const std::string &vertex_vec4,
                                    const std::string &fragment_shader,
                                    const std::string &fragment_ir,
                                    const std::string &fragment_simd8,
                                    const std::string &fragment_simd16)
{
    m_vs_ir = vertex_ir.c_str();
    m_fs_ir = fragment_ir.c_str();
    m_vs_shader = vertex_shader.c_str();
    m_fs_shader = fragment_shader.c_str();
    m_vs_vec4 = vertex_vec4.c_str();
    m_fs_simd8 = fragment_simd8.c_str();
    m_fs_simd16 = fragment_simd16.c_str();
    emit onShaders();
}
void
FrameRetraceModel::onRenderTarget(const RenderBookmark &render, RenderTargetType type,
                        const RenderTargetData &data)
{}
void
FrameRetraceModel::onShaderCompile(const RenderBookmark &render, int status,
                         std::string errorString)
{}

void
FrameRetraceModel::retrace(int start)
{
    for (auto i : m_renders_model) {
        if (i->start() != start)
            continue;
        m_retrace->retraceShaderAssembly(i->bookmark, this);
        return;
    }
}
