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

#ifndef _VIRTUAL_MMU_HPP
#define _VIRTUAL_MMU_HPP

#include "MMU.hpp"

struct PageTableEntry {
    bool Present             : 1;
    bool Readable            : 1;
    bool Writable            : 1;
    bool Executable          : 1;
    bool User                : 1;
    bool Lowest              : 1;
    uint8_t Reserved         : 6;
    uint64_t PhysicalAddress : 52;
} __attribute__((packed));

enum PageSize {
    PS_4KiB = 0,
    PS_16KiB = 1,
    PS_64KiB = 2
};

enum PageTableLevelCount {
    PTLC_3 = 0,
    PTLC_4 = 1,
    PTLC_5 = 2
};

enum class PageTranslateMode {
    Read,
    Write,
    Execute
};

class VirtualMMU : public MMU {
   public:
    VirtualMMU(MMU* physicalMMU, uint64_t pageTableRoot, PageSize pageSize, PageTableLevelCount pageTableLevelCount);
    virtual ~VirtualMMU() override;

    virtual void ReadBuffer(uint64_t address, uint8_t* data, size_t size) override;
    virtual void WriteBuffer(uint64_t address, const uint8_t* data, size_t size) override;

    virtual uint8_t read8(uint64_t address) override;
    virtual uint16_t read16(uint64_t address) override;
    virtual uint32_t read32(uint64_t address) override;
    virtual uint64_t read64(uint64_t address) override;

    virtual void write8(uint64_t address, uint8_t data) override;
    virtual void write16(uint64_t address, uint16_t data) override;
    virtual void write32(uint64_t address, uint32_t data) override;
    virtual void write64(uint64_t address, uint64_t data) override;

    virtual bool ValidateRead(uint64_t address, size_t size) override;
    virtual bool ValidateWrite(uint64_t address, size_t size) override;
    virtual bool ValidateExecute(uint64_t address, size_t size) override;

    virtual void AddMemoryRegion(MemoryRegion* region) override; // disabled
    virtual void RemoveMemoryRegion(MemoryRegion* region) override; // disabled

    virtual void DumpMemory(FILE* fp) const override;

    virtual bool RemoveRegionSegment(uint64_t, uint64_t, void**) override;
    virtual bool ReaddRegionSegment(void*) override;

    void SetPageTableRoot(uint64_t pageTableRoot);

   private:
    /*
     * safe flag prevents Paging Violation exceptions, but not Physical Memory Violation exceptions.
     * success is only written to when not nullptr and safe is true.
     */
    uint64_t TranslateAddress(uint64_t address, PageTranslateMode mode, bool safe = false, bool* success = nullptr) const;
    bool GetNextTableLevel(PageTableEntry table, uint64_t tableIndex, PageTableEntry* out) const;

   private:
    MMU* m_physicalMMU;
    uint64_t m_pageTableRoot;
    PageSize m_pageSize;
    PageTableLevelCount m_pageTableLevelCount;
};

#endif /* _VIRTUAL_MMU_HPP */