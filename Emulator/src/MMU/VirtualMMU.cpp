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

#include "VirtualMMU.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <common/util.h>

#include <Emulator.hpp>
#include <Exceptions.hpp>

VirtualMMU::VirtualMMU(MMU* physicalMMU, uint64_t pageTableRoot, PageSize pageSize, PageTableLevelCount pageTableLevelCount)
    : m_physicalMMU(physicalMMU), m_pageTableRoot(pageTableRoot), m_pageSize(pageSize), m_pageTableLevelCount(pageTableLevelCount) {
    assert(m_physicalMMU != nullptr);
}

VirtualMMU::~VirtualMMU() {
}

void VirtualMMU::ReadBuffer(uint64_t address, uint8_t* data, size_t size) {
    m_physicalMMU->ReadBuffer(TranslateAddress(address, PageTranslateMode::Read), data, size);
}

void VirtualMMU::WriteBuffer(uint64_t address, const uint8_t* data, size_t size) {
    m_physicalMMU->WriteBuffer(TranslateAddress(address, PageTranslateMode::Write), data, size);
}

uint8_t VirtualMMU::read8(uint64_t address) {
    return m_physicalMMU->read8(TranslateAddress(address, PageTranslateMode::Read));
}

uint16_t VirtualMMU::read16(uint64_t address) {
    return m_physicalMMU->read16(TranslateAddress(address, PageTranslateMode::Read));
}

uint32_t VirtualMMU::read32(uint64_t address) {
    return m_physicalMMU->read32(TranslateAddress(address, PageTranslateMode::Read));
}

uint64_t VirtualMMU::read64(uint64_t address) {
    return m_physicalMMU->read64(TranslateAddress(address, PageTranslateMode::Read));
}

void VirtualMMU::write8(uint64_t address, uint8_t data) {
    m_physicalMMU->write8(TranslateAddress(address, PageTranslateMode::Write), data);
}

void VirtualMMU::write16(uint64_t address, uint16_t data) {
    m_physicalMMU->write16(TranslateAddress(address, PageTranslateMode::Write), data);
}

void VirtualMMU::write32(uint64_t address, uint32_t data) {
    m_physicalMMU->write32(TranslateAddress(address, PageTranslateMode::Write), data);
}

void VirtualMMU::write64(uint64_t address, uint64_t data) {
    m_physicalMMU->write64(TranslateAddress(address, PageTranslateMode::Write), data);
}

bool VirtualMMU::ValidateRead(uint64_t address, size_t size) {
    uint64_t pageSize = 0;
    switch (m_pageSize) {
    case PS_4KiB:
        pageSize = 4'096;
        break;
    case PS_16KiB:
        pageSize = 16'384;
        break;
    case PS_64KiB:
        pageSize = 65'536;
        break;
    default:
        Emulator::Crash("Invalid page size");
    }
    uint64_t start = ALIGN_DOWN_BASE2(address, pageSize);
    uint64_t end = ALIGN_UP_BASE2(address + size, pageSize);
    for (uint64_t i = start; i < end; i += pageSize) {
        bool success = false;
        TranslateAddress(i, PageTranslateMode::Read, true, &success);
        if (!success)
            return false;
    }
    return true;
}

bool VirtualMMU::ValidateWrite(uint64_t address, size_t size) {
    uint64_t pageSize = 0;
    switch (m_pageSize) {
    case PS_4KiB:
        pageSize = 4'096;
        break;
    case PS_16KiB:
        pageSize = 16'384;
        break;
    case PS_64KiB:
        pageSize = 65'536;
        break;
    default:
        Emulator::Crash("Invalid page size");
    }
    uint64_t start = ALIGN_DOWN_BASE2(address, pageSize);
    uint64_t end = ALIGN_UP_BASE2(address + size, pageSize);
    for (uint64_t i = start; i < end; i += pageSize) {
        bool success = false;
        TranslateAddress(i, PageTranslateMode::Write, true, &success);
        if (!success)
            return false;
    }
    return true;
}

bool VirtualMMU::ValidateExecute(uint64_t address, size_t size) {
    uint64_t pageSize = 0;
    switch (m_pageSize) {
    case PS_4KiB:
        pageSize = 4'096;
        break;
    case PS_16KiB:
        pageSize = 16'384;
        break;
    case PS_64KiB:
        pageSize = 65'536;
        break;
    default:
        Emulator::Crash("Invalid page size");
    }
    uint64_t start = ALIGN_DOWN_BASE2(address, pageSize);
    uint64_t end = ALIGN_UP_BASE2(address + size, pageSize);
    for (uint64_t i = start; i < end; i += pageSize) {
        bool success = false;
        TranslateAddress(i, PageTranslateMode::Execute, true, &success);
        if (!success)
            return false;
    }
    return true;
}

void VirtualMMU::AddMemoryRegion(MemoryRegion* region) {
    (void)region;
}

void VirtualMMU::RemoveMemoryRegion(MemoryRegion* region) {
    (void)region;
}

void VirtualMMU::DumpMemory(FILE*) const {
}

