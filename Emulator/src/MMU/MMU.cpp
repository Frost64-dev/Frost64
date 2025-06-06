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

#include "MemoryRegion.hpp"
#include "MMU.hpp"
#include "StandardMemoryRegion.hpp"

#include <common/util.h>

#include <Exceptions.hpp>

MMU::MMU() {
}

MMU::~MMU() {
}

void MMU::ReadBuffer(uint64_t address, uint8_t* data, size_t size) {
    MemoryRegion* startingRegion = nullptr;
    uint64_t regionIndex = 0;
    for (MemoryRegion* region : m_regions) {
        if (region->isInside(address)) {
            startingRegion = region;
            regionIndex++;
            break;
        }
    }
    if (startingRegion == nullptr) {
        // no region found
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);
    }
    size_t remainingSize = size;
    uint64_t currentAddress = address;
    while (remainingSize > 0) {
        size_t currentSize = startingRegion->getEnd() - currentAddress;
        if (currentSize > remainingSize)
            currentSize = remainingSize;
        startingRegion->read(currentAddress, data, currentSize);
        remainingSize -= currentSize;
        if (remainingSize == 0)
            break;
        currentAddress += currentSize;
        data = reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(data) + currentSize * sizeof(uint8_t));
        startingRegion = nullptr;
        for (auto iterator = m_regions.at(regionIndex); iterator != m_regions.end(); ++iterator) {
            MemoryRegion* region = *iterator;
            if (region->isInside(currentAddress)) {
                startingRegion = region;
                regionIndex++;
                break;
            }
        }
        if (startingRegion == nullptr) {
            // no region found
            g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, currentAddress);
        }
    }
}

void MMU::WriteBuffer(uint64_t address, const uint8_t* data, size_t size) {
    MemoryRegion* startingRegion = nullptr;
    uint64_t regionIndex = 0;
    for (MemoryRegion* region : m_regions) {
        if (region->isInside(address)) {
            startingRegion = region;
            regionIndex++;
            break;
        }
    }
    if (startingRegion == nullptr) {
        // no region found
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);
    }
    size_t remainingSize = size;
    uint64_t currentAddress = address;
    while (remainingSize > 0) {
        size_t currentSize = startingRegion->getEnd() - currentAddress;
        if (currentSize > remainingSize)
            currentSize = remainingSize;
        startingRegion->write(currentAddress, data, currentSize);
        remainingSize -= currentSize;
        if (remainingSize == 0)
            break;
        currentAddress += currentSize;
        data = reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(data) + currentSize * sizeof(uint8_t));
        startingRegion = nullptr;
        for (auto iterator = m_regions.at(regionIndex); iterator != m_regions.end(); ++iterator) {
            MemoryRegion* region = *iterator;
            if (region->isInside(currentAddress)) {
                startingRegion = region;
                regionIndex++;
                break;
            }
        }
        if (startingRegion == nullptr) {
            // no region found
            g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, currentAddress);
        }
    }
}

uint8_t MMU::read8(uint64_t address) {
    uint8_t data = 0;
    bool found = false;
    for (MemoryRegion* region : m_regions) {
        if (region->isInside(address)) {
            region->read8(address, &data);
            found = true;
            break;
        }
    }
    if (!found)
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);
    return data;
}

uint16_t MMU::read16(uint64_t address) {
    uint16_t data = 0;
    bool found = false;
    for (MemoryRegion* region : m_regions) {
        if (region->isInside(address, 2)) {
            region->read16(address, &data);
            found = true;
            break;
        }
    }
    if (!found)
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);
    return data;
}

uint32_t MMU::read32(uint64_t address) {
    uint32_t data = 0;
    bool found = false;
    for (MemoryRegion* region : m_regions) {
        if (region->isInside(address, 4)) {
            region->read32(address, &data);
            found = true;
            break;
        }
    }
    if (!found)
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);
    return data;
}

uint64_t MMU::read64(uint64_t address) {
    uint64_t data = 0;
    bool found = false;
    for (MemoryRegion* region : m_regions) {
        if (region->isInside(address, 8)) {
            region->read64(address, &data);
            found = true;
            break;
        }
    }
    if (!found)
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);
    return data;
}

void MMU::write8(uint64_t address, uint8_t data) {
    bool found = false;
    for (MemoryRegion* region : m_regions) {
        if (region->isInside(address)) {
            region->write8(address, &data);
            found = true;
            break;
        }
    }
    if (!found)
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);
}

void MMU::write16(uint64_t address, uint16_t data) {
    bool found = false;
    for (MemoryRegion* region : m_regions) {
        if (region->isInside(address, 2)) {
            region->write16(address, &data);
            found = true;
            break;
        }
    }
    if (!found)
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);
}

void MMU::write32(uint64_t address, uint32_t data) {
    bool found = false;
    for (MemoryRegion* region : m_regions) {
        if (region->isInside(address, 4)) {
            region->write32(address, &data);
            found = true;
            break;
        }
    }
    if (!found)
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);    
}

void MMU::write64(uint64_t address, uint64_t data) {
    bool found = false;
    for (MemoryRegion* region : m_regions) {
        if (region->isInside(address, 8)) {
            region->write64(address, &data);
            found = true;
            break;
        }
    }
    if (!found)
        g_ExceptionHandler->RaiseException(Exception::PHYS_MEM_VIOLATION, address);
}

