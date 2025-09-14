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

#ifndef _DISASSEMBLER_FILEBUFFER_HPP
#define _DISASSEMBLER_FILEBUFFER_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <Common/DataStructures/Buffer.hpp>

class FileBuffer : public StreamBuffer {
   public:
    FileBuffer();
    FileBuffer(FILE* file, size_t size);
    ~FileBuffer();

    // Write size bytes from data to the buffer at offset
    void WriteStream(const uint8_t* data, size_t size) override;

    // Read size bytes from the buffer at offset to data
    void ReadStream(uint8_t* data, size_t size) const override;

    void SeekStream(uint64_t offset) override;

    [[nodiscard]] uint64_t GetOffset() const override;
    [[nodiscard]] size_t GetSize() const;

   private:
    FILE* m_file;
    size_t m_size;
};

#endif /* _DISASSEMBLER_FILEBUFFER_HPP */