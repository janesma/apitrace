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

#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <sstream>

namespace glretrace {

int fork_execv(const char *path, const char *const argv[]) {
  pid_t pid = fork();
  if (pid == -1) {
    // When fork() returns -1, an error happened.
    perror("fork failed");
    exit(-1);
  }
  if (pid == 0) {
    return ::execvp(path, (char *const*) argv);
  }
  return 0;
}

struct tm *
glretrace_localtime(const time_t *timep, struct tm *result) {
  return localtime_r(timep, result);
}

int
glretrace_rand(unsigned int *seedp) {
  return rand_r(seedp);
}

void
glretrace_delay(unsigned int ms) {
  usleep(1000 * ms);
}

std::string application_cache_directory() {
  const char *homedir = "/tmp";

  struct passwd pwd, *result;
  char buf[16384];
  if ((homedir = getenv("HOME")) == NULL) {
    getpwuid_r(getuid(), &pwd, buf, 16384, &result);
    homedir = pwd.pw_dir;
  }

  std::stringstream cache_path;
  cache_path << homedir;
  cache_path << "/.frameretrace";

  mkdir(cache_path.str().c_str(), S_IRWXU);
  cache_path << "/";
  return cache_path.str();
}

}  // namespace glretrace
