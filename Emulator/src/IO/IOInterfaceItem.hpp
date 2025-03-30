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

#ifndef _IO_INTERFACE_ITEM_HPP
#define _IO_INTERFACE_ITEM_HPP

#include <string_view>

enum class IOInterfaceType {
    STDIO,
    FILE,
    NETWORK,
    UNKNOWN
};

class IOInterfaceItem {
public:
    IOInterfaceItem() : m_type(IOInterfaceType::UNKNOWN), m_rawData(nullptr) {}
    explicit IOInterfaceItem(IOInterfaceType type, const std::string_view& data = "") : m_type(type), m_data(data), m_rawData(nullptr) {}
    virtual ~IOInterfaceItem() = default;

    virtual void InterfaceInit() = 0;
    virtual void InterfaceShutdown() = 0;

    // handle a write operation
    virtual void InterfaceWrite() = 0;

    [[nodiscard]] IOInterfaceType GetType() const { return m_type; }
    void SetType(IOInterfaceType type) { m_type = type; }

    [[nodiscard]] std::string_view GetStringData() const { return m_data; }
    void SetStringData(const std::string_view& data) { m_data = data; }

    [[nodiscard]] void* GetRawData() const { return m_rawData; }
    void SetRawData(void* data) { m_rawData = data; }

private:
    IOInterfaceType m_type;
    std::string_view m_data;
    void* m_rawData;
};

#endif /* _IO_INTERFACE_ITEM_HPP */