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

#ifndef _MMU_HPP
#define _MMU_HPP

#include <cstddef>
#include <cstdint>

#include <Common/DataStructures/LinkedList.hpp>

#include "MemoryRegion.hpp"

class MMU {
   public:
    MMU();
    virtual ~MMU();

    virtual void ReadBuffer(uint64_t address, uint8_t* data, size_t size);
    virtual void WriteBuffer(uint64_t address, const uint8_t* data, size_t size);

    virtual uint8_t read8(uint64_t address);
    virtual uint16_t read16(uint64_t address);
    virtual uint32_t read32(uint64_t address);
    virtual uint64_t read64(uint64_t address);

    virtual void write8(uint64_t address, uint8_t data);
    virtual void write16(uint64_t address, uint16_t data);
    virtual void write32(uint64_t address, uint32_t data);
    virtual void write64(uint64_t address, uint64_t data);

    virtual bool ValidateRead(uint64_t address, size_t size);
    virtual bool ValidateWrite(uint64_t address, size_t size);
    virtual bool ValidateExecute(uint64_t address, size_t size);

    virtual void AddMemoryRegion(MemoryRegion* region);
    virtual void RemoveMemoryRegion(MemoryRegion* region);

    virtual void DumpMemory(FILE* fp) const;
    virtual void PrintRegions(void (*write)(void* data, const char* format, ...), void* data) const;

    virtual bool RemoveRegionSegment(uint64_t start, uint64_t end, void** data_out);
    virtual bool ReaddRegionSegment(void* data_in);

   private:
    struct RegionSegmentInfo {
        uint64_t start;
        uint64_t end;
    };

    LinkedList::SimpleLinkedList<MemoryRegion> m_regions;
};

#endif /* _MMU_HPP */