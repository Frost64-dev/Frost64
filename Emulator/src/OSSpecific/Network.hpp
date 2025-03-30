/*
Copyright (©) 2025  Frosty515

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _OS_SPECIFIC_NETWORK_HPP
#define _OS_SPECIFIC_NETWORK_HPP

#include <cstddef>

#ifdef __unix__
#include <sys/types.h>
#endif /* __unix__ */

struct TCPSocketHandle_t;

TCPSocketHandle_t* OpenTCPSocket(int port);
void CloseTCPSocket(TCPSocketHandle_t* handle);

// Read from the oldest active client
ssize_t ReadFromTCPSocket(TCPSocketHandle_t* handle, void* buffer, size_t size);

// Write to the oldest active client
ssize_t WriteToTCPSocket(TCPSocketHandle_t* handle, const void* buffer, size_t size);

#endif /* _OS_SPECIFIC_NETWORK_HPP */