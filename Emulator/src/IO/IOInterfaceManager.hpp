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

#ifndef _IO_INTERFACE_MANAGER_HPP
#define _IO_INTERFACE_MANAGER_HPP

#include <cstddef>
#include <vector>

#include "IOInterfaceItem.hpp"

class IOInterfaceManager {
public:
    IOInterfaceManager();
    ~IOInterfaceManager();

    void AddInterfaceItem(IOInterfaceItem* item);
    void RemoveInterfaceItem(IOInterfaceItem* item);

    // Read data from the interface type of the item
    void Read(IOInterfaceItem* item, void* buffer, size_t size);
    // Write data to the interface type of the item
    void Write(IOInterfaceItem* item, const void* buffer, size_t size);

    // Write string data to the interface type of the item
    void Write(IOInterfaceItem* item, const char* buffer);

    // Format and write string data to the interface type of the item
    void WriteFormatted(IOInterfaceItem* item, const char* format, ...);
    void WritevFormatted(IOInterfaceItem* item, const char* format, va_list args);


private:
    std::vector<IOInterfaceItem*> m_interfaceItems;
};

extern IOInterfaceManager* g_IOInterfaceManager;

#endif /* _IO_INTERFACE_MANAGER_HPP */