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

#include "BIOSMemoryRegion.hpp"

#include <cstdio>

BIOSMemoryRegion::BIOSMemoryRegion(uint64_t start, uint64_t end, uint64_t real_size) : StandardMemoryRegion(start, end), m_real_size(real_size) {

}

BIOSMemoryRegion::~BIOSMemoryRegion() {

}

void BIOSMemoryRegion::dump(FILE* fp) {
    fprintf(fp, "BIOSMemoryRegion: %lx - %lx, real_size = %lx\n", getStart(), getEnd(), m_real_size);
    // uint8_t buffer[16];
    // uint8_t buffer_index = 0;
    // for (uint64_t i = getStart(); i < m_real_size; i++, buffer_index++) {
    //     if ((i - getStart()) % 16 == 0) {
    //         buffer_index = 0;
    //         fprintf(fp, "\n%016lx: ", i);
    //     }
    //     else if ((i - getStart()) % 8 == 0)
    //         fprintf(fp, " ");
    //     uint8_t data = 0;
    //     read8(i, &data);
    //     buffer[buffer_index] = data;
    //     fprintf(fp, "%02X ", data);
    //     if ((i - getStart()) % 16 == 15) {
    //         fprintf(fp, " |");
    //         for (uint64_t j = 0; j < 16; j++)
    //             fprintf(fp, "%c", buffer[j] >= 32 && buffer[j] <= 126 ? buffer[j] : '.');
    //         fprintf(fp, "|");
    //     }
    // }
    // fprintf(fp, "\n");
}

void BIOSMemoryRegion::printData(void (*write)(void* data, const char* format, ...), void* data) {
    write(data, "BIOSMemoryRegion: %lx - %lx, real_size = %lx\n", getStart(), getEnd(), m_real_size);
}