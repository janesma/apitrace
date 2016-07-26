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

#ifndef OS_GFLOGGER_H_
#define OS_GFLOGGER_H_

#include <stdarg.h>
#include <stdio.h>

#include <mutex>
#include <queue>
#include <string>

#include "glframe_os.hpp"
#include "glframe_thread.hpp"

namespace glretrace {

enum Severity {
  DEBUG,
  INFO,
  WARN,
  ERR
};

class Logger : public Thread {
 public:
  static void Create();
  static void Begin();
  static void Destroy();
  static void Log(Severity s, const std::string &file, int line,
                  const std::string &message);
  static void GetLog(std::string *out);
  static void Flush();
  static void SetSeverity(Severity s);
  void Run();

 private:
  explicit Logger(const std::string &out_path);
  ~Logger();
  void Stop();

  static Logger *m_instance;
  std::queue<std::string> m_q;
  std::mutex m_protect;
  Semaphore m_sem;
  FILE *m_fh, *m_read_fh;
  int m_severity;
  bool m_running;
};

}  // namespace glretrace

inline void glretrace_log_message(glretrace::Severity s,
                                  const char *file,
                                  int line,
                                  const char *format, ... ) {
  static const int BUF_SIZE = 16384;
  char buf[BUF_SIZE];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf, BUF_SIZE, format, ap);
  va_end(ap);
  glretrace::Logger::Log(s, file, line, buf);
}

#define GRLOGF(sev, format, ...) glretrace_log_message(sev, __FUNCTION__, \
                                                       __LINE__,        \
                                                       format,          \
                                                       __VA_ARGS__);
#define GRLOG(sev, format) glretrace_log_message(sev, __FUNCTION__, __LINE__, \
                                                 format);

#endif  // OS_GFLOGGER_H_
