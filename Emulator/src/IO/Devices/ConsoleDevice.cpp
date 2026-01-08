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

#include "ConsoleDevice.hpp"

#include <Emulator.hpp>

#include <IO/IOInterfaceManager.hpp>

#ifdef EMULATOR_DEBUG
#include <stdio.h>
#endif

ConsoleDevice::ConsoleDevice(uint64_t size, const std::string_view& data)
    : IODevice(IODeviceID::CONSOLE, size, 0), IOInterfaceItem(IOInterfaceType::UNKNOWN, data) {
}

ConsoleDevice::~ConsoleDevice() {
}

void ConsoleDevice::InterfaceInit() {
}

void ConsoleDevice::InterfaceShutdown() {
}

void ConsoleDevice::InterfaceWrite() {
}

uint8_t ConsoleDevice::ReadByte(uint64_t address) {
#ifdef EMULATOR_DEBUG
    printf("ConsoleDevice::ReadByte(%lu)\n", address);
#else
    (void)address;
#endif
    uint8_t data = 0;
    g_IOInterfaceManager->Read(this, &data, 1);
    return data;
}

uint16_t ConsoleDevice::ReadWord(uint64_t address) {
#ifdef EMULATOR_DEBUG
    printf("ConsoleDevice::ReadWord(%lu)\n", address);
#else
    (void)address;
#endif
    return 0;
}

uint32_t ConsoleDevice::ReadDWord(uint64_t address) {
#ifdef EMULATOR_DEBUG
    printf("ConsoleDevice::ReadDWord(%lu)\n", address);
#else
    (void)address;
#endif
    return 0;
}

uint64_t ConsoleDevice::ReadQWord(uint64_t address) {
#ifdef EMULATOR_DEBUG
    printf("ConsoleDevice::ReadQWord(%lu)\n", address);
#else
    (void)address;
#endif
    return 0;
}

void ConsoleDevice::WriteByte(uint64_t address, uint8_t data) {
#ifdef EMULATOR_DEBUG
    printf("ConsoleDevice::WriteByte(%lu, %hhu)\n", address, data);
#else
    (void)address;
#endif
    g_IOInterfaceManager->Write(this, &data, 1);
}

void ConsoleDevice::WriteWord(uint64_t address, uint16_t data) {
#ifdef EMULATOR_DEBUG
    printf("ConsoleDevice::WriteWord(%lu, %hu)\n", address, data);
#else
    (void)address;
#endif
    (void)data;
}

void ConsoleDevice::WriteDWord(uint64_t address, uint32_t data) {
#ifdef EMULATOR_DEBUG
    printf("ConsoleDevice::WriteDWord(%lu, %u)\n", address, data);
#else
    (void)address;
#endif
    (void)data;
}

void ConsoleDevice::WriteQWord(uint64_t address, uint64_t data) {
#ifdef EMULATOR_DEBUG
    printf("ConsoleDevice::WriteQWord(%lu, %lx)\n", address, data);
#else
    (void)address;
#endif
    (void)data;
}
