// Copyright (c) 2014 Sami Väisänen, Ensisoft 
//
// http://www.ensisoft.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#pragma once

#include <newsflash/config.h>

#if defined(WINDOWS_OS)
#  include <windows.h>
#  include <winsock.h>
#elif defined(LINUX_OS)
#  include <sys/socket.h>
#endif

#include <utility> // for pair
#include "types.h"

namespace newsflash
{
    // resolve the host name and return the first address in network byte order
    // or 0 if host could not be resolved. (use socket_error() to query for error)
    ipv4_addr_t resolve_host_ipv4(const std::string& hostname);

    // initiate a new stream (TCP) based connection to the given host.
    // returns the new socket and wait object handles. The socket is 
    // initially non-blocking. Check the wait handle for completion of the connection attempt.
    std::pair<native_socket_t, native_handle_t> begin_socket_connect(ipv4_addr_t host, uint16_t port);

    native_handle_t get_wait_handle(native_socket_t sock);

    // complete the previously started connection attempt. 
    // makes the socket blocking again and returns a platform specific
    // error code indicating the status of the connection attempt.
    native_errcode_t complete_socket_connect(native_handle_t handle, native_socket_t sock);

    void closesocket(native_handle_t handle, native_socket_t sock);

    void closesocket(native_socket_t sock);
    
    native_errcode_t get_last_socket_error();

} // newsflash
