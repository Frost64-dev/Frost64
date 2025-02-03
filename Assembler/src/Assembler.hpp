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

#ifndef _ASSEMBLER_HPP
#define _ASSEMBLER_HPP

#include <cstdint>

#include <common/Data-structures/LinkedList.hpp>
#include <common/Data-structures/Buffer.hpp>

#include <libarch/Instruction.hpp>

class Section {
public:
    

    Section(char* name, uint64_t name_size, uint64_t offset);
    ~Section();

    [[nodiscard]] char const* GetName() const;
    [[nodiscard]] uint64_t GetNameSize() const;
    [[nodiscard]] uint64_t GetOffset() const;

private:
    char* m_name;
    uint64_t m_name_size;
    uint64_t m_offset;
};

class Assembler {
public:
    Assembler();
    ~Assembler();

    void assemble(const LinkedList::RearInsertLinkedList<InsEncoding::Label>& labels, uint64_t base_address);

    [[nodiscard]] const Buffer& GetBuffer() const;

    void Clear();

private:

    [[noreturn]] static void error(const char* message, const std::string& file_name, size_t line);

    uint64_t m_current_offset;
    Buffer m_buffer;
    LinkedList::RearInsertLinkedList<Section> m_sections;
};

#endif /* _ASSEMBLER_HPP */