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

#ifndef OS_GFSOCKET_H_
#define OS_GFSOCKET_H_

#include <string>
#include <vector>

#include "glframe_traits.hpp"

namespace glretrace {

// Socket class abstracts the simplest socket functionality for
// portability.  The intention is to have OS-specific implementations.
class Socket : NoAssign, NoCopy, NoMove {
 public:
  // Client-side constructor: connects to server.  For server-side
  // sockets, use the ServerSocket class
  Socket(const std::string &address, int port);
  ~Socket();
  static void Init();
  static void Cleanup();

  bool Read(void * buf, int size);
  template <typename T> bool Read(T *val) { return Read(val, sizeof(T)); }
  template <typename T> bool ReadVec(std::vector<T> *vec) {
    return Read(vec->data(), vec->size() * sizeof(T));
  }
  bool Write(const void * buf, int size);
  template <typename T> bool Write(const T &val) {
    return Write(&val, sizeof(val));
  }
  template <typename T> bool WriteVec(const std::vector<T> &vec) {
    return Write(vec.data(), sizeof(T) * vec.size());
  }

  const std::string &Address() const { return m_address; }

 private:
  friend class ServerSocket;
  // this constructor only called by ServerSocket, when connection is
  // accepted.
  Socket(int fd, const std::string &address)
      : m_address(address), m_socket_fd(fd) {}

  // address is stored so a server process can query the address of a
  // connected client.
  const std::string m_address;
  int m_socket_fd;
};

// ServerSocket class listens for a single connection, creating a
// server side Socket object.  The intention is to have OS-specific
// implementations.
class ServerSocket : NoAssign, NoCopy, NoMove {
 public:
  // establishes a server, waits for a client to connect
  explicit ServerSocket(int port);
  ~ServerSocket();

  Socket *Accept();

  // if 0 is passed as port, to choose an unused ephemeral port, then the
  // chosen port can be retrieved with GetPort
  int GetPort() const;
 private:
  int m_server_fd;
};

}  // namespace glretrace

#endif  // OS_GFSOCKET_H_
