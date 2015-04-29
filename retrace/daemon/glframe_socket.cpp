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

#include "glframe_socket.hpp"

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

using glretrace::Socket;
using glretrace::ServerSocket;

bool
Socket::Read(void * buf, int size) {
  int bytes_remaining = size;
  void *curPtr = buf;
  while (bytes_remaining > 0) {
    int bytes_read = ::read(m_socket_fd, curPtr, bytes_remaining);
    if (bytes_read <= 0)
      return false;
    bytes_remaining -= bytes_read;
    curPtr = reinterpret_cast<char*>(curPtr) + bytes_read;
  }
  return true;
}

bool
Socket::Write(const void * buf, int size) {
  int bytes_remaining = size;
  const void *curPtr = buf;
  while (bytes_remaining > 0) {
    ssize_t bytes_written = ::send(m_socket_fd, curPtr,
                                   bytes_remaining,
                                   0);  // default flags

    if (bytes_written < 0) {
      if (errno == EINTR)
        continue;
      return false;
    }
    bytes_remaining -= bytes_written;
    curPtr = reinterpret_cast<const char*>(curPtr) + bytes_written;
    // ASKME: Should we sleep here if bytes_remaining > 0?
  }
  return true;
}

class FreeAddrInfo {
 public:
  explicit FreeAddrInfo(addrinfo *p) : m_p(p) {}
  ~FreeAddrInfo() { freeaddrinfo(m_p); }
 private:
  addrinfo *m_p;
};

Socket::Socket(const std::string &address, int port)
    : m_address(address) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof (hints));
  hints.ai_family = AF_INET;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo * resolved_address = NULL;

  int result = 0;
  result = getaddrinfo(address.c_str(), NULL, &hints, &resolved_address);

  assert(result == 0);

  FreeAddrInfo a(resolved_address);

  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(fd);

  const int nodelay_flag = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay_flag,
             sizeof(nodelay_flag));

  struct sockaddr_in *ip_address =
      reinterpret_cast<sockaddr_in *>(resolved_address->ai_addr);
  ip_address->sin_port = htons(port);

  result = ::connect(fd, resolved_address->ai_addr,
                     static_cast<int>(resolved_address->ai_addrlen));

  assert(0 == result);

  // TODO(majanes): need to raise here if couldn't connect

  m_socket_fd = fd;
}

Socket::~Socket() {
  close(m_socket_fd);
}

ServerSocket::ServerSocket(int port) {
  m_server_fd = socket(PF_INET, SOCK_STREAM, 0);
  assert(m_server_fd != -1);

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  const int flag = 1;
  setsockopt( m_server_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag) );

  int bind_result;
  for (int retry = 0; retry < 5; ++retry) {
    bind_result = bind(m_server_fd, (struct sockaddr *) &address,
                       sizeof(struct sockaddr_in));
    if (bind_result != -1)
      break;

    // delay, retry
    usleep(100000);
    perror("bind failure: ");
  }
  assert(bind_result != -1);
  // todo raise on error

  const int backlog = 1;  // single connection
  const int listen_result = listen(m_server_fd, backlog);
  assert(listen_result != -1);
  // todo raise on error
}

Socket *
ServerSocket::Accept() {
  socklen_t client_addr_size = sizeof(struct sockaddr_in);
  struct sockaddr_in client_address;
  int socket_fd = accept(m_server_fd, (struct sockaddr *) &client_address,
                         &client_addr_size);

  assert(socket_fd != -1);
  // todo raise on error

  // now that we have a connected server socket, we don't need to listen for
  // subsequent connections.

  return new Socket(socket_fd, inet_ntoa(client_address.sin_addr));
}

ServerSocket::~ServerSocket() {
  close(m_server_fd);
}

int
ServerSocket::GetPort() const {
  struct sockaddr_in addr;
  socklen_t l = sizeof(addr);
  const int result = getsockname(m_server_fd,
                                 reinterpret_cast<sockaddr*>(&addr),
                                 &l);
  assert(result == 0);
  return ntohs(addr.sin_port);
}
