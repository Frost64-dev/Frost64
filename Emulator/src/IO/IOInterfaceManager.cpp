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

#include "IOInterfaceManager.hpp"
#include "IOInterfaceItem.hpp"

#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include <Emulator.hpp>

#include <OSSpecific/File.hpp>
#include <OSSpecific/Network.hpp>

IOInterfaceManager::IOInterfaceManager() {

}

IOInterfaceManager::~IOInterfaceManager() {

}

void IOInterfaceManager::AddInterfaceItem(IOInterfaceItem* item) {
    if (item->GetType() == IOInterfaceType::UNKNOWN) {
        // get the type from the string data
        std::string_view data = item->GetStringData();
        if (data == "stdio")
            item->SetType(IOInterfaceType::STDIO);
        else if (data.starts_with("file:"))
            item->SetType(IOInterfaceType::FILE);
        else if (data.starts_with("port:"))
            item->SetType(IOInterfaceType::NETWORK);
    }

    std::string_view name;
    if (item->GetType() == IOInterfaceType::FILE || item->GetType() == IOInterfaceType::NETWORK) {
        // need to remove the prefix
        name = item->GetStringData().substr(item->GetStringData().find(':') + 1);
    }

    switch (item->GetType()) {
    case IOInterfaceType::STDIO:
        // nothing to do
        break;
    case IOInterfaceType::FILE:
        item->SetRawData(FILE_HANDLE_TO_VOID_PTR(OpenFile(name.data(), true))); // OpenFile already crashes on error
        break;
    case IOInterfaceType::NETWORK:
        item->SetRawData(OpenTCPSocket(static_cast<int>(strtol(name.data(), nullptr, 10))));
        break;
    case IOInterfaceType::UNKNOWN:
        Emulator::Crash("Unknown IO interface type");
        break;
    }
}

void IOInterfaceManager::RemoveInterfaceItem(IOInterfaceItem*) {
}

void IOInterfaceManager::Read(IOInterfaceItem* item, void* buffer, size_t size) {
    if (item == nullptr)
        return;

    switch (item->GetType()) {
    case IOInterfaceType::STDIO:
        ReadFile(GetFileHandleForStdIn(), buffer, size, SIZE_MAX);
        break;
    case IOInterfaceType::FILE:
        ReadFile(VOID_PTR_TO_FILE_HANDLE(item->GetRawData()), buffer, size, SIZE_MAX);
        break;
    case IOInterfaceType::NETWORK:
        ReadFromTCPSocket(reinterpret_cast<TCPSocketHandle_t*>(item->GetRawData()), buffer, size);
        break;
    case IOInterfaceType::UNKNOWN:
        break;
    }
}

void IOInterfaceManager::Write(IOInterfaceItem* item, const void* buffer, size_t size) {
    if (item == nullptr)
        return;

    switch (item->GetType()) {
    case IOInterfaceType::STDIO:
        WriteFile(GetFileHandleForStdOut(), buffer, size, SIZE_MAX);
        break;
    case IOInterfaceType::FILE:
        WriteFile(VOID_PTR_TO_FILE_HANDLE(item->GetRawData()), buffer, size, SIZE_MAX);
        break;
    case IOInterfaceType::NETWORK:
        WriteToTCPSocket(reinterpret_cast<TCPSocketHandle_t*>(item->GetRawData()), buffer, size);
        break;
    case IOInterfaceType::UNKNOWN:
        break;
    }
}

void IOInterfaceManager::Write(IOInterfaceItem* item, const char* buffer) {
    Write(item, buffer, strlen(buffer));
}

void IOInterfaceManager::WriteFormatted(IOInterfaceItem* item, const char* format, ...) {
    va_list args;
    va_start(args, format);

    int len = vsnprintf(nullptr, 0, format, args);
    va_end(args);

    va_start(args, format);

    char* buffer = new char[len + 1];
    vsnprintf(buffer, len + 1, format, args);

    Write(item, buffer);

    delete[] buffer;

}

void IOInterfaceManager::WritevFormatted(IOInterfaceItem* item, const char* format, va_list args) {
    va_list argsCopy;
    va_copy(argsCopy, args);

    int len = vsnprintf(nullptr, 0, format, argsCopy);

    va_end(argsCopy);
    va_copy(argsCopy, args);

    char* buffer = new char[len + 1];
    vsnprintf(buffer, len + 1, format, args);

    va_end(argsCopy);

    Write(item, buffer);

    delete[] buffer;
}

IOInterfaceManager* g_IOInterfaceManager = nullptr;