bool MMU::ValidateRead(uint64_t address, size_t size) {
    MemoryRegion* startingRegion = nullptr;
    uint64_t regionIndex = 0;
    for (MemoryRegion* region : m_regions) {
        if (region->isInside(address)) {
            startingRegion = region;
            regionIndex++;
            break;
        }
    }
    if (startingRegion == nullptr) {
        // no region found
        return false;
    }
    size_t remainingSize = size;
    uint64_t currentAddress = address;
    while (remainingSize > 0) {
        size_t currentSize = startingRegion->getEnd() - currentAddress;
        if (currentSize > remainingSize)
            currentSize = remainingSize;
        remainingSize -= currentSize;
        if (remainingSize == 0)
            break;
        currentAddress += currentSize;
        startingRegion = nullptr;
        for (auto iterator = m_regions.at(regionIndex); iterator != m_regions.end(); ++iterator) {
            MemoryRegion* region = *iterator;
            if (region->isInside(currentAddress)) {
                startingRegion = region;
                regionIndex++;
                break;
            }
        }
        if (startingRegion == nullptr) {
            // no region found
            return false;
        }
    }
    return true;
}

bool MMU::ValidateWrite(uint64_t address, size_t size) {
    return ValidateRead(address, size);
}


bool MMU::ValidateExecute(uint64_t address, size_t size) {
    return ValidateRead(address, size);
}

void MMU::AddMemoryRegion(MemoryRegion* region) {
    // find the correct place to insert the region
    if (m_regions.getCount() == 0) {
        m_regions.insert(region);
        return;
    }
    for (auto iterator = m_regions.begin(); iterator != m_regions.end(); ++iterator) {
        MemoryRegion* currentRegion = *iterator;
        if (region->getStart() < currentRegion->getStart()) {
            m_regions.insertAfter(iterator, region);
            return;
        }
    }
}

void MMU::RemoveMemoryRegion(MemoryRegion* region) {
    m_regions.remove(region);
}

void MMU::DumpMemory(FILE* fp) const {
    for (MemoryRegion* region = m_regions.get(0); region != nullptr; region = m_regions.getNext(region))
        region->dump(fp);
}

void MMU::PrintRegions(void (*write)(void* data, const char* format, ...), void* data) const {
    write(data, "Physical Memory Regions:\n");
    for (MemoryRegion* region = m_regions.get(0); region != nullptr; region = m_regions.getNext(region))
        region->printData(write, data);
}

bool MMU::RemoveRegionSegment(uint64_t start, uint64_t end, void** data_out) {
    for (auto iterator = m_regions.begin(); iterator != m_regions.end(); ++iterator) {
        if (MemoryRegion* region = *iterator; start >= region->getStart() && end > region->getStart() && region->getEnd() > start) {
            if (!region->canSplit())
                return false;
            uint64_t regionStart = region->getStart();
            uint64_t regionEnd = region->getEnd();

            if (regionEnd < end) {
                if (MemoryRegion* region2 = m_regions.getNext(iterator); region2 != nullptr) {
                    if (region2->getStart() < end)
                        return false;
                }
            }

            RemoveMemoryRegion(region);
            delete region;

            // check if the region starts at start
            if (regionStart < start) {
                StandardMemoryRegion* newRegion = new StandardMemoryRegion(regionStart, start);
                AddMemoryRegion(newRegion);
            }

            // check if the region ends at end
            if (regionEnd > end) {
                StandardMemoryRegion* newRegion = new StandardMemoryRegion(end, regionEnd);
                AddMemoryRegion(newRegion);
            }

            RegionSegmentInfo* info = new RegionSegmentInfo();
            info->start = start;
            info->end = MIN(end, regionEnd);
            *data_out = info;

            return true;
        }
    }

    // didn't find a region
    *data_out = nullptr;
    return true;
}

bool MMU::ReaddRegionSegment(void* data_in) {
    if (data_in == nullptr)
        return true;
    uint64_t start = static_cast<RegionSegmentInfo*>(data_in)->start;
    uint64_t end = static_cast<RegionSegmentInfo*>(data_in)->end;
    MemoryRegion* region = nullptr;
    MemoryRegion* previousRegion = nullptr;
    for (auto i_region : m_regions) {
        previousRegion = region;
        region = i_region;
        if (previousRegion == nullptr)
            continue;
        if (start >= previousRegion->getEnd() && end <= region->getStart()) {
            if (start == previousRegion->getEnd() && end == region->getStart()) {
                uint64_t newStart = previousRegion->getStart();
                uint64_t newEnd = region->getEnd();
                RemoveMemoryRegion(previousRegion);
                RemoveMemoryRegion(region);
                StandardMemoryRegion* newRegion = new StandardMemoryRegion(newStart, newEnd);
                AddMemoryRegion(newRegion);
            } else if (start == previousRegion->getEnd()) {
                uint64_t newStart = previousRegion->getStart();
                RemoveMemoryRegion(previousRegion);
                StandardMemoryRegion* newRegion = new StandardMemoryRegion(newStart, end);
                AddMemoryRegion(newRegion);
            } else if (end == region->getStart()) {
                uint64_t newEnd = region->getEnd();
                RemoveMemoryRegion(region);
                StandardMemoryRegion* newRegion = new StandardMemoryRegion(start, newEnd);
                AddMemoryRegion(newRegion);
            } else {
                StandardMemoryRegion* newRegion = new StandardMemoryRegion(start, end);
                AddMemoryRegion(newRegion);
            }
            return true;
        }
    }
    return false;
}
