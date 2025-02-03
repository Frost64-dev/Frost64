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

#ifndef _INSTRUCTION_BUFFER_HPP
#define _INSTRUCTION_BUFFER_HPP

#include <cstdint>
#include <cstddef>

#include <common/Data-structures/Buffer.hpp>

#include <MMU/MMU.hpp>

class InstructionBuffer : public Buffer {
public:
    InstructionBuffer(MMU* mmu, uint64_t base_address);
    ~InstructionBuffer();

    // Write size bytes from data to the buffer at offset
    void Write(uint64_t offset, const uint8_t* data, size_t size) override;

    // Read size bytes from the buffer at offset to data
    void Read(uint64_t offset, uint8_t* data, size_t size) const override;

protected:
    Block* AddBlock(size_t size) override;
    void DeleteBlock(uint64_t index) override;

private:
    MMU* m_mmu;
    uint64_t m_base_address;
};

#endif /* _INSTRUCTION_BUFFER_HPP */