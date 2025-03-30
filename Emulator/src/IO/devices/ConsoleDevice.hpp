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

#ifndef _CONSOLE_IO_DEVICE_HPP
#define _CONSOLE_IO_DEVICE_HPP

#include <IO/IODevice.hpp>
#include <IO/IOInterfaceItem.hpp>

class ConsoleDevice : public IODevice, public IOInterfaceItem {
public:
    ConsoleDevice(uint64_t size, const std::string_view& data);
    virtual ~ConsoleDevice();

    virtual void InterfaceInit() override;
    virtual void InterfaceShutdown() override;

    virtual void InterfaceWrite() override;

    virtual uint8_t ReadByte(uint64_t address) override;
    virtual uint16_t ReadWord(uint64_t address) override;
    virtual uint32_t ReadDWord(uint64_t address) override;
    virtual uint64_t ReadQWord(uint64_t address) override;

    virtual void WriteByte(uint64_t address, uint8_t data) override;
    virtual void WriteWord(uint64_t address, uint16_t data) override;
    virtual void WriteDWord(uint64_t address, uint32_t data) override;
    virtual void WriteQWord(uint64_t address, uint64_t data) override;
};

#endif /* _CONSOLE_IO_DEVICE_HPP */