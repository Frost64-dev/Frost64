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

#include "SystemControlMemoryRegion.hpp"

#include <cstddef>
#include <cstdint>
#include <IO/IOMemoryRegion.hpp>

#include "Emulator.hpp"
#include "Exceptions.hpp"
#include "MemoryControlSystem.hpp"

SystemControlMemoryRegion::SystemControlMemoryRegion(uint64_t start, uint64_t end, IOBus* bus, uint64_t RAMSize, MMU* mmu) : MemoryRegion(start, end), m_IORegion(start + 0x10, start + 0x40, bus), m_MemControl(RAMSize, mmu) {

}

SystemControlMemoryRegion::~SystemControlMemoryRegion() {

}

void SystemControlMemoryRegion::read(uint64_t address, uint8_t* buffer, size_t size) {
    if (uint64_t offset = address - getStart(); offset < 0x10) {
        for (size_t i = 0; i < size; i++)
            buffer[i] = ReadRegister((offset + i) / 8) >> ((address + i) % 8 * 8) & 0xFF;
    } else if (offset < 0x40)
        m_IORegion.read(address, buffer, size);
    else if (offset < 0x70) {
        for (size_t i = 0; i < size; i++)
            buffer[i] = m_MemControl.ReadRegister((offset + i) / 8) >> ((address + i) % 8 * 8) & 0xFF;
    } else
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);
}

void SystemControlMemoryRegion::write(uint64_t address, const uint8_t* buffer, size_t size) {
    if (uint64_t offset = address - getStart(); offset < 0x10) {
        for (size_t i = 0; i < size; i++) {
            uint64_t currentData = ReadRegister((offset + i) / 8);
            currentData &= ~(0xFFULL << ((address + i) % 8 * 8));
            currentData |= static_cast<uint64_t>(buffer[i]) << ((address + i) % 8 * 8);
            WriteRegister((offset + i) / 8, currentData);
        }
    } else if (offset < 0x40)
        m_IORegion.write(address, buffer, size);
    else if (offset < 0x70) {
        for (size_t i = 0; i < size; i++) {
            uint64_t currentData = ReadRegister((offset + i) / 8);
            currentData &= ~(0xFFULL << ((address + i) % 8 * 8));
            currentData |= static_cast<uint64_t>(buffer[i]) << ((address + i) % 8 * 8);
            m_MemControl.WriteRegister((offset + i) / 8, currentData);
        }
    }
    else
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);
}

#define SYSCTRLReadFunc(size) \
void SystemControlMemoryRegion::read##size(uint64_t address, uint##size##_t* buffer) {  \
    if (uint64_t offset = address - getStart(); offset < 0x10)                          \
        *buffer = ReadRegister(offset / 8);                                             \
    else if (offset < 0x40)                                                             \
        m_IORegion.read##size(address, buffer);                                         \
    else if (offset < 0x70)                                                             \
        *buffer = m_MemControl.ReadRegister((offset - 0x40) / 8);                       \
    else                                                                                \
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);     \
}

#define SYSCTRLWriteFunc(size) \
void SystemControlMemoryRegion::write##size(uint64_t address, const uint##size##_t* buffer) {  \
    if (uint64_t offset = address - getStart(); offset < 0x10)                                 \
        WriteRegister(offset / 8, *buffer);                                                    \
    else if (offset < 0x40)                                                                    \
        m_IORegion.write##size(address, buffer);                                               \
    else if (offset < 0x70)                                                                    \
        m_MemControl.WriteRegister((offset - 0x40) / 8, *buffer);                              \
    else                                                                                       \
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);            \
}

SYSCTRLReadFunc(8)
SYSCTRLReadFunc(16)
SYSCTRLReadFunc(32)
SYSCTRLReadFunc(64)
SYSCTRLWriteFunc(8)
SYSCTRLWriteFunc(16)
SYSCTRLWriteFunc(32)
SYSCTRLWriteFunc(64)

#undef SYSCTRLReadFunc
#undef SYSCTRLWriteFunc

uint64_t SystemControlMemoryRegion::ReadRegister(uint64_t offset) {
    (void)offset;
    return 0;
}

void SystemControlMemoryRegion::WriteRegister(uint64_t offset, uint64_t data) {
    (void)offset;
    (void)data;
}
