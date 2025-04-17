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

FILE* debug = nullptr;

FileBuffer::FileBuffer() : StreamBuffer(), m_file(nullptr), m_size(0) {}
FileBuffer::FileBuffer(FILE* file, size_t size) : StreamBuffer(), m_file(file), m_size(size) {}
FileBuffer::~FileBuffer() {
}

void FileBuffer::WriteStream(const uint8_t* data, size_t size) {
    if (m_file == nullptr)
        return;

    fwrite(data, 1, size, m_file);
}

void FileBuffer::ReadStream(uint8_t* data, size_t size) const {
    if (m_file == nullptr)
        return;

    fread(data, 1, size, m_file);
}

void FileBuffer::SeekStream(uint64_t offset) {
    if (m_file == nullptr)
        return;

    fseek(m_file, static_cast<int64_t>(offset), SEEK_SET);
}

uint64_t FileBuffer::GetOffset() const {
    if (m_file == nullptr)
        return 0;

    return ftell(m_file);
}

size_t FileBuffer::GetSize() const {
    return m_size;
}
