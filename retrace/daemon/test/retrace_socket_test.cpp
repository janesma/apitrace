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

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "glframe_os.hpp"
#include "glframe_socket.hpp"
#include "glframe_thread.hpp"

using glretrace::Socket;
using glretrace::ServerSocket;
using glretrace::Thread;
using glretrace::glretrace_rand;

class TestServer : public Thread {
 public:
  explicit TestServer(const std::vector<uint8_t> b)
      : Thread("gtest server thread"),
        m_buf(b),
        m_sock(0) {}
  ~TestServer() {}
  int GetPort() { return m_sock.GetPort(); }
  void Run() {
    Socket *s = m_sock.Accept();
    std::vector<uint8_t> read_buf;
    read_buf.resize(10000);
    s->Read(read_buf.data(), 10000);
    EXPECT_EQ(read_buf, m_buf);

    s->Write(read_buf.data(), 10000);
    delete s;
  }

 private:
  std::vector<uint8_t> m_buf;
  ServerSocket m_sock;
};

TEST(Thread, Socket_Read_Write) {
  Socket::Init();
  std::vector<uint8_t> buf;
  buf.reserve(10000);
  unsigned int seed = 1;
  for (int i = 0; i < 10000; ++i) {
    buf.push_back(glretrace_rand(&seed));
  }

  TestServer server(buf);
  server.Start();

  Socket s("localhost", server.GetPort());

  for (int i = 0; i < 10000; ++i) {
    s.Write(&(buf[i]), 1);
  }

  std::vector<uint8_t> buf2;
  buf2.resize(10000);
  s.Read(buf2.data(), 10000);
  EXPECT_EQ(buf, buf2);
  server.Join();
  Socket::Cleanup();
}

