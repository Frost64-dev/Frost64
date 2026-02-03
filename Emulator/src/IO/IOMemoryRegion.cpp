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

#include "IODevice.hpp"
#include "IOMemoryRegion.hpp"

IOMemoryRegion::IOMemoryRegion(uint64_t start, uint64_t end, IOBus* bus)
    : MemoryRegion(start, end), m_data(bus), m_isBus(true) {
}

IOMemoryRegion::IOMemoryRegion(uint64_t start, uint64_t end, IODevice* device)
    : MemoryRegion(start, end), m_data(reinterpret_cast<IOBus*>(device)), m_isBus(false) {
}

IOMemoryRegion::~IOMemoryRegion() {
}

void IOMemoryRegion::read(uint64_t address, uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (m_isBus)
            buffer[i] = m_data.bus->ReadRegister((address - getStart() + i) / 8) & 0xFF;
        else
            buffer[i] = m_data.device->ReadByte((address - getStart() + i) / 8);
    }
}

void IOMemoryRegion::write(uint64_t address, const uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (m_isBus)
            m_data.bus->WriteRegister((address - getStart() + i) / 8, buffer[i]);
        else
            m_data.device->WriteByte((address - getStart() + i) / 8, buffer[i]);
    }
}

uint8_t IOMemoryRegion::read8(uint64_t address) {
    if (m_isBus)
        return m_data.bus->ReadRegister((address - getStart()) / 8) & 0xFF;
    return m_data.device->ReadByte((address - getStart()) / 8);
}

uint16_t IOMemoryRegion::read16(uint64_t address) {
    if (m_isBus)
        return m_data.bus->ReadRegister((address - getStart()) / 8) & 0xFFFF;
    return m_data.device->ReadWord((address - getStart()) / 8);
}

uint32_t IOMemoryRegion::read32(uint64_t address) {
    if (m_isBus)
        return m_data.bus->ReadRegister((address - getStart()) / 8) & 0xFFFF'FFFF;
    return m_data.device->ReadDWord((address - getStart()) / 8);
}

uint64_t IOMemoryRegion::read64(uint64_t address) {
    if (m_isBus)
        return m_data.bus->ReadRegister((address - getStart()) / 8);
    return m_data.device->ReadQWord((address - getStart()) / 8);
}

void IOMemoryRegion::write8(uint64_t address, uint8_t data) {
    if (m_isBus)
        m_data.bus->WriteRegister((address - getStart()) / 8, data);
    else
        m_data.device->WriteByte((address - getStart()) / 8, data);
}

void IOMemoryRegion::write16(uint64_t address, uint16_t data) {
    if (m_isBus)
        m_data.bus->WriteRegister((address - getStart()) / 8, data);
    else
        m_data.device->WriteWord((address - getStart()) / 8, data);
}

void IOMemoryRegion::write32(uint64_t address, uint32_t data) {
    if (m_isBus)
        m_data.bus->WriteRegister((address - getStart()) / 8, data);
    else
        m_data.device->WriteDWord((address - getStart()) / 8, data);
}

void IOMemoryRegion::write64(uint64_t address, uint64_t data) {
    if (m_isBus)
        m_data.bus->WriteRegister((address - getStart()) / 8, data);
    else
        m_data.device->WriteQWord((address - getStart()) / 8, data);
}

void IOMemoryRegion::dump(FILE* fp) {
    fprintf(fp, "IOMemoryRegion: %lx - %lx\n", getStart(), getEnd());
}

void IOMemoryRegion::printData(void (*write)(void* data, const char* format, ...), void* data) {
    write(data, "IOMemoryRegion: %lx - %lx\n", getStart(), getEnd());
}
