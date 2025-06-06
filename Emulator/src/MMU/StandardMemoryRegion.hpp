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

#ifndef _STANDARD_MEMORY_REGION_HPP
#define _STANDARD_MEMORY_REGION_HPP

#include <stdint.h>

#include "MemoryRegion.hpp"

class StandardMemoryRegion : public MemoryRegion {
public:
    StandardMemoryRegion(uint64_t start, uint64_t end);
    ~StandardMemoryRegion();

    virtual void read(uint64_t address, uint8_t* buffer, size_t size) override;
    virtual void write(uint64_t address, const uint8_t* buffer, size_t size) override;

    virtual bool canSplit() override { return true; }

private:
    uint8_t* m_data;
};

#endif /* _STANDARD_MEMORY_REGION_HPP */