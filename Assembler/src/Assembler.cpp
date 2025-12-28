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

#include "Assembler.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <Common/Util.hpp>

#include "../../LibExec/include/LibExec/Exec.hpp"

DataBlock::DataBlock(char* name, uint64_t nameSize, uint64_t offset)
    : m_name(name), m_nameSize(nameSize), m_offset(offset) {
}

DataBlock::~DataBlock() {
}

char const* DataBlock::GetName() const {
    return m_name;
}

uint64_t DataBlock::GetNameSize() const {
    return m_nameSize;
}

uint64_t DataBlock::GetOffset() const {
    return m_offset;
}

Assembler::Assembler() {
}

Assembler::~Assembler() {
}

void Assembler::assemble(const LinkedList::RearInsertLinkedList<InsEncoding::Section>& sections, AssemblerFileFormat format, FILE* outStream) {
    using namespace InsEncoding;

    Section* prevSection = nullptr;
    uint64_t prevSize = 0; // previous section size

    sections.Enumerate([&](Section* section) {
        if (!section->startingAddressSet) {
            section->startingAddress = prevSection == nullptr ? 0 : prevSection->startingAddress + prevSize;
            section->startingAddressSet = true;
        }

        DataSection* dataSection = new DataSection();
        dataSection->startingAddress = section->startingAddress;
        dataSection->name = section->name;
        dataSection->nameSize = section->nameSize;
        m_sections.insert(dataSection);
        Buffer& buffer = dataSection->buffer;
        uint64_t currentOffset = 0;

        section->labels.Enumerate([&](Label* label) {
            label->blocks.Enumerate([&](Block* block) {
                size_t nameSize = label->nameSize + block->nameSize;
                char* name = new char[nameSize + 1];
                memcpy(name, label->name, label->nameSize);
                memcpy(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(name) + label->nameSize), block->name, block->nameSize);
                name[nameSize] = '\0';
                DataBlock* dataBlock = new DataBlock(name, nameSize, currentOffset);
                m_dataBlocks.insert(dataBlock);

                block->dataBlocks.Enumerate([&](Data* data) {
                    if (data->type) { // instruction
                        Instruction* instruction = static_cast<Instruction*>(data->data);
                        if (instruction == nullptr)
                            error("Instruction is null", "", 0);
                        uint8_t i_data[64] = {};
                        size_t bytesWritten = EncodeInstruction(instruction, i_data, 64, currentOffset, dataSection);
                        buffer.Write(currentOffset, i_data, bytesWritten);
                        currentOffset += bytesWritten;
                    } else { // raw data
                        switch (RawData* rawData = static_cast<RawData*>(data->data); rawData->type) {
                        case RawDataType::RAW:
                            buffer.Write(currentOffset, static_cast<uint8_t*>(rawData->data), rawData->dataSize);
                            currentOffset += rawData->dataSize;
                            break;
                        case RawDataType::LABEL: {
                            Label* i_label = static_cast<Label*>(rawData->data);
                            Block* i_block = i_label->blocks.get(0);
                            Block::Jump* jump = new Block::Jump;
                            jump->offset = currentOffset;
                            jump->section = dataSection;
                            i_block->jumpsToHere.insert(jump);
                            uint64_t tempOffset = 0xDEAD'BEEF'DEAD'BEEF;
                            buffer.Write(currentOffset, reinterpret_cast<uint8_t*>(&tempOffset), 8);
                            currentOffset += 8;
                            break;
                        }
                        case RawDataType::SUBLABEL: {
                            Block* i_block = static_cast<Block*>(rawData->data);
                            Block::Jump* jump = new Block::Jump;
                            jump->offset = currentOffset;
                            jump->section = dataSection;
                            i_block->jumpsToHere.insert(jump);
                            uint64_t tempOffset = 0xDEAD'BEEF'DEAD'BEEF;
                            buffer.Write(currentOffset, reinterpret_cast<uint8_t*>(&tempOffset), 8);
                            currentOffset += 8;
                            break;
                        }
                        case RawDataType::ASCII:
                        case RawDataType::ASCIIZ: // null terminator is already handled by the parser
                            buffer.Write(currentOffset, static_cast<uint8_t*>(rawData->data), rawData->dataSize);
                            currentOffset += rawData->dataSize;
                            break;
                        case RawDataType::ALIGNMENT: {
                            uint64_t align = *static_cast<uint64_t*>(rawData->data);
                            if (!IS_POWER_OF_TWO(align))
                                error("Alignment must be a power of 2", rawData->fileName, rawData->line);
                            uint64_t globalOffset = section->startingAddress + currentOffset;
                            uint64_t bytesToAdd = ALIGN_UP_BASE2(globalOffset, align) - globalOffset;
                            // fill the space with `nop`s
                            uint8_t* i_data = new uint8_t[bytesToAdd];
                            uint8_t nop = static_cast<uint8_t>(Opcode::NOP);
                            memset(i_data, nop, bytesToAdd);
                            buffer.Write(currentOffset, i_data, bytesToAdd);
                            currentOffset += bytesToAdd;
                            delete[] i_data;
                            break;
                        }
                        case RawDataType::SKIP: {
                            uint64_t skip = *static_cast<uint64_t*>(rawData->data);
                            uint8_t* i_data = new uint8_t[skip];
                            uint8_t nop = static_cast<uint8_t>(Opcode::NOP);
                            memset(i_data, nop, skip);
                            buffer.Write(currentOffset, i_data, skip);
                            currentOffset += skip;
                            delete[] i_data;
                            break;
                        }
                        default:
                            break;
                        }
                    }
                });
            });
        });

        dataSection->size = currentOffset;
        prevSize = currentOffset;
        prevSection = section;
    });

    // Enumerate through everything and fill in the jumps
    uint64_t dataBlockIndex = 0;
    sections.Enumerate([&](Section* section) {
        section->labels.Enumerate([&](Label* label) {
            label->blocks.Enumerate([&](Block* block) {
                DataBlock* dataBlock = m_dataBlocks.get(dataBlockIndex);
                uint64_t realOffset = dataBlock->GetOffset() + section->startingAddress;
                block->jumpsToHere.Enumerate([&](Block::Jump* j) {
                    j->section->buffer.Write(j->offset, reinterpret_cast<uint8_t*>(&realOffset), 8);
                });
                dataBlockIndex++;
            });
       });
    });

    if (format == AssemblerFileFormat::BINARY) {
        // Just write all sections one after another
        m_sections.Enumerate([&](DataSection* section) {
            uint8_t* data = new uint8_t[section->size];
            section->buffer.Read(0, data, section->size);
            fwrite(data, 1, section->size, outStream);
            delete[] data;
        });
    } else if (format == AssemblerFileFormat::ELF) {
        ELFExecutable* elf = new ELFExecutable();
        if (!elf->Create())
            error("Failed to create ELF executable", "", 0);

        LinkedList::RearInsertLinkedList<char> names;

        // Add sections to ELF
        m_sections.Enumerate([&](DataSection* section) {
            // Program section stuff
            ELFProgramSection* progSection = elf->CreateNewProgramSection();
            progSection->SetType(PT_LOAD);
            progSection->SetVirtAddr(section->startingAddress);
            progSection->SetPhysAddr(section->startingAddress);
            progSection->SetAlignment(1);

            // Section header stuff
            ELFSection* sectionHeader = elf->CreateNewSection();
            sectionHeader->SetType(SHT_PROGBITS);
            sectionHeader->SetRegion(section->startingAddress, section->size, 1);

            // Section name
            char* name = new char[section->nameSize + 1];
            strncpy(name, section->name, section->nameSize);
            name[section->nameSize] = '\0';
            sectionHeader->SetName(name);
            names.insert(name);

            // Section data
            uint8_t* data = new uint8_t[section->size];
            section->buffer.Read(0, data, section->size);
            progSection->SetData(data, section->size);
            delete[] data;

#define EQUALS(s1, s2) (strcmp(s1, s2) == 0)

            // Program section & section header permissions
            if (EQUALS(name, ".text")) {
                progSection->SetFlags(PF_R | PF_X);
                sectionHeader->SetFlags(SHF_ALLOC | SHF_EXECINSTR);
            } else if (EQUALS(name, ".rodata") || EQUALS(name, ".rodata1")) {
                progSection->SetFlags(PF_R);
                sectionHeader->SetFlags(SHF_ALLOC);
            } else { // treat all other section types as read/write
                progSection->SetFlags(PF_R | PF_W);
                sectionHeader->SetFlags(SHF_ALLOC | SHF_WRITE);
            }
#undef EQUALS

            sectionHeader->SetProgSection(progSection);

        });

        DataSection* firstDataSection = m_sections.get(0);
        if (firstDataSection == nullptr)
            error("Invalid first data section", "", 0);
        elf->SetEntryPoint(firstDataSection->startingAddress);

        if (!elf->WriteToStream(outStream))
            error("Failed to write ELF to stream", "", 0);

        names.Enumerate([&](char* name) {
           delete[] name;
        });
        names.clear();
    }
}


void Assembler::Clear() {
    m_sections.Enumerate([&](DataSection* section) {
        section->buffer.Clear();
        delete[] section->name;
        delete section;
    });
    m_sections.clear();
    m_dataBlocks.Enumerate([&](DataBlock* dataBlock) {
        delete[] dataBlock->GetName();
        delete dataBlock;
    });
    m_dataBlocks.clear();
}

[[noreturn]] void Assembler::error(const char* message, const std::string& fileName, size_t line) {
    fprintf(stderr, "Assembler error at %s:%zu: %s\n", fileName.c_str(), line, message);
    exit(1);
}