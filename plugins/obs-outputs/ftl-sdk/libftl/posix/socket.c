/**
* \file socket.c - Windows Socket Abstractions
*
* Copyright (c) 2015 Michael Casadevall
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
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
**/

#include "ftl.h"
#include "ftl_private.h"

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <poll.h>

void init_sockets() {
  //BSD sockets are smarter and don't need silly init
}

int close_socket(SOCKET sock) {
  return close(sock);
}

int shutdown_socket(SOCKET sock, int how) {
  return shutdown(sock, how);
}

char * get_socket_error() {
  return strerror(errno);
}

int set_socket_recv_timeout(SOCKET socket, int ms_timeout){
  struct timeval tv = {0};

  while (ms_timeout >= 1000) {
    tv.tv_sec++;
    ms_timeout -= 1000;
  }
  tv.tv_usec = ms_timeout * 1000;

  return setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv));  
}

int set_socket_send_timeout(SOCKET socket, int ms_timeout){
  struct timeval tv = {0};

  while (ms_timeout >= 1000) {
    tv.tv_sec++;
    ms_timeout -= 1000;
  }
  tv.tv_usec = ms_timeout * 1000;

  return setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
}

int set_socket_enable_keepalive(SOCKET socket){
  int keep_alive = 1;
  return setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&keep_alive, sizeof(keep_alive));
}

int get_socket_send_buf(SOCKET socket, int *buffer_space) {
  int len = sizeof(*buffer_space);
  return getsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char*)buffer_space, &len);
}

int set_socket_send_buf(SOCKET socket, int buffer_space) {
  return setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char*)&buffer_space, sizeof(buffer_space));
}

int get_socket_bytes_available(SOCKET socket, unsigned long *bytes_available) {
  return ioctl(socket, FIONREAD, bytes_available);
}

int poll_socket_for_receive(SOCKET socket, int timeoutMs)
{
  // timeoutMs behavior
  //    > 0 time in ms to wait
  //    = 0 return instantly
  //    < 0 wait forever

  struct pollfd fd;
  fd.fd = socket;
  fd.events = POLLIN;

  int ret = poll(&fd, 1, timeoutMs);

  // Function return values
  //    = 0 timeout reached
  //    = 1 data received
  //    = -1 socket error
  if (ret == 0)
  {
    return 0;
  }
  else if (ret == 1 && fd.revents == POLLIN)
  {
    return 1;
  }
  else
  {
    return SOCKET_ERROR;
  }
}
