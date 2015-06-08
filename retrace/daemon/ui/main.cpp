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

#include <unistd.h>

#include <sstream>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>

#include "glframe_os.hpp"
#include "glframe_retrace_model.hpp"
#include "glframe_retrace_skeleton.hpp"
#include "glframe_retrace_stub.hpp"
#include "glframe_socket.hpp"
#include "glframe_retrace_images.hpp"

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>


using glretrace::FrameRetraceModel;
using glretrace::FrameRetraceSkeleton;
using glretrace::FrameRetraceStub;
using glretrace::QRenderBookmark;
using glretrace::ServerSocket;

int fork_retracer() {
  ServerSocket sock(0);
  pid_t pid = fork();
  //pid_t pid = 0;
  if (pid == -1) {
    // When fork() returns -1, an error happened.
    perror("fork failed");
    exit(-1);
  }
  if (pid == 0) {
    // child: create retrace skeleton
    FrameRetraceSkeleton skel(sock.Accept());
    skel.Run();
    exit(0);  // exit() is unreliable here, so _exit must be used
  }
  //exit(0);  // exit() is unreliable here, so _exit must be used
  return sock.GetPort();
}

int main(int argc, char *argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    const int port = fork_retracer();

    FrameRetraceStub::Init(port);
    
    QGuiApplication app(argc, argv);

    qmlRegisterType<glretrace::QRenderBookmark>("ApiTrace", 1, 0, "QRenderBookmark");
    qmlRegisterType<glretrace::FrameRetraceModel>("ApiTrace", 1, 0, "FrameRetrace");

    glretrace::FrameImages::Create();
    
    QQmlApplicationEngine engine(QUrl("qrc:///qml/mainwin.qml"));
    engine.addImageProvider("myimageprovider", glretrace::FrameImages::instance());
    int ret = app.exec();

    FrameRetraceStub::Shutdown();
    ::google::protobuf::ShutdownProtobufLibrary();
    glretrace::FrameImages::Destroy();
    return ret;
}
