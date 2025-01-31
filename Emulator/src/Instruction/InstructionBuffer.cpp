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

#include "InstructionBuffer.hpp"

#include <Emulator.hpp>
#include <Exceptions.hpp>

InstructionBuffer::InstructionBuffer(MMU* mmu, uint64_t base_address) : m_mmu(mmu), m_base_address(base_address) {

}

InstructionBuffer::~InstructionBuffer() {

}

void InstructionBuffer::Write(uint64_t offset, const uint8_t* data, size_t size) {
    m_mmu->WriteBuffer(m_base_address + offset, data, size);
}

void InstructionBuffer::Read(uint64_t offset, uint8_t* data, size_t size) const {
    if (!m_mmu->ValidateExecute(m_base_address + offset, size))
        g_ExceptionHandler->RaiseException(Emulator::isPagingEnabled() ? Exception::PAGING_VIOLATION : Exception::PHYS_MEM_VIOLATION, m_base_address + offset);
    m_mmu->ReadBuffer(m_base_address + offset, data, size);
}

InsEncoding::Buffer::Block* InstructionBuffer::AddBlock(size_t size) {
    (void)size;
    return nullptr;
}

void InstructionBuffer::DeleteBlock(uint64_t index) {
    (void)index;
}
