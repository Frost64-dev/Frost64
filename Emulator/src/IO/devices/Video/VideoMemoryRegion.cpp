/*
Copyright (©) 2024  Frosty515

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

#include "VideoMemoryRegion.hpp"

#include <stdio.h>

VideoMemoryRegion::VideoMemoryRegion(uint64_t start, uint64_t end, void (*operationCallback)(bool write, uint64_t address, uint8_t* buffer, size_t size, void* data), void* data) : MemoryRegion(start, end), m_operationCallback(operationCallback), m_data(data) {

}

VideoMemoryRegion::~VideoMemoryRegion() {

}

void VideoMemoryRegion::read(uint64_t address, uint8_t* buffer, size_t size) {
    m_operationCallback(false, address - getStart(), buffer, size, m_data);
}

void VideoMemoryRegion::write(uint64_t address, const uint8_t* buffer, size_t size) {
    m_operationCallback(true, address - getStart(), const_cast<uint8_t*>(buffer), size, m_data);
}

void VideoMemoryRegion::dump(FILE* fp) {
    fprintf(fp, "VideoMemoryRegion: %lx-%lx\n", getStart(), getEnd());
}