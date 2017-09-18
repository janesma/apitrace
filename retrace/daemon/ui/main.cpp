// Copyright (C) Intel Corp.  2015.  All Rights Reserved.

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice (including the
// next paragraph) shall be included in all copies or substantial
// portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//  **********************************************************************/
//  * Authors:
//  *   Mark Janes <mark.a.janes@intel.com>
//  **********************************************************************/

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QList>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>

#ifndef NDEBUG
#define QT_QML_DEBUG
#include <QQmlDebuggingEnabler>
#endif

#include <sstream>
#include <string>

#include "glframe_api_model.hpp"
#include "glframe_experiment_model.hpp"
#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"
#include "glframe_os.hpp"
#include "glframe_qbargraph.hpp"
#include "glframe_rendertarget_model.hpp"
#include "glframe_retrace_images.hpp"
#include "glframe_retrace_model.hpp"
#include "glframe_retrace_skeleton.hpp"
#include "glframe_retrace_stub.hpp"
#include "glframe_shader_model.hpp"
#include "glframe_socket.hpp"
#include "glframe_uniform_model.hpp"
#include "glretrace.hpp"

using glretrace::FrameRetraceModel;
using glretrace::FrameRetraceSkeleton;
using glretrace::FrameRetraceStub;
using glretrace::GlFunctions;
using glretrace::Logger;
using glretrace::QApiModel;
using glretrace::QMetric;
using glretrace::QRenderBookmark;
using glretrace::QRenderShaders;
using glretrace::ServerSocket;
using glretrace::Socket;


Q_DECLARE_METATYPE(QList<glretrace::BarMetrics>)
Q_DECLARE_METATYPE(QList<glretrace::SelectionId>)
Q_DECLARE_METATYPE(glretrace::ExperimentId)
Q_DECLARE_METATYPE(glretrace::SelectionId)

int main(int argc, char *argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

#ifndef NDEBUG
  QQmlDebuggingEnabler enabler;
#endif

  GlFunctions::Init();
  Logger::Create();
  Logger::SetSeverity(glretrace::DEBUG);
  Socket::Init();
  Logger::Begin();

  QGuiApplication app(argc, argv);
  app.setOrganizationName("Open Source Technology Center");
  app.setOrganizationDomain("intel.com");
  app.setApplicationName("frameretrace");

  qRegisterMetaType<glretrace::ExperimentId>();
  qRegisterMetaType<glretrace::SelectionId>();
  qRegisterMetaType<QList<glretrace::BarMetrics> >();
  qRegisterMetaType<QList<int> >();
  qmlRegisterType<glretrace::QRenderBookmark>("ApiTrace", 1, 0,
                                              "QRenderBookmark");
  qmlRegisterType<glretrace::QMetric>("ApiTrace", 1, 0,
                                      "QMetric");
  qmlRegisterType<glretrace::FrameRetraceModel>("ApiTrace", 1, 0,
                                                "FrameRetrace");
  qmlRegisterType<glretrace::QMetricValue>("ApiTrace", 1, 0,
                                      "QMetricValue");
  qmlRegisterType<glretrace::QMetricsModel>("ApiTrace", 1, 0,
                                            "QMetricsModel");

  qmlRegisterType<glretrace::BarGraphView>("ApiTrace", 1, 0, "BarGraph");
  qmlRegisterType<glretrace::QSelection>("ApiTrace", 1, 0, "Selection");
  qmlRegisterType<glretrace::QShader>("ApiTrace", 1, 0, "Shader");
  qmlRegisterType<glretrace::QRenderShaders>("ApiTrace", 1, 0,
                                             "QRenderShaders");
  qmlRegisterType<glretrace::QRenderShadersList>("ApiTrace", 1, 0,
                                                 "QRenderShadersList");
  qmlRegisterType<glretrace::QApiModel>("ApiTrace", 1, 0,
                                        "QApiModel");
  qmlRegisterType<glretrace::QBatchModel>("ApiTrace", 1, 0,
                                        "QBatchModel");
  qmlRegisterType<glretrace::QExperimentModel>("ApiTrace", 1, 0,
                                               "QExperimentModel");
  qmlRegisterType<glretrace::QRenderTargetModel>("ApiTrace", 1, 0,
                                                 "QRenderTargetModel");
  qmlRegisterType<glretrace::QUniformValue>("ApiTrace", 1, 0,
                                             "QUniformValue");
  qmlRegisterType<glretrace::QUniformsModel>("ApiTrace", 1, 0,
                                             "QUniformsModel");

  glretrace::FrameImages::Create();

  int ret = -1;
  {
    QQmlApplicationEngine engine(QUrl("qrc:///qml/mainwin.qml"));
    engine.addImageProvider("myimageprovider",
                            glretrace::FrameImages::instance());
    ret = app.exec();
  }
  ::google::protobuf::ShutdownProtobufLibrary();
  Logger::Destroy();
  Socket::Cleanup();
  return ret;
}
