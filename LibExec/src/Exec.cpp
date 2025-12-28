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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>

#include <Common/Util.hpp>

#include <LibExec/elf.h>
#include <LibExec/Exec.hpp>


ELFProgramSection::ELFProgramSection() : m_phdr(new Elf64_Phdr()), m_data(nullptr) {
    memset(m_phdr, 0, sizeof(Elf64_Phdr));
}

ELFProgramSection::~ELFProgramSection() {

}

void ELFProgramSection::SetData(const uint8_t* data, size_t size) {
    if (size == 0 || data == nullptr)
        return;
    m_data = new uint8_t[size];
    memcpy(m_data, data, size);
    m_phdr->p_filesz = size;
    if (m_phdr->p_memsz == 0)
        m_phdr->p_memsz = size;
}

void ELFProgramSection::SetVirtAddr(uint64_t addr) {
    m_phdr->p_vaddr = addr;
}

void ELFProgramSection::SetPhysAddr(uint64_t addr) {
    m_phdr->p_paddr = addr;
}

void ELFProgramSection::SetMemSize(uint64_t size) {
    m_phdr->p_memsz = size;
}

void ELFProgramSection::SetAlignment(uint64_t align) {
    m_phdr->p_align = align;
}

void ELFProgramSection::SetFlags(int flags) {
    m_phdr->p_flags = flags;
}

void ELFProgramSection::SetType(int type) {
    m_phdr->p_type = type;
}


ELFSection::ELFSection() : m_phdr(nullptr), m_shdr(new Elf64_Shdr()) {
    memset(m_shdr, 0, sizeof(Elf64_Shdr));

}

ELFSection::~ELFSection() {

}

void ELFSection::SetName(const char* name) {
    m_name = name;
}

void ELFSection::SetType(int type) {
    m_shdr->sh_type = type;
}

void ELFSection::SetFlags(int flags) {
    m_shdr->sh_flags = flags;
}

void ELFSection::SetRegion(uint64_t addr, size_t size, uint64_t align) {
    m_shdr->sh_addr = addr;
    m_shdr->sh_size = size;
    m_shdr->sh_addralign = align;
}

void ELFSection::SetProgSection(ELFProgramSection* section) {
    m_phdr = section;
}


ELFExecutable::ELFExecutable() : m_ehdr(nullptr) {

}

ELFExecutable::~ELFExecutable() {

}

bool ELFExecutable::Create() {
    m_ehdr = new Elf64_Ehdr();
    memset(m_ehdr, 0, sizeof(Elf64_Ehdr));
    m_ehdr->e_ident[EI_MAG0] = ELFMAG0;
    m_ehdr->e_ident[EI_MAG1] = ELFMAG1;
    m_ehdr->e_ident[EI_MAG2] = ELFMAG2;
    m_ehdr->e_ident[EI_MAG3] = ELFMAG3;
    m_ehdr->e_ident[EI_CLASS] = ELFCLASS64;
    m_ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
    m_ehdr->e_ident[EI_VERSION] = EV_CURRENT;
    m_ehdr->e_ident[EI_OSABI] = ELFOSABI_SYSV;
    m_ehdr->e_type = ET_EXEC;
    m_ehdr->e_machine = EM_FROST64;
    m_ehdr->e_version = EV_CURRENT;
    m_ehdr->e_entry = 0x400000; // Default entry point
    m_ehdr->e_phoff = 0; // To be filled later
    m_ehdr->e_shoff = 0; // To be filled later
    m_ehdr->e_flags = 0;
    m_ehdr->e_ehsize = sizeof(Elf64_Ehdr);
    m_ehdr->e_phentsize = sizeof(Elf64_Phdr);
    m_ehdr->e_phnum = 0; // To be filled later
    m_ehdr->e_shentsize = sizeof(Elf64_Shdr);
    m_ehdr->e_shnum = 1; // to be updated later
    m_ehdr->e_shstrndx = SHN_UNDEF;
    return true;
}

void ELFExecutable::SetEntryPoint(uint64_t entry) {
    m_ehdr->e_entry = entry;
}

ELFProgramSection* ELFExecutable::CreateNewProgramSection() {
    ELFProgramSection* newSection = new ELFProgramSection();
    m_programSections.insert(newSection);
    return newSection;
}

ELFSection* ELFExecutable::CreateNewSection() {
    ELFSection* newSection = new ELFSection();
    m_sections.insert(newSection);
    return newSection;
}