bool VirtualMMU::RemoveRegionSegment(uint64_t, uint64_t, void**) {
    return false;
}

bool VirtualMMU::ReaddRegionSegment(void*) {
    return false;
}

void VirtualMMU::SetPageTableRoot(uint64_t pageTableRoot) {
    m_pageTableRoot = pageTableRoot;
}

uint64_t VirtualMMU::TranslateAddress(uint64_t address, PageTranslateMode mode, bool safe, bool* success) const {
    uint8_t levelCount = 0;
    switch (m_pageTableLevelCount) {
    case PTLC_3:
        levelCount = 3;
        break;
    case PTLC_4:
        levelCount = 4;
        break;
    case PTLC_5:
        levelCount = 5;
        break;
    default:
        assert(false); // deal with error handling later...
    }
    uint64_t page = 0;
    uint16_t offset = 0;
    uint8_t pageShift = 0;
    switch (m_pageSize) {
    case PS_4KiB:
        page = address >> 12;
        offset = address & 0xFFF;
        pageShift = 12;
        break;
    case PS_16KiB:
        page = address >> 14;
        offset = address & 0x3FFF;
        pageShift = 14;
        break;
    case PS_64KiB:
        page = address >> 16;
        offset = address & 0xFFFF;
        pageShift = 16;
        break;
    default:
        assert(false); // deal with error handling later...
    }
    bool inUserMode = Emulator::isInProtectedMode() && Emulator::isInUserMode();
    PageTableEntry table;
    uint64_t physicalAddress = 0;
    for (uint8_t i = 0; i < levelCount; i++) {
        uint64_t index = (page >> (10 * (levelCount - i - 1))) & 0x3FF;
        if (i == 0) {
            // need to fetch the table from guest memory
            if (!m_physicalMMU->ValidateRead(m_pageTableRoot + index * 8, 8))
                g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, m_pageTableRoot + index * 8);
            {
                uint64_t raw = m_physicalMMU->read64(m_pageTableRoot + index * 8);
                PageTableEntry* temp = reinterpret_cast<PageTableEntry*>(&raw);
                table = *temp;
            }
        } else if (table.Lowest) {
            if (safe && success != nullptr)
                *success = true;
            return (((table.PhysicalAddress >> (pageShift - 12)) | (page & ((1 << (10 * (i - 1))) - 1))) << pageShift) | offset;
        } else if (!GetNextTableLevel(table, index, &table)) {
            if (safe) {
                if (success != nullptr)
                    *success = false;
                return 0;
            }
            PagingViolationErrorCode code = {false, false, false, false, false, false, 0};
            code.read = mode == PageTranslateMode::Read;
            code.write = mode == PageTranslateMode::Write;
            code.execute = mode == PageTranslateMode::Execute;
            code.user = inUserMode;
            g_ExceptionHandler->RaiseException(Exception::PAGING_VIOLATION, address, code);
        }
        if (!table.Present) {
            if (safe) {
                if (success != nullptr)
                    *success = false;
                return 0;
            }
            PagingViolationErrorCode code = {false, false, false, false, false, false, 0};
            code.read = mode == PageTranslateMode::Read;
            code.write = mode == PageTranslateMode::Write;
            code.execute = mode == PageTranslateMode::Execute;
            code.user = inUserMode;
            g_ExceptionHandler->RaiseException(Exception::PAGING_VIOLATION, address, code);
        }
        if ((!table.Readable && mode == PageTranslateMode::Read) || (!table.Writable && mode == PageTranslateMode::Write) || (!table.Executable && mode == PageTranslateMode::Execute) || (inUserMode && !table.User)) {
            if (safe) {
                if (success != nullptr)
                    *success = false;
                return 0;
            }
            PagingViolationErrorCode code = {false, false, false, false, false, false, 0};
            code.read = mode == PageTranslateMode::Read;
            code.write = mode == PageTranslateMode::Write;
            code.execute = mode == PageTranslateMode::Execute;
            code.user = inUserMode;
            g_ExceptionHandler->RaiseException(Exception::PAGING_VIOLATION, address, code);
        }
        physicalAddress = table.PhysicalAddress >> (pageShift - 12);
    }
    if (safe && success != nullptr)
        *success = true;
    return (physicalAddress << pageShift) | offset;
}

bool VirtualMMU::GetNextTableLevel(PageTableEntry table, uint64_t tableIndex, PageTableEntry* out) const { // tableIndex = index within table
    assert(tableIndex < 1'024);

    if (!table.Present)
        return false;

    if (table.Lowest)
        return false; // should already be handled by the caller

    // data structure for PageTableEntry only supports 4KiB pages, so just shift by 12
    if (!m_physicalMMU->ValidateRead(((uint64_t)table.PhysicalAddress << 12) + tableIndex * 8, 8))
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, ((uint64_t)table.PhysicalAddress << 12) + tableIndex * 8);

    uint64_t raw = m_physicalMMU->read64(((uint64_t)table.PhysicalAddress << 12) + tableIndex * 8);
    PageTableEntry* temp = reinterpret_cast<PageTableEntry*>(&raw);
    if (out != nullptr)
        *out = *temp;
    return true;
}
