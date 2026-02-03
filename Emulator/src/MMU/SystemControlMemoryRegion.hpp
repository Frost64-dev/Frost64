/*
Copyright (©) 2025-2026  Frosty515

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

#ifndef _SYS_CONTROL_MEMORY_REGION_HPP
#define _SYS_CONTROL_MEMORY_REGION_HPP

#include <cstddef>
#include <cstdint>

#include <IO/IOMemoryRegion.hpp>

#include "MemoryRegion.hpp"
#include "MemoryControlSystem.hpp"
#include "MMU.hpp"

class SystemControlMemoryRegion : public MemoryRegion {
public:
    SystemControlMemoryRegion(uint64_t start, uint64_t end, IOBus* bus, uint64_t RAMSize, MMU* mmu);
    virtual ~SystemControlMemoryRegion() override;

    virtual void read(uint64_t address, uint8_t* buffer, size_t size) override;
    virtual void write(uint64_t address, const uint8_t* buffer, size_t size) override;

    virtual uint8_t read8(uint64_t address) override;
    virtual uint16_t read16(uint64_t address) override;
    virtual uint32_t read32(uint64_t address) override;
    virtual uint64_t read64(uint64_t address) override;

    virtual void write8(uint64_t address, uint8_t data) override;
    virtual void write16(uint64_t address, uint16_t data) override;
    virtual void write32(uint64_t address, uint32_t data) override;
    virtual void write64(uint64_t address, uint64_t data) override;

    virtual bool canSplit() override { return false; }

private:
    uint64_t ReadRegister(uint64_t offset);
    void WriteRegister(uint64_t offset, uint64_t data);

private:
    IOMemoryRegion m_IORegion;
    MemoryControlSystem m_MemControl;
};

#endif /* _SYS_CONTROL_MEMORY_REGION_HPP */