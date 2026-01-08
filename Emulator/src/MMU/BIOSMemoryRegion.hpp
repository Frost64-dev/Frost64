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

#ifndef _BIOS_MEMORY_REGION_HPP
#define _BIOS_MEMORY_REGION_HPP

#include "StandardMemoryRegion.hpp"

class BIOSMemoryRegion : public StandardMemoryRegion {
public:
    BIOSMemoryRegion(uint64_t start, uint64_t end, uint64_t real_size);
    ~BIOSMemoryRegion();

    virtual void dump(FILE* fp) override;
    virtual void printData(void (*write)(void* data, const char* format, ...), void* data) override;

    virtual bool isBIOS() override { return true; }
    virtual bool isExecutable() override { return true; }

private:
    uint64_t m_real_size;
};

#endif /* _BIOS_MEMORY_REGION_HPP */