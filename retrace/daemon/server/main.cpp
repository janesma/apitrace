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

#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/io/coded_stream.h>

#include <getopt.h>
#include <signal.h>

#include "glframe_glhelper.hpp"
#include "glframe_logger.hpp"
#include "glframe_retrace_skeleton.hpp"
#include "glframe_socket.hpp"
#include "glretrace.hpp"

using glretrace::FrameRetraceSkeleton;
using glretrace::GlFunctions;
using glretrace::Logger;
using glretrace::ServerSocket;
using glretrace::Socket;

int parse_args(int argc, char *argv[]) {
  int opt;

  while ((opt = getopt(argc, argv, "p:h")) != -1) {
    switch (opt) {
      case 'p':
        return atoi(optarg);
      case 'h':
      default: /* '?' */
        printf("USAGE: frame_retrace_server -p port\n");
        exit(-1);
    }
  }
  exit(-1);
}


int main(int argc, char *argv[]) {
  // signal(SIGHUP, SIG_IGN);
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  GlFunctions::Init();
  Logger::Create();
  Logger::SetSeverity(glretrace::WARN);
  Logger::Begin();
  Socket::Init();
  int port = parse_args(argc, argv);
  ServerSocket sock(port);
  FrameRetraceSkeleton skel(sock.Accept());
  skel.Run();
  Socket::Cleanup();
  Logger::Destroy();
  return 0;
}
