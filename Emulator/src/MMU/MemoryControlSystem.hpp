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

#ifndef _MEM_CONTROL_SYSTEM_HPP
#define _MEM_CONTROL_SYSTEM_HPP

#include <cstddef>
#include <cstdint>

#include "MMU.hpp"

class MemoryControlSystem {
public:
    MemoryControlSystem(uint64_t RAMSize, MMU* mmu);

    uint64_t ReadRegister(uint64_t offset);
    void WriteRegister(uint64_t offset, uint64_t data);

private:
    enum class Registers {
        COMMAND = 0,
        STATUS = 1,
        DATA0 = 2,
        DATA1 = 3,
        DATA2 = 4,
        DATA3 = 5
    };

    enum class Commands {
        GET_INFO = 0,
        SET_REGION = 1
    };

    void RunCommand(uint64_t command);

private:
    uint64_t m_Status;
    uint64_t m_Data[4];
    uint64_t m_RAMSize;
    uint64_t m_currentUsedRAM;
    MMU* m_MMU; // always the physical MMU
};

#endif /* _MEM_CONTROL_SYSTEM_HPP */