bool ELFExecutable::WriteToFile(const char* path) {
    FILE* file = fopen(path, "wb");
    if (file == nullptr)
        return false;
    bool result = WriteToStream(file);
    fclose(file);
    return result;
}

bool ELFExecutable::WriteToStream(FILE* stream) {
    /* File layout
     *
     * ELF Header
     * Program Headers
     * Program Section Data
     * Section Headers
     * Section Header String Table
     */

    // Update ELF header with program header info
    m_ehdr->e_phoff = sizeof(Elf64_Ehdr);
    m_ehdr->e_phnum = m_programSections.getCount();

    // Write ELF header
    fwrite(m_ehdr, 1, sizeof(Elf64_Ehdr), stream);

    // Write Program Headers
    uint64_t dataOffset = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) * m_programSections.getCount();
    m_programSections.EnumerateNoExit([&](ELFProgramSection* section) {
        Elf64_Phdr* phdr = section->GetPhdr();
        dataOffset = ALIGN_UP_BASE2(dataOffset, phdr->p_align);
        phdr->p_offset = dataOffset;
        fwrite(phdr, 1, sizeof(Elf64_Phdr), stream);
        dataOffset += phdr->p_filesz;
    });

    // Write Program Section Data
    m_programSections.EnumerateNoExit([&](ELFProgramSection* section) {
        uint8_t* data = section->GetData();
        Elf64_Phdr* phdr = section->GetPhdr();
        uint64_t currentPos = ftell(stream);
        if (currentPos < phdr->p_offset) {
            uint64_t paddingSize = phdr->p_offset - currentPos;
            for (uint64_t i = 0; i < paddingSize; i++)
                fputc(0, stream);
        }
        fwrite(data, 1, phdr->p_filesz, stream);
    });

    // Update ELF header with section header info
    m_ehdr->e_shoff = ftell(stream);
    m_ehdr->e_shnum = m_sections.getCount() + 2; // NULL and string table
    m_ehdr->e_shstrndx = m_sections.getCount() + 1; // Last section is the string table
    fseek(stream, 0, SEEK_SET);
    fwrite(m_ehdr, 1, sizeof(Elf64_Ehdr), stream);
    fseek(stream, m_ehdr->e_shoff, SEEK_SET);

    // Write NULL Section header
    Elf64_Shdr nullShdr;
    memset(&nullShdr, 0, sizeof(Elf64_Shdr));
    // even those these 2 values are 0, set them explicitly for clarity
    nullShdr.sh_type = SHT_NULL;
    nullShdr.sh_link = SHN_UNDEF;
    fwrite(&nullShdr, 1, sizeof(Elf64_Shdr), stream);

    // Fill in the string table offsets and get its size
    uint64_t stringSize = 1; // initial null byte
    m_sections.EnumerateNoExit([&](ELFSection* section) {
        section->GetShdr()->sh_name = stringSize;
        if (section->GetName().size() > 0)
            stringSize += section->GetName().size() + 1;
    });
    stringSize += 10; // for the .shstrtab string and its null byte

    // Write Section headers
    m_sections.EnumerateNoExit([&](ELFSection* section) {
        Elf64_Shdr* shdr = section->GetShdr();
        Elf64_Phdr* phdr = section->GetProgSection()->GetPhdr();
        assert(shdr->sh_type == SHT_PROGBITS);
        shdr->sh_offset = shdr->sh_addr - phdr->p_vaddr + phdr->p_offset;
        fwrite(shdr, 1, sizeof(Elf64_Shdr), stream);
    });

    // Write String table section header
    Elf64_Shdr strtabShdr = {};
    strtabShdr.sh_name = stringSize - 10; // offset of ".shstrtab"
    strtabShdr.sh_type = SHT_STRTAB;
    strtabShdr.sh_offset = ftell(stream) + sizeof(Elf64_Shdr);
    strtabShdr.sh_size = stringSize;
    strtabShdr.sh_addralign = 1;
    fwrite(&strtabShdr, 1, sizeof(Elf64_Shdr), stream);

    // Write String table data
    fputc(0, stream); // initial null byte
    m_sections.EnumerateNoExit([&](ELFSection* section) {
        const std::string_view& name = section->GetName();
        if (name.size() > 0) {
            fwrite(name.data(), 1, name.size(), stream);
            fputc(0, stream);
        }
    });
    constexpr char shstrtabName[] = ".shstrtab";
    fwrite(shstrtabName, 1, 10, stream);
    return true;
}
