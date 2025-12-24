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

#ifndef _LIBEXEC_EXEC_HPP
#define _LIBEXEC_EXEC_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <Common/DataStructures/LinkedList.hpp>

#include "elf.h"

class ELFProgramSection {
public:
    ELFProgramSection();
    ~ELFProgramSection();

    void SetData(const uint8_t* data, size_t size); // copies the data into an internal buffer, does not take ownership
    void SetVirtAddr(uint64_t addr);
    void SetPhysAddr(uint64_t addr);
    void SetMemSize(uint64_t size);
    void SetAlignment(uint64_t align); // in bytes, must be a power of 2
    void SetFlags(int flags);
    void SetType(int type);

    Elf64_Phdr* GetPhdr() { return m_phdr; }
    uint8_t* GetData() { return m_data; }

private:
    Elf64_Phdr* m_phdr;
    uint8_t* m_data;
};

class ELFSection {
public:
    ELFSection();
    ~ELFSection();

    void SetName(const char* name);
    void SetType(int type);
    void SetFlags(int flags);
    void SetRegion(uint64_t addr, size_t size, uint64_t align);

    void SetProgSection(ELFProgramSection* section);

    ELFProgramSection* GetProgSection() { return m_phdr; }
    Elf64_Shdr* GetShdr() { return m_shdr; }
    const std::string_view& GetName() { return m_name; }

private:
    ELFProgramSection* m_phdr;
    Elf64_Shdr* m_shdr;
    std::string_view m_name;
};

class ELFExecutable {
public:
    ELFExecutable();
    ~ELFExecutable();

    bool Create();

    void SetEntryPoint(uint64_t entry);

    ELFProgramSection* CreateNewProgramSection();
    ELFSection* CreateNewSection();

    bool WriteToFile(const char* path);

private:
    LinkedList::SimpleLinkedList<ELFProgramSection> m_programSections;
    LinkedList::SimpleLinkedList<ELFSection> m_sections;
    Elf64_Ehdr* m_ehdr;
};


#endif /* _LIBEXEC_EXEC_HPP */