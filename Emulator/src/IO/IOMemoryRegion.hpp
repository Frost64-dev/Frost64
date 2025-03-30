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

#ifndef _IO_MEMORY_REGION_HPP
#define _IO_MEMORY_REGION_HPP

#include <cstdint>

#include <MMU/MemoryRegion.hpp>

#include "IOBus.hpp"

class IOMemoryRegion : public MemoryRegion {
   public:
    IOMemoryRegion(uint64_t start, uint64_t end, IOBus* bus);
    IOMemoryRegion(uint64_t start, uint64_t end, IODevice* device);
    ~IOMemoryRegion() override;

    virtual void read(uint64_t address, uint8_t* buffer, size_t size) override;
    virtual void write(uint64_t address, const uint8_t* buffer, size_t size) override;

    virtual void read8(uint64_t address, uint8_t* buffer) override;
    virtual void read16(uint64_t address, uint16_t* buffer) override;
    virtual void read32(uint64_t address, uint32_t* buffer) override;
    virtual void read64(uint64_t address, uint64_t* buffer) override;
    virtual void write8(uint64_t address, const uint8_t* buffer) override;
    virtual void write16(uint64_t address, const uint16_t* buffer) override;
    virtual void write32(uint64_t address, const uint32_t* buffer) override;
    virtual void write64(uint64_t address, const uint64_t* buffer) override;

    virtual void dump(FILE* fp) override;
    virtual void printData(void (*write)(void* data, const char* format, ...), void* data) override;

   private:
    union {
        IOBus* bus;
        IODevice* device;
    } m_data;
    bool m_isBus;
};

#endif /* _IO_MEMORY_REGION_HPP */