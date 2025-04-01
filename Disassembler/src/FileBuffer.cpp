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

#include "FileBuffer.hpp"

#include <cstdio>

FileBuffer::FileBuffer() : Buffer(), m_file(nullptr), m_size(0) {}
FileBuffer::FileBuffer(FILE* file, size_t size) : Buffer(size), m_file(file), m_size(size) {}
FileBuffer::~FileBuffer() {
}

void FileBuffer::Write(uint64_t offset, const uint8_t* data, size_t size) {
    if (m_file == nullptr)
        return;

    int int_offset = static_cast<int>(offset);
    
    if (ftell(m_file) != int_offset)
        fseek(m_file, int_offset, SEEK_SET);

    fwrite(data, 1, size, m_file);
}

void FileBuffer::Read(uint64_t offset, uint8_t* data, size_t size) const {
    if (m_file == nullptr)
        return;

    int int_offset = static_cast<int>(offset);
    
    if (ftell(m_file) != int_offset)
        fseek(m_file, int_offset, SEEK_SET);

    fread(data, 1, size, m_file);
}
