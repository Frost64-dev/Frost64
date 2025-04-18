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

#include "../Network.hpp"

#include <cerrno>
#include <cstring>
#include <sstream>
#include <thread>
#include <unistd.h>
#include <vector>

#include <netinet/in.h>

#include <sys/socket.h>

#include <Common/Spinlock.hpp>

#include <Emulator.hpp>

struct TCPSocketHandle_t {
    int sockfd;

    std::thread* acceptThread;

    std::vector<int> clientSockets;
    spinlock_t clientSocketsLock;
};

void SocketWaitThread(TCPSocketHandle_t* handle) {
    while (true) {
        int client_sockfd = accept(handle->sockfd, nullptr, nullptr);
        if (client_sockfd < 0) {
            std::stringstream ss = std::stringstream();
            ss << "Failed to accept TCP socket with error: " << strerror(errno);
            Emulator::Crash(ss.str().c_str());
        }

        spinlock_acquire(&handle->clientSocketsLock);
        handle->clientSockets.push_back(client_sockfd);
        spinlock_release(&handle->clientSocketsLock);
    }
}

TCPSocketHandle_t* OpenTCPSocket(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::stringstream ss = std::stringstream();
        ss << "Failed to open TCP socket with error: " << strerror(errno);
        Emulator::Crash(ss.str().c_str());
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::stringstream ss = std::stringstream();
        ss << "Failed to bind TCP socket with error: " << strerror(errno);
        Emulator::Crash(ss.str().c_str());
    }

    if (listen(sockfd, 5) < 0) {
        std::stringstream ss = std::stringstream();
        ss << "Failed to listen on TCP socket with error: " << strerror(errno);
        Emulator::Crash(ss.str().c_str());
    }

    TCPSocketHandle_t* handle = new TCPSocketHandle_t();
    handle->sockfd = sockfd;
    spinlock_init(&handle->clientSocketsLock);

    // wait for at least one client
    int client_sockfd = accept(handle->sockfd, nullptr, nullptr);
    if (client_sockfd < 0) {
        std::stringstream ss = std::stringstream();
        ss << "Failed to accept TCP socket with error: " << strerror(errno);
        Emulator::Crash(ss.str().c_str());
    }

    handle->clientSockets.push_back(client_sockfd);

    handle->acceptThread = new std::thread(SocketWaitThread, handle);

    return handle;
}

void CloseTCPSocket(TCPSocketHandle_t* handle) {
    handle->acceptThread->detach();
    delete handle->acceptThread;

    spinlock_acquire(&handle->clientSocketsLock);
    for (int client_sockfd : handle->clientSockets)
        close(client_sockfd);
    handle->clientSockets.clear();

    // don't release the lock, we're about to delete the handle

    close(handle->sockfd);
    delete handle;
}

ssize_t ReadFromTCPSocket(TCPSocketHandle_t* handle, void* buffer, size_t size) {
    spinlock_acquire(&handle->clientSocketsLock);
    if (handle->clientSockets.empty()) {
        spinlock_release(&handle->clientSocketsLock);
        return -1;
    }

    int client_sockfd = handle->clientSockets.front();
    spinlock_release(&handle->clientSocketsLock);

    ssize_t read_size = read(client_sockfd, buffer, size);
    if (read_size < 0) {
        switch (errno) {
            case ECONNRESET:
            case ENOTCONN:
            case ETIMEDOUT: {
                spinlock_acquire(&handle->clientSocketsLock);
                close(handle->clientSockets.front());
                handle->clientSockets.erase(handle->clientSockets.begin());
                spinlock_release(&handle->clientSocketsLock);
                return ReadFromTCPSocket(handle, buffer, size); // try again
            }
            default: {
                std::stringstream ss = std::stringstream();
                ss << "Failed to read from TCP socket with error: " << strerror(errno);
                Emulator::Crash(ss.str().c_str());
            }
        }
    }

    return read_size;
}

ssize_t WriteToTCPSocket(TCPSocketHandle_t* handle, const void* buffer, size_t size) {
    spinlock_acquire(&handle->clientSocketsLock);
    if (handle->clientSockets.empty()) {
        spinlock_release(&handle->clientSocketsLock);
        return -1;
    }

    int client_sockfd = handle->clientSockets.front();
    spinlock_release(&handle->clientSocketsLock);

    ssize_t write_size = write(client_sockfd, buffer, size);
    if (write_size < 0) {
        switch (errno) {
            case ECONNRESET:
            case ENOTCONN:
            case ETIMEDOUT: {
                spinlock_acquire(&handle->clientSocketsLock);
                close(handle->clientSockets.front());
                handle->clientSockets.erase(handle->clientSockets.begin());
                spinlock_release(&handle->clientSocketsLock);
                return WriteToTCPSocket(handle, buffer, size); // try again
            }
            default: {
                std::stringstream ss = std::stringstream();
                ss << "Failed to write to TCP socket with error: " << strerror(errno);
                Emulator::Crash(ss.str().c_str());
            }
        }
    }

    return write_size;
}