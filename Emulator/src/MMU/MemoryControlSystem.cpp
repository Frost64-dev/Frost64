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

#include "MemoryControlSystem.hpp"

#include <cstddef>
#include <cstdint>

#include "Emulator.hpp"
#include "Exceptions.hpp"
#include "MMU.hpp"
#include "StandardMemoryRegion.hpp"

MemoryControlSystem::MemoryControlSystem(uint64_t RAMSize, MMU* mmu) : m_Status(0), m_Data{0, 0, 0, 0}, m_RAMSize(RAMSize), m_currentUsedRAM(0), m_MMU(mmu) {

}

uint64_t MemoryControlSystem::ReadRegister(uint64_t offset) {
    switch (static_cast<Registers>(offset)) {
    case Registers::COMMAND:
        return 0;
    case Registers::STATUS:
        return m_Status;
    case Registers::DATA0:
    case Registers::DATA1:
    case Registers::DATA2:
    case Registers::DATA3:
        return m_Data[offset - static_cast<uint64_t>(Registers::DATA0)];
    default:
        return 0;
    }
}

void MemoryControlSystem::WriteRegister(uint64_t offset, uint64_t data) {
    switch (static_cast<Registers>(offset)) {
    case Registers::COMMAND:
        RunCommand(data);
        break;
    case Registers::STATUS:
        m_Status = data;
        break;
    case Registers::DATA0:
    case Registers::DATA1:
    case Registers::DATA2:
    case Registers::DATA3:
        m_Data[offset - static_cast<uint64_t>(Registers::DATA0)] = data;
        break;
    }
}

void MemoryControlSystem::RunCommand(uint64_t command) {
    Commands comm = static_cast<Commands>(command);
    m_Status = 0;
    switch (comm) {
    case Commands::GET_INFO:
        m_Data[0] = m_RAMSize;
        break;
    case Commands::SET_REGION: {
        uint64_t base = m_Data[0];
        uint64_t size = m_Data[1];
        if (base % 4096 != 0 || size % 4096 != 0 || size == 0 || m_currentUsedRAM + size > m_RAMSize || m_MMU->HasRegion(base, size)) {
            m_Status = 1; // error: not aligned or zero size, not enough RAM, or region already exists
            break;
        }
        StandardMemoryRegion* region = new StandardMemoryRegion(base, base + size);
        m_MMU->AddMemoryRegion(region);
        m_currentUsedRAM += size;
        break;
    }
    default:
        m_Status = 1; // error: unknown command
        break;
    }
}
