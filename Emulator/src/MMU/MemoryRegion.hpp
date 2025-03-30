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

#ifndef _MEMORY_REGION_HPP
#define _MEMORY_REGION_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>

class MemoryRegion {
   public:
    MemoryRegion(uint64_t start, uint64_t end);
    virtual ~MemoryRegion();

    virtual void read(uint64_t address, uint8_t* buffer, size_t size) = 0;
    virtual void write(uint64_t address, const uint8_t* buffer, size_t size) = 0;

    virtual void read8(uint64_t address, uint8_t* buffer);
    virtual void read16(uint64_t address, uint16_t* buffer);
    virtual void read32(uint64_t address, uint32_t* buffer);
    virtual void read64(uint64_t address, uint64_t* buffer);

    virtual void write8(uint64_t address, const uint8_t* buffer);
    virtual void write16(uint64_t address, const uint16_t* buffer);
    virtual void write32(uint64_t address, const uint32_t* buffer);
    virtual void write64(uint64_t address, const uint64_t* buffer);

    virtual uint64_t getStart();
    virtual uint64_t getEnd();
    virtual size_t getSize();

    virtual bool isInside(uint64_t address, size_t size) {
      return address >= m_start && (address + size) <= m_end;
    }

    virtual void dump(FILE* fp);
    virtual void printData(void (*write)(void* data, const char* format, ...), void* data);

    virtual bool canSplit() { return false; }

   private:
    uint64_t m_start;
    uint64_t m_end;
    size_t m_size;
};

#endif /* _MEMORY_REGION_HPP */