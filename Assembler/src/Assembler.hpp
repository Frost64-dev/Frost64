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

#include <Common/DataStructures/LinkedList.hpp>
#include <Common/DataStructures/Buffer.hpp>

#include <LibArch/Instruction.hpp>

class DataBlock {
public:
    DataBlock(char* name, uint64_t nameSize, uint64_t offset);
    ~DataBlock();

    [[nodiscard]] char const* GetName() const;
    [[nodiscard]] uint64_t GetNameSize() const;
    [[nodiscard]] uint64_t GetOffset() const;

private:
    char* m_name;
    uint64_t m_nameSize;
    uint64_t m_offset;
};

enum class AssemblerFileFormat {
    BINARY,
    ELF
};

struct DataSection {
    uint64_t startingAddress;
    uint64_t size;
    char* name;
    uint64_t nameSize;
    Buffer buffer;
};

class Assembler {
public:
    Assembler();
    ~Assembler();

    void assemble(const LinkedList::RearInsertLinkedList<InsEncoding::Section>& sections, AssemblerFileFormat format, FILE* outStream);

    void Clear();

private:

    [[noreturn]] static void error(const char* message, const std::string& fileName, size_t line);

    LinkedList::RearInsertLinkedList<DataSection> m_sections;
    LinkedList::RearInsertLinkedList<DataBlock> m_dataBlocks;
};

#endif /* _ASSEMBLER_HPP */