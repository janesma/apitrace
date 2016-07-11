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

#include <sstream>
#include <string>

#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"
#include "glframe_os.hpp"
#include "glframe_qbargraph.hpp"
#include "glframe_retrace_images.hpp"
#include "glframe_retrace_model.hpp"
#include "glframe_retrace_skeleton.hpp"
#include "glframe_retrace_stub.hpp"
#include "glframe_socket.hpp"

using glretrace::FrameRetraceModel;
using glretrace::FrameRetraceSkeleton;
using glretrace::FrameRetraceStub;
using glretrace::GlFunctions;
using glretrace::Logger;
using glretrace::QMetric;
using glretrace::QRenderBookmark;
using glretrace::ServerSocket;

void exec_retracer(const char *main_exe, int port) {
  // frame_retrace_server should be at the same path as frame_retrace
  std::string server_exe(main_exe);
  size_t last_sep = server_exe.rfind('/');
  if (last_sep != std::string::npos)
    server_exe.resize(last_sep + 1);
  else
    server_exe = std::string("");
  server_exe += "frame_retrace_server";

  std::stringstream port_s;
  port_s << port;
  const char *const args[] = {server_exe.c_str(),
                              "-p",
                              port_s.str().c_str(),
                              NULL};
  glretrace::fork_execv(server_exe.c_str(), args);
}

Q_DECLARE_METATYPE(QList<glretrace::BarMetrics>)

int main(int argc, char *argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  GlFunctions::Init();
  Logger::Create("/tmp");
  Logger::SetSeverity(glretrace::WARN);

  int port = 0;
  {
    ServerSocket sock(0);
    port = sock.GetPort();
  }
  exec_retracer(argv[0], port);
  Logger::Begin();
  GRLOGF(glretrace::WARN, "using port: %d", port);

  FrameRetraceStub::Init(port);
  QGuiApplication app(argc, argv);
  app.setOrganizationName("Open Source Technology Center");
  app.setOrganizationDomain("intel.com");
  app.setApplicationName("frame_retrace");

  qRegisterMetaType<QList<glretrace::BarMetrics> >();
  qmlRegisterType<glretrace::QRenderBookmark>("ApiTrace", 1, 0,
                                              "QRenderBookmark");
  qmlRegisterType<glretrace::QMetric>("ApiTrace", 1, 0,
                                      "QMetric");
  qmlRegisterType<glretrace::FrameRetraceModel>("ApiTrace", 1, 0,
                                                "FrameRetrace");

  qmlRegisterType<glretrace::BarGraphView>("ApiTrace", 1, 0, "BarGraph");
  qmlRegisterType<glretrace::QSelection>("ApiTrace", 1, 0, "Selection");

  glretrace::FrameImages::Create();

  int ret = -1;
  {
    QQmlApplicationEngine engine(QUrl("qrc:///qml/mainwin.qml"));
    engine.addImageProvider("myimageprovider",
                            glretrace::FrameImages::instance());
    ret = app.exec();
  }
  FrameRetraceStub::Shutdown();
  ::google::protobuf::ShutdownProtobufLibrary();
  Logger::Destroy();
  return ret;
}
