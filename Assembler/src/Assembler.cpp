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

Section::Section(char* name, uint64_t nameSize, uint64_t offset)
    : m_name(name), m_nameSize(nameSize), m_offset(offset) {
}

Section::~Section() {
}

char const* Section::GetName() const {
    return m_name;
}

uint64_t Section::GetNameSize() const {
    return m_nameSize;
}

uint64_t Section::GetOffset() const {
    return m_offset;
}

Assembler::Assembler()
    : m_currentOffset(0), m_buffer() {
}

Assembler::~Assembler() {
}

void Assembler::assemble(const LinkedList::RearInsertLinkedList<InsEncoding::Label>& labels, uint64_t baseAddress) {
    using namespace InsEncoding;
    labels.Enumerate([&](Label* label) {
        label->blocks.Enumerate([&](Block* block) {
            size_t nameSize = label->nameSize + block->nameSize;
            char* name = new char[nameSize + 1];
            memcpy(name, label->name, label->nameSize);
            memcpy(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(name) + label->nameSize), block->name, block->nameSize);
            name[nameSize] = '\0';
            Section* section = new Section(name, nameSize, m_currentOffset);
            m_sections.insert(section);

            block->dataBlocks.Enumerate([&](Data* data) {
                if (data->type) { // instruction
                    Instruction* instruction = static_cast<Instruction*>(data->data);
                    uint8_t i_data[64] = {};
                    size_t bytesWritten = EncodeInstruction(instruction, i_data, 64, m_currentOffset);
                    m_buffer.Write(m_currentOffset, i_data, bytesWritten);
                    m_currentOffset += bytesWritten;
                } else { // raw data
                    switch (RawData* rawData = static_cast<RawData*>(data->data); rawData->type) {
                    case RawDataType::RAW:
                        m_buffer.Write(m_currentOffset, static_cast<uint8_t*>(rawData->data), rawData->dataSize);
                        m_currentOffset += rawData->dataSize;
                        break;
                    case RawDataType::LABEL: {
                        Label* i_label = static_cast<Label*>(rawData->data);
                        Block* i_block = i_label->blocks.get(0);
                        uint64_t* offset = new uint64_t;
                        *offset = m_currentOffset;
                        i_block->jumpsToHere.insert(offset);
                        uint64_t tempOffset = 0xDEAD'BEEF'DEAD'BEEF;
                        m_buffer.Write(m_currentOffset, reinterpret_cast<uint8_t*>(&tempOffset), 8);
                        m_currentOffset += 8;
                        break;
                    }
                    case RawDataType::SUBLABEL: {
                        Block* i_block = static_cast<Block*>(rawData->data);
                        uint64_t* offset = new uint64_t;
                        *offset = m_currentOffset;
                        i_block->jumpsToHere.insert(offset);
                        uint64_t tempOffset = 0xDEAD'BEEF'DEAD'BEEF;
                        m_buffer.Write(m_currentOffset, reinterpret_cast<uint8_t*>(&tempOffset), 8);
                        m_currentOffset += 8;
                        break;
                    }
                    case RawDataType::ASCII:
                    case RawDataType::ASCIIZ: // null terminator is already handled by the parser
                        m_buffer.Write(m_currentOffset, static_cast<uint8_t*>(rawData->data), rawData->dataSize);
                        m_currentOffset += rawData->dataSize;
                        break;
                    case RawDataType::ALIGNMENT: {
                        uint64_t align = *static_cast<uint64_t*>(rawData->data);
                        if (!IS_POWER_OF_TWO(align))
                            error("Alignment must be a power of 2", rawData->fileName, rawData->line);
                        uint64_t bytesToAdd = ALIGN_UP_BASE2(m_currentOffset, align) - m_currentOffset;
                        // fill the space with `nop`s
                        uint8_t* i_data = new uint8_t[bytesToAdd];
                        uint8_t nop = static_cast<uint8_t>(Opcode::NOP);
                        memset(i_data, nop, bytesToAdd);
                        m_buffer.Write(m_currentOffset, i_data, bytesToAdd);
                        m_currentOffset += bytesToAdd;
                        delete[] i_data;
                        break;
                    }
                    }
                }
            });
        });
    });
    // Enumerate through the labels, and the blocks within them again and fill in the jumps
    uint64_t sectionIndex = 0;
    labels.Enumerate([&](Label* label) {
        label->blocks.Enumerate([&](Block* block) {
            Section* section = m_sections.get(sectionIndex);
            uint64_t realOffset = section->GetOffset() + baseAddress;
            block->jumpsToHere.Enumerate([&](const uint64_t* offset) {
                m_buffer.Write(*offset, reinterpret_cast<uint8_t*>(&realOffset), 8);
            });
            sectionIndex++;
        });
    });
}

const Buffer& Assembler::GetBuffer() const {
    return m_buffer;
}

void Assembler::Clear() {
    m_buffer.Clear();
    m_sections.Enumerate([&](Section* section) {
        delete[] section->GetName();
        delete section;
    });
    m_sections.clear();
    m_currentOffset = 0;
}

[[noreturn]] void Assembler::error(const char* message, const std::string& fileName, size_t line) {
    fprintf(stderr, "Assembler error at %s:%zu: %s\n", fileName.c_str(), line, message);
    exit(1);
}