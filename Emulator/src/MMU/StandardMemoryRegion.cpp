/*
Copyright (©) 2024-2026  Frosty515

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

#include "StandardMemoryRegion.hpp"

#include <string.h>

#include <OSSpecific/Memory.hpp>

StandardMemoryRegion::StandardMemoryRegion(uint64_t start, uint64_t end)
    : MemoryRegion(start, end) {
    m_data = static_cast<uint8_t*>(OSSpecific::AllocateCOWMemory(MemoryRegion::getSize()));
}

StandardMemoryRegion::~StandardMemoryRegion() {
    OSSpecific::FreeCOWMemory(m_data);
}

uint8_t StandardMemoryRegion::read8(uint64_t address) {
    return m_data[address - getStart()];
}

uint16_t StandardMemoryRegion::read16(uint64_t address) {
    uint16_t* buf = reinterpret_cast<uint16_t*>(m_data + (address - getStart()));
    return *buf;
}

uint32_t StandardMemoryRegion::read32(uint64_t address) {
    uint32_t* buf = reinterpret_cast<uint32_t*>(m_data + (address - getStart()));
    return *buf;
}

uint64_t StandardMemoryRegion::read64(uint64_t address) {
    uint64_t* buf = reinterpret_cast<uint64_t*>(m_data + (address - getStart()));
    return *buf;
}

void StandardMemoryRegion::write8(uint64_t address, uint8_t data) {
    m_data[address - getStart()] = data;
}

void StandardMemoryRegion::write16(uint64_t address, uint16_t data) {
    uint16_t* buf = reinterpret_cast<uint16_t*>(m_data + (address - getStart()));
    *buf = data;
}

void StandardMemoryRegion::write32(uint64_t address, uint32_t data) {
    uint32_t* buf = reinterpret_cast<uint32_t*>(m_data + (address - getStart()));
    *buf = data;
}

void StandardMemoryRegion::write64(uint64_t address, uint64_t data) {
    uint64_t* buf = reinterpret_cast<uint64_t*>(m_data + (address - getStart()));
    *buf = data;
}


void StandardMemoryRegion::read(uint64_t address, uint8_t* buffer, size_t size) {
    memcpy(buffer, m_data + (address - getStart()), size);
}

void StandardMemoryRegion::write(uint64_t address, const uint8_t* buffer, size_t size) {
    memcpy(m_data + (address - getStart()), buffer, size);
}
