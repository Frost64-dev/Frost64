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

#include "MemoryRegion.hpp"

#include <cstdio>

#include <Common/Util.hpp>

MemoryRegion::MemoryRegion(uint64_t start, uint64_t end) : m_start(start), m_end(end), m_size(end - start + 1) {

}

MemoryRegion::~MemoryRegion() {

}

void MemoryRegion::read8(uint64_t address, uint8_t* buffer) {
    read(address, buffer, 1);
}

void MemoryRegion::read16(uint64_t address, uint16_t* buffer) {
    read(address, reinterpret_cast<uint8_t*>(buffer), 2);
}

void MemoryRegion::read32(uint64_t address, uint32_t* buffer) {
    read(address, reinterpret_cast<uint8_t*>(buffer), 4);
}

void MemoryRegion::read64(uint64_t address, uint64_t* buffer) {
    read(address, reinterpret_cast<uint8_t*>(buffer), 8);
}

void MemoryRegion::write8(uint64_t address, const uint8_t* buffer) {
    write(address, buffer, 1);
}

void MemoryRegion::write16(uint64_t address, const uint16_t* buffer) {
    write(address, reinterpret_cast<const uint8_t*>(buffer), 2);
}

void MemoryRegion::write32(uint64_t address, const uint32_t* buffer) {
    write(address, reinterpret_cast<const uint8_t*>(buffer), 4);
}

void MemoryRegion::write64(uint64_t address, const uint64_t* buffer) {
    write(address, reinterpret_cast<const uint8_t*>(buffer), 8);
}

uint64_t MemoryRegion::getStart() {
    return m_start;
}

uint64_t MemoryRegion::getEnd() {
    return m_end;
}

size_t MemoryRegion::getSize() {
    return m_size;
}

void MemoryRegion::dump(FILE* fp) {
    fprintf(fp, "MemoryRegion: %lx - %lx", m_start, m_end);
    uint8_t buffer[16];
    uint8_t buffer_index = 0;
    uint8_t last_printed = 0;
    for (uint64_t i = m_start; i < m_end; i++, buffer_index++) {
        uint8_t data = 0;
        read8(i, &data);
        buffer[buffer_index] = data;
        if ((i - m_start) % 16 == 15) {
            buffer_index = 0;
            if (i > 16 && m_end - i > 16) {
                bool result = false;
                CMP16_B(buffer, last_printed, result);
                if (result)
                    continue; // skip printing this line
            }
            fprintf(fp, "\n%016lx: ", i - 15);
            for (uint64_t j = 0; j < 16; j++) {
                if (j == 8)
                    fprintf(fp, " ");
                fprintf(fp, "%02X ", buffer[j]);
            }
            fprintf(fp, " |");
            for (uint64_t j = 0; j < 16; j++)
                fprintf(fp, "%c", buffer[j] >= 32 && buffer[j] <= 126 ? buffer[j] : '.');
            last_printed = buffer[15];
            fprintf(fp, "|");
        }
    }
    fprintf(fp, "\n");
}

void MemoryRegion::printData(void (*write)(void* data, const char* format, ...), void* data) {
    write(data, "MemoryRegion: %lx - %lx\n", m_start, m_end);
}
