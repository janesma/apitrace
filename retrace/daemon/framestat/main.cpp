// Copyright (C) Intel Corp.  2016.  All Rights Reserved.

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

#include <getopt.h>

#include <string>
#include <vector>

#include "glframe_loop.hpp"

using glretrace::FrameLoop;

int main(int argc, char *argv[]) {
  int loop_count = 0;
  std::string frame_file, out_file;
  std::vector<int> frames;
  const char *usage = "USAGE: framestat -n {loop_count} [-o {out_file}]"
                      "-f {trace} frame_1 frame_2 ... frame_n\n";
  int opt;
  while ((opt = getopt(argc, argv, "n:f:p:o:h")) != -1) {
    switch (opt) {
      case 'n':
        loop_count = atoi(optarg);
        continue;
      case 'f':
        frame_file = optarg;
        continue;
      case 'o':
        out_file = optarg;
        continue;
      case 'h':
      default: /* '?' */
        printf("%s", usage);
        return 0;
    }
  }

  for (int index = optind; index < argc; index++) {
    frames.push_back(atoi(argv[index]));
  }
  if (FILE *file = fopen(frame_file.c_str(), "r")) {
    fclose(file);
  } else {
    printf("ERROR: frame file not found: %s\n", frame_file.c_str());
    printf("%s", usage);
    return -1;
  }
  if (loop_count == 0) {
    printf("ERROR: loop count not specified.\n");
    printf("%s", usage);
    return -1;
  }
  if (frames.empty()) {
    printf("ERROR: target frames not specified.\n");
    printf("%s", usage);
    return -1;
  }

  FrameLoop loop(frame_file, out_file);
  for (auto f : frames) {
    loop.advanceToFrame(f);
    loop.loop(loop_count);
  }
  return 0;
}
