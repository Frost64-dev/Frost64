/*
Copyright (©) 2024-2025  Frosty515

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

#include "../File.hpp"


#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <unistd.h>

#include <sys/mman.h>

#include <Emulator.hpp>

FileHandle_t GetFileHandleForStdIn() {
    return STDIN_FILENO;
}

FileHandle_t GetFileHandleForStdOut() {
    return STDOUT_FILENO;
}

FileHandle_t GetFileHandleForStdErr() {
    return STDERR_FILENO;
}

FileHandle_t OpenFile(const char* path, bool create) {
    int fd = -1;
    if (create)
        fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
    else
        fd = open(path, O_RDWR);
    if (fd < 0) {
        const char* err = strerror(errno);
        std::string str = "Failed to open file: ";
        str += path;
        str += " with error: ";
        str += err;
        Emulator::Crash(str.c_str());
    }

    return fd;
}

void CloseFile(FileHandle_t handle) {
    close(handle);
}

size_t GetFileSize(FileHandle_t handle) {
    off_t size = lseek(handle, 0, SEEK_END);
    if (size < 0) {
        const char* err = strerror(errno);
        std::string str = "Failed to get file size with error: ";
        str += err;
        Emulator::Crash(str.c_str());
    }

    return static_cast<size_t>(size);
}

size_t ReadFile(FileHandle_t handle, void* buffer, size_t size, size_t offset) {
    if (offset != SIZE_MAX) {
        if (off_t ret = lseek(handle, offset, SEEK_SET); ret < 0) {
            const char* err = strerror(errno);
            std::string str = "Failed to seek file with error: ";
            str += err;
            Emulator::Crash(str.c_str());
        }
    }

    ssize_t read_size = read(handle, buffer, size);
    if (read_size < 0) {
        const char* err = strerror(errno);
        std::string str = "Failed to read file with error: ";
        str += err;
        Emulator::Crash(str.c_str());
    }

    return static_cast<size_t>(read_size);
}

size_t WriteFile(FileHandle_t handle, const void* buffer, size_t size, size_t offset) {
    if (offset != SIZE_MAX) {
        if (off_t ret = lseek(handle, offset, SEEK_SET); ret < 0) {
            const char* err = strerror(errno);
            std::string str = "Failed to seek file with error: ";
            str += err;
            Emulator::Crash(str.c_str());
        }
    }

    ssize_t write_size = write(handle, buffer, size);
    if (write_size < 0) {
        const char* err = strerror(errno);
        std::string str = "Failed to write file with error: ";
        str += err;
        Emulator::Crash(str.c_str());
    }

    return static_cast<size_t>(write_size);
}

void* MapFile(FileHandle_t handle, size_t size, size_t offset) {
    void* address = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle, offset);
    if (address == MAP_FAILED) {
        const char* err = strerror(errno);
        std::string str = "Failed to map file with error: ";
        str += err;
        Emulator::Crash(str.c_str());
    }

    return address;
}

void UnmapFile(void* address, size_t size) {
    if (munmap(address, size) < 0) {
        const char* err = strerror(errno);
        std::string str = "Failed to unmap file with error: ";
        str += err;
        Emulator::Crash(str.c_str());
    }
}
