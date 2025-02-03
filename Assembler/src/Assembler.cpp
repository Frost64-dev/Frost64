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

#include <common/util.h>

#include <libarch/Instruction.hpp>

Section::Section(char* name, uint64_t name_size, uint64_t offset)
    : m_name(name), m_name_size(name_size), m_offset(offset) {
}

Section::~Section() {
}

char const* Section::GetName() const {
    return m_name;
}

uint64_t Section::GetNameSize() const {
    return m_name_size;
}

uint64_t Section::GetOffset() const {
    return m_offset;
}

Assembler::Assembler()
    : m_current_offset(0), m_buffer() {
}

Assembler::~Assembler() {
}

void Assembler::assemble(const LinkedList::RearInsertLinkedList<InsEncoding::Label>& labels, uint64_t base_address) {
    using namespace InsEncoding;
    labels.Enumerate([&](Label* label) {
        label->blocks.Enumerate([&](Block* block) {
            size_t name_size = label->name_size + block->name_size;
            char* name = new char[name_size + 1];
            memcpy(name, label->name, label->name_size);
            memcpy(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(name) + label->name_size), block->name, block->name_size);
            name[name_size] = '\0';
            Section* section = new Section(name, name_size, m_current_offset);
            m_sections.insert(section);

            block->data_blocks.Enumerate([&](Data* data) {
                if (data->type) { // instruction
                    Instruction* instruction = static_cast<Instruction*>(data->data);
                    uint8_t i_data[64] = {};
                    size_t bytes_written = EncodeInstruction(instruction, i_data, 64, m_current_offset);
                    m_buffer.Write(m_current_offset, i_data, bytes_written);
                    m_current_offset += bytes_written;
                } else { // raw data
                    switch (RawData* raw_data = static_cast<RawData*>(data->data); raw_data->type) {
                    case RawDataType::RAW:
                        m_buffer.Write(m_current_offset, static_cast<uint8_t*>(raw_data->data), raw_data->data_size);
                        m_current_offset += raw_data->data_size;
                        break;
                    case RawDataType::LABEL: {
                        Label* i_label = static_cast<Label*>(raw_data->data);
                        Block* i_block = i_label->blocks.get(0);
                        uint64_t* offset = new uint64_t;
                        *offset = m_current_offset;
                        i_block->jumps_to_here.insert(offset);
                        uint64_t temp_offset = 0xDEAD'BEEF'DEAD'BEEF;
                        m_buffer.Write(m_current_offset, reinterpret_cast<uint8_t*>(&temp_offset), 8);
                        m_current_offset += 8;
                        break;
                    }
                    case RawDataType::SUBLABEL: {
                        Block* i_block = static_cast<Block*>(raw_data->data);
                        uint64_t* offset = new uint64_t;
                        *offset = m_current_offset;
                        i_block->jumps_to_here.insert(offset);
                        uint64_t temp_offset = 0xDEAD'BEEF'DEAD'BEEF;
                        m_buffer.Write(m_current_offset, reinterpret_cast<uint8_t*>(&temp_offset), 8);
                        m_current_offset += 8;
                        break;
                    }
                    case RawDataType::ASCII:
                    case RawDataType::ASCIIZ: // null terminator is already handled by the parser
                        m_buffer.Write(m_current_offset, static_cast<uint8_t*>(raw_data->data), raw_data->data_size);
                        m_current_offset += raw_data->data_size;
                        break;
                    case RawDataType::ALIGNMENT: {
                        uint64_t align = *static_cast<uint64_t*>(raw_data->data);
                        if (!IS_POWER_OF_TWO(align))
                            error("Alignment must be a power of 2", raw_data->file_name, raw_data->line);
                        uint64_t bytes_to_add = ALIGN_UP_BASE2(m_current_offset, align) - m_current_offset;
                        // fill the space with `nop`s
                        uint8_t* i_data = new uint8_t[bytes_to_add];
                        uint8_t nop = static_cast<uint8_t>(Opcode::NOP);
                        memset(i_data, nop, bytes_to_add);
                        m_buffer.Write(m_current_offset, i_data, bytes_to_add);
                        m_current_offset += bytes_to_add;
                        delete[] i_data;
                        break;
                    }
                    }
                }
            });
        });
    });
    // Enumerate through the labels, and the blocks within them again and fill in the jumps
    uint64_t section_index = 0;
    labels.Enumerate([&](Label* label) {
        label->blocks.Enumerate([&](Block* block) {
            Section* section = m_sections.get(section_index);
            uint64_t real_offset = section->GetOffset() + base_address;
            block->jumps_to_here.Enumerate([&](const uint64_t* offset) {
                m_buffer.Write(*offset, reinterpret_cast<uint8_t*>(&real_offset), 8);
            });
            section_index++;
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
    m_current_offset = 0;
}

[[noreturn]] void Assembler::error(const char* message, const std::string& file_name, size_t line) {
    fprintf(stderr, "Assembler error at %s:%zu: %s\n", file_name.c_str(), line, message);
    exit(1);
}