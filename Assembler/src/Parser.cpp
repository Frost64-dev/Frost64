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

#include "Parser.hpp"

#include <string.h>

#include <cstdint>
#include <cstdlib>
#include <libarch/Instruction.hpp>
#include <libarch/Operand.hpp>

Parser::Parser()
    : m_base_address(0), m_opcodeTableInitialised(false), m_registerTableInitialised(false) {
}

Parser::~Parser() {
}

#define EQUALS(str1, str2) (strlen(str2) == name_size && strncmp(str1, str2, name_size) == 0)

void Parser::parse(const LinkedList::RearInsertLinkedList<Token>& tokens) {
    using namespace InsEncoding;
    Label* current_label = nullptr;
    Block* current_block = nullptr;
    Data* current_data = nullptr;
    Operand* current_operand = nullptr;
    bool in_directive = false;
    bool in_instruction = true;
    bool in_operand = false;

    m_base_address = 0;
    bool base_address_set = false;
    bool base_address_parsed = false;

    // First scan for labels
    tokens.Enumerate([&](Token* token) -> void {
        if (token->type == TokenType::BLABEL) {
            Label* label = new Label;
            label->name = static_cast<char*>(token->data);
            label->name_size = token->data_size - sizeof(char); // remove the colon at the end
            m_labels.insert(label);
            current_label = label;

            Block* block = new Block;
            block->name = const_cast<char*>("");
            block->name_size = 0;
            current_label->blocks.insert(block);
            current_block = block;
            in_instruction = false;
        } else if (token->type == TokenType::BSUBLABEL) {
            Block* block = new Block;
            block->name = reinterpret_cast<char*>(reinterpret_cast<uint64_t>(token->data) + sizeof(char)); // remove the dot at the start
            block->name_size = token->data_size - 2 * sizeof(char); // remove the dot at the start and colon at the end
            current_label->blocks.insert(block);
            current_block = block;
            in_instruction = false;
        }
    });

    tokens.Enumerate([&](Token* token, uint64_t index) -> bool {
#ifdef ASSEMBLER_DEBUG
        printf("Token: \"%.*s\", index = %lu, type = %lu\n", static_cast<int>(token->data_size), static_cast<char*>(token->data), index, static_cast<unsigned long int>(token->type));
#else
        (void)index;
#endif

        if (in_directive) {
            if (token->type == TokenType::NUMBER && base_address_parsed && !base_address_set) {
                m_base_address = strtoull(static_cast<const char*>(token->data), nullptr, 0);
                base_address_set = true;
                in_directive = false;
                return true;
            }
            if (static_cast<RawData*>(current_data->data)->type == RawDataType::ASCII || static_cast<RawData*>(current_data->data)->type == RawDataType::ASCIIZ) {
                if (token->type == TokenType::STRING) {
                    RawData* raw_data = static_cast<RawData*>(current_data->data);
                    std::string out_str;
                    // need to loop through the string and resolve escape sequences
                    for (uint64_t i = 1; i < (token->data_size - 1) /* remove the start and end quotes */; i++) {
                        if (static_cast<const char*>(token->data)[i] == '\\') {
                            i++;
                            switch (static_cast<const char*>(token->data)[i]) {
                            case 'n':
                                out_str += '\n';
                                break;
                            case 't':
                                out_str += '\t';
                                break;
                            case 'r':
                                out_str += '\r';
                                break;
                            case '0':
                                out_str += '\0';
                                break;
                            case '\\':
                                out_str += '\\';
                                break;
                            case '\'':
                                out_str += '\'';
                                break;
                            case '\"':
                                out_str += '\"';
                                break;
                            case 'x': {
                                // next 2 chars are hex
                                uint8_t hex = 0;
                                for (uint8_t j = 0; j < 2; j++) {
                                    hex *= 16;
                                    i++;
                                    if (i + 1 >= token->data_size)
                                        error("Invalid escape sequence", token);
                                    if (static_cast<const char*>(token->data)[i] >= '0' && static_cast<const char*>(token->data)[i] <= '9')
                                        hex += static_cast<const char*>(token->data)[i] - '0';
                                    else if (static_cast<const char*>(token->data)[i] >= 'a' && static_cast<const char*>(token->data)[i] <= 'f')
                                        hex += static_cast<const char*>(token->data)[i] - 'a' + 10;
                                    else if (static_cast<const char*>(token->data)[i] >= 'A' && static_cast<const char*>(token->data)[i] <= 'F')
                                        hex += static_cast<const char*>(token->data)[i] - 'A' + 10;
                                    else
                                        error("Invalid escape sequence", token);
                                }
                                out_str += static_cast<char>(hex);
                                break;
                            }
                            default:
                                error("Invalid escape sequence", token);
                            }
                        } else
                            out_str += static_cast<const char*>(token->data)[i];
                    }
                    uint64_t str_len = out_str.size();
                    raw_data->data_size = str_len + (raw_data->type == RawDataType::ASCIIZ ? 1 : 0);
                    raw_data->data = new char[raw_data->data_size];
                    memcpy(raw_data->data, out_str.c_str(), str_len);
                    if (raw_data->type == RawDataType::ASCIIZ)
                        static_cast<char*>(raw_data->data)[str_len] = 0;
                } else
                    error("Invalid token after directive", token, true);
                in_directive = false;
                return true;
            }
            switch (token->type) {
            case TokenType::NUMBER:
                switch (static_cast<RawData*>(current_data->data)->data_size) {
                case 1: { // byte
                    uint8_t* data = new uint8_t;
                    *data = strtol(static_cast<const char*>(token->data), nullptr, 0) & 0xFF;
                    static_cast<RawData*>(current_data->data)->data = data;
                    break;
                }
                case 2: { // word
                    uint16_t* data = new uint16_t;
                    *data = strtol(static_cast<const char*>(token->data), nullptr, 0) & 0xFFFF;
                    static_cast<RawData*>(current_data->data)->data = data;
                    break;
                }
                case 4: { // dword
                    uint32_t* data = new uint32_t;
                    *data = strtoul(static_cast<const char*>(token->data), nullptr, 0) & 0xFFFF'FFFF;
                    static_cast<RawData*>(current_data->data)->data = data;
                    break;
                }
                case 8: { // qword
                    uint64_t* data = new uint64_t;
                    *data = strtoull(static_cast<const char*>(token->data), nullptr, 0);
                    static_cast<RawData*>(current_data->data)->data = data;
                    break;
                }
                default:
                    error("Invalid data size for directive", token);
                    break;
                }
                if (static_cast<RawData*>(current_data->data)->type != RawDataType::ALIGNMENT)
                    static_cast<RawData*>(current_data->data)->type = RawDataType::RAW;
                break;
            case TokenType::LABEL: {
                if (current_operand != nullptr)
                    error("Invalid label location", token);
                Label* label = nullptr;
                // find the label
                char* name = new char[token->data_size + 1];
                strncpy(name, static_cast<const char*>(token->data), token->data_size);
                name[token->data_size] = 0;
                m_labels.Enumerate([&](Label* i_label) -> bool {
                    if (i_label->name_size < token->data_size) // strncmp can only properly handle strings of equal or greater length.
                        return true;
                    if (strncmp(i_label->name, name, i_label->name_size) == 0) {
                        label = i_label;
                        return false;
                    }
                    return true;
                });
                delete[] name;
                if (label == nullptr)
                    error("Invalid label", token, true);
                static_cast<RawData*>(current_data->data)->type = RawDataType::LABEL;
                static_cast<RawData*>(current_data->data)->data = label;
                break;
            }
            case TokenType::SUBLABEL: {
                if (current_operand != nullptr)
                    error("Invalid sublabel location", token);
                Block* block = nullptr;
                // find the block
                char* name = new char[token->data_size + 1];
                strncpy(name, static_cast<const char*>(token->data), token->data_size);
                name[token->data_size] = 0;
                if (name[0] == '.')
                    name = &(name[1]);
                else {
                    delete[] name;
                    error("Invalid sublabel name", token, true);
                }
                current_label->blocks.Enumerate([&](Block* i_block) -> bool {
                    if (i_block->name_size < (token->data_size - 1)) // strncmp can only properly handle strings of equal or greater length
                        return true;
                    if (strncmp(i_block->name, name, i_block->name_size) == 0) {
                        block = i_block;
                        return false;
                    }
                    return true;
                });
                delete[] reinterpret_cast<char*>(reinterpret_cast<uint64_t>(name) - sizeof(char));
                if (block == nullptr)
                    error("Invalid sublabel", token, true);
                static_cast<RawData*>(current_data->data)->type = RawDataType::SUBLABEL;
                static_cast<RawData*>(current_data->data)->data = block;
                break;
            }
            default:
                error("Invalid token after directive", token);
                break;
            }
            in_directive = false;
        } else if (token->type == TokenType::COMMA) {
            if (!in_instruction)
                error("Comma (',') outside of instruction.", token);
            in_operand = true;
        } else if (token->type == TokenType::DIRECTIVE) {
            if (in_operand)
                error("Directive inside operand", token);
            if (strncmp(static_cast<char*>(token->data), "org", token->data_size) == 0) {
                if (base_address_set)
                    error("Multiple base addresses", token);
                base_address_set = false;
                base_address_parsed = true;
                in_directive = true;
                in_instruction = false;
                return true;
            }
            Data* data = new Data;
            RawData* raw_data = new RawData;
            data->data = raw_data;
            data->type = false;
            raw_data->file_name = token->file_name;
            raw_data->line = token->line;
            if (strncmp(static_cast<char*>(token->data), "db", token->data_size) == 0)
                static_cast<RawData*>(data->data)->data_size = 1;
            else if (strncmp(static_cast<char*>(token->data), "dw", token->data_size) == 0)
                static_cast<RawData*>(data->data)->data_size = 2;
            else if (strncmp(static_cast<char*>(token->data), "dd", token->data_size) == 0)
                static_cast<RawData*>(data->data)->data_size = 4;
            else if (strncmp(static_cast<char*>(token->data), "dq", token->data_size) == 0)
                static_cast<RawData*>(data->data)->data_size = 8;
            else if (strncmp(static_cast<char*>(token->data), "align", token->data_size) == 0) {
                static_cast<RawData*>(data->data)->data_size = 8;
                static_cast<RawData*>(data->data)->type = RawDataType::ALIGNMENT;
            }
            else if (strncmp(static_cast<char*>(token->data), "ascii", token->data_size) == 0)
                static_cast<RawData*>(data->data)->type = RawDataType::ASCII;
            else if (strncmp(static_cast<char*>(token->data), "asciiz", token->data_size) == 0)
                static_cast<RawData*>(data->data)->type = RawDataType::ASCIIZ;
            else
                error("Invalid directive", token);
            static_cast<RawData*>(data->data)->data = nullptr;
            current_block->data_blocks.insert(data);
            current_data = data;
            in_directive = true;
            in_instruction = false;
        } else if (token->type == TokenType::BLABEL) {
            if (in_instruction && in_operand)
                error("Label inside operand", token);
            // find the label
            Label* label = nullptr;
            char* name = new char[token->data_size + 1];
            strncpy(name, static_cast<const char*>(token->data), token->data_size);
            name[token->data_size] = 0;
            m_labels.Enumerate([&](Label* i_label) -> bool {
                if (i_label->name_size < (token->data_size - 1)) // strncmp can only properly handle strings of equal or greater length. -1 to remove the colon
                    return true;
                if (strncmp(i_label->name, name, i_label->name_size) == 0) {
                    label = i_label;
                    return false;
                }
                return true;
            });
            delete[] name;
            current_label = label;
            if (current_label == nullptr)
                error("Invalid label", token, true);
            current_block = current_label->blocks.get(0);
            in_instruction = false;
        } else if (token->type == TokenType::BSUBLABEL) {
            if (in_instruction && in_operand)
                error("Sublabel inside operand", token);
            // find the block
            Block* block = nullptr;
            char* name = new char[token->data_size + 1];
            strncpy(name, static_cast<const char*>(token->data), token->data_size);
            name[token->data_size] = 0;
            if (name[0] == '.')
                name = &(name[1]);
            else {
                delete[] name;
                error("Invalid sublabel name", token, true);
            }
            current_label->blocks.Enumerate([&](Block* i_block) -> bool {
                if (i_block->name_size < (token->data_size - 2)) // strncmp can only properly handle strings of equal or greater length
                    return true;
                if (strncmp(i_block->name, name, i_block->name_size) == 0) {
                    block = i_block;
                    return false;
                }
                return true;
            });
            delete[] reinterpret_cast<char*>(reinterpret_cast<uint64_t>(name) - sizeof(char));
            current_block = block;
            if (current_block == nullptr)
                error("Invalid sublabel", token, true);
            in_instruction = false;
        } else if (token->type == TokenType::INSTRUCTION) {
            if (in_operand)
                error("Instruction inside operand", token);
            Data* data = new Data;
            Instruction* instruction = new Instruction(GetOpcode(static_cast<const char*>(token->data), token->data_size), token->file_name, token->line);
            data->type = true;
            data->data = instruction;
            current_block->data_blocks.insert(data);
            current_data = data;
            if (uint64_t name_size = token->data_size; EQUALS(static_cast<const char*>(token->data), "ret") || EQUALS(static_cast<const char*>(token->data), "nop") || EQUALS(static_cast<const char*>(token->data), "hlt") || EQUALS(static_cast<const char*>(token->data), "pusha") || EQUALS(static_cast<const char*>(token->data), "popa") || EQUALS(static_cast<const char*>(token->data), "iret") || EQUALS(static_cast<const char*>(token->data), "syscall") || EQUALS(static_cast<const char*>(token->data), "sysret")) {
                in_instruction = false;
                in_operand = false;
            } else {
                in_instruction = true;
                in_operand = true;
            }
        } else if (in_instruction) {
            if (in_operand) {
                if (token->type == TokenType::SIZE) {
                    if (current_operand != nullptr)
                        error("Invalid size location", token);
                    Operand* operand = new Operand;
                    operand->complete = false;
                    operand->type = OperandType::UNKNOWN;
                    static_cast<Instruction*>(current_data->data)->operands.insert(operand);
                    current_operand = operand;
                    if (size_t name_size = token->data_size; EQUALS(static_cast<const char*>(token->data), "byte"))
                        current_operand->size = OperandSize::BYTE;
                    else if (EQUALS(static_cast<const char*>(token->data), "word"))
                        current_operand->size = OperandSize::WORD;
                    else if (EQUALS(static_cast<const char*>(token->data), "dword"))
                        current_operand->size = OperandSize::DWORD;
                    else if (EQUALS(static_cast<const char*>(token->data), "qword"))
                        current_operand->size = OperandSize::QWORD;
                    else
                        error("Invalid size", token);
                } else if (token->type == TokenType::LBRACKET) {
                    if (current_operand == nullptr) {
                        Operand* operand = new Operand;
                        operand->size = OperandSize::QWORD;
                        operand->complete = false;
                        static_cast<Instruction*>(current_data->data)->operands.insert(operand);
                        current_operand = operand;
                    }
                    current_operand->type = OperandType::POTENTIAL_MEMORY;
                } else if (token->type == TokenType::RBRACKET) {
                    if (current_operand == nullptr || !(current_operand->type == OperandType::COMPLEX || current_operand->type == OperandType::MEMORY))
                        error("Invalid operand", token);
                    if (!(current_operand->complete) && current_operand->type != OperandType::COMPLEX)
                        error("Invalid operand", token);
                    current_operand = nullptr;
                    in_operand = false;
                } else if (token->type == TokenType::NUMBER) {
                    if (current_operand == nullptr) { // must be immediate
                        Operand* operand = new Operand;
                        operand->complete = false;
                        static_cast<Instruction*>(current_data->data)->operands.insert(operand);
                        current_operand = operand;
                        current_operand->type = OperandType::IMMEDIATE;
                        if (long imm = static_cast<long>(strtoull(static_cast<const char*>(token->data), nullptr, 0)); imm >= INT8_MIN && imm <= INT8_MAX) {
                            current_operand->size = OperandSize::BYTE;
                            uint8_t* imm8 = new uint8_t;
                            *imm8 = static_cast<uint64_t>(imm) & 0xFF;
                            current_operand->data = imm8;
                        } else if (imm >= INT16_MIN && imm <= INT16_MAX) {
                            current_operand->size = OperandSize::WORD;
                            uint16_t* imm16 = new uint16_t;
                            *imm16 = static_cast<uint64_t>(imm) & 0xFFFF;
                            current_operand->data = imm16;
                        } else if (imm >= INT32_MIN && imm <= INT32_MAX) {
                            current_operand->size = OperandSize::DWORD;
                            uint32_t* imm32 = new uint32_t;
                            *imm32 = static_cast<uint64_t>(imm) & 0xFFFF'FFFF;
                            current_operand->data = imm32;
                        } else {
                            current_operand->size = OperandSize::QWORD;
                            uint64_t* imm64 = new uint64_t;
                            *imm64 = static_cast<uint64_t>(imm);
                            current_operand->data = imm64;
                        }
                        current_operand->complete = true;
                        current_operand = nullptr;
                        in_operand = false;
                    } else if (current_operand->type == OperandType::POTENTIAL_MEMORY) { // must be memory
                        // current_operand->size = OperandSize::QWORD;
                        uint64_t* addr = new uint64_t;
                        *addr = static_cast<uint64_t>(strtoull(static_cast<const char*>(token->data), nullptr, 0));
                        current_operand->data = addr;
                        current_operand->type = OperandType::MEMORY;
                        current_operand->complete = true;
                    } else if (current_operand->type == OperandType::COMPLEX || current_operand->type == OperandType::MEMORY) { // only other option is complex
                        if (current_operand->type == OperandType::MEMORY) {
                            current_operand->type = OperandType::COMPLEX;
                            ComplexData* data = new ComplexData;
                            data->base.present = true;
                            data->base.data.imm.size = OperandSize::QWORD;
                            data->base.data.imm.data = current_operand->data;
                            data->base.type = ComplexItem::Type::IMMEDIATE;
                            data->index.present = false;
                            data->offset.present = false;
                            data->stage = ComplexData::Stage::BASE;
                            current_operand->data = data;
                        }
                        ComplexData* data = static_cast<ComplexData*>(current_operand->data);
                        long imm = strtoll(static_cast<const char*>(token->data), nullptr, 0);
                        void* i_data;
                        OperandSize data_size;
                        bool negative = imm < 0;
                        if (imm >= INT8_MIN && imm <= INT8_MAX) {
                            data_size = OperandSize::BYTE;
                            uint8_t* imm8 = new uint8_t;
                            *imm8 = static_cast<uint64_t>(imm) & 0xFF;
                            i_data = imm8;
                        } else if (imm >= INT16_MIN && imm <= INT16_MAX) {
                            data_size = OperandSize::WORD;
                            uint16_t* imm16 = new uint16_t;
                            *imm16 = static_cast<uint64_t>(imm) & 0xFFFF;
                            i_data = imm16;
                        } else if (imm >= INT32_MIN && imm <= INT32_MAX) {
                            data_size = OperandSize::DWORD;
                            uint32_t* imm32 = new uint32_t;
                            *imm32 = static_cast<uint64_t>(imm) & 0xFFFF'FFFF;
                            i_data = imm32;
                        } else {
                            data_size = OperandSize::QWORD;
                            uint64_t* imm64 = new uint64_t;
                            *imm64 = static_cast<uint64_t>(imm);
                            i_data = imm64;
                        }
                        switch (data->stage) {
                        case ComplexData::Stage::BASE: {
                            if (data->base.present) {
                                if (!negative || data->index.present || data->offset.present)
                                    error("Invalid immediate location", token);
                                data->offset.present = true;
                                data->offset.data.imm.size = data_size;
                                data->offset.data.imm.data = i_data;
                                data->offset.type = ComplexItem::Type::IMMEDIATE;
                                current_operand->complete = true;
                            } else {
                                data->base.present = true;
                                data->base.data.imm.size = data_size;
                                data->base.data.imm.data = i_data;
                                data->base.type = ComplexItem::Type::IMMEDIATE;
                            }
                            break;
                        }
                        case ComplexData::Stage::INDEX: {
                            if (data->index.present) {
                                if (!negative || data->offset.present)
                                    error("Invalid immediate location", token);
                                data->offset.present = true;
                                data->offset.data.imm.size = data_size;
                                data->offset.data.imm.data = i_data;
                                data->offset.type = ComplexItem::Type::IMMEDIATE;
                                current_operand->complete = true;
                            } else {
                                data->index.present = true;
                                data->index.data.imm.size = data_size;
                                data->index.data.imm.data = i_data;
                                data->index.type = ComplexItem::Type::IMMEDIATE;
                            }
                            break;
                        }
                        case ComplexData::Stage::OFFSET: {
                            if (data->offset.present)
                                error("Invalid immediate location", token);
                            data->offset.present = true;
                            data->offset.data.imm.size = data_size;
                            data->offset.data.imm.data = i_data;
                            data->offset.type = ComplexItem::Type::IMMEDIATE;
                            current_operand->complete = true;
                            break;
                        }
                        default:
                            error("Invalid immediate location", token);
                        }
                    } else
                        error("Invalid immediate location", token);
                } else if (token->type == TokenType::REGISTER) {
                    if (current_operand == nullptr || current_operand->type == OperandType::UNKNOWN) { // must be just a register
                        if (current_operand == nullptr) {
                            Operand* operand = new Operand;
                            operand->complete = false;
                            operand->size = OperandSize::QWORD;
                            static_cast<Instruction*>(current_data->data)->operands.insert(operand);
                            current_operand = operand;
                        }
                        current_operand->type = OperandType::REGISTER;
                        Register* reg = new Register(GetRegister(static_cast<const char*>(token->data), token->data_size));
                        current_operand->data = reg;
                        current_operand->complete = true;
                        current_operand = nullptr;
                        in_operand = false;
                    } else if (current_operand->type == OperandType::POTENTIAL_MEMORY) {
                        current_operand->type = OperandType::COMPLEX;
                        ComplexData* data = new ComplexData;
                        data->base.present = true;
                        Register* reg = new Register(GetRegister(static_cast<const char*>(token->data), token->data_size));
                        data->base.data.reg = reg;
                        data->base.type = ComplexItem::Type::REGISTER;
                        data->index.present = false;
                        data->index.data.raw = 0;
                        data->index.type = ComplexItem::Type::UNKNOWN;
                        data->offset.present = false;
                        data->offset.data.raw = 0;
                        data->offset.type = ComplexItem::Type::UNKNOWN;
                        data->stage = ComplexData::Stage::BASE;
                        current_operand->data = data;
                    } else if (current_operand->type == OperandType::COMPLEX) {
                        switch (ComplexData* data = static_cast<ComplexData*>(current_operand->data); data->stage) {
                        case ComplexData::Stage::BASE: {
                            if (data->base.present)
                                error("Invalid Register location", token);
                            data->base.present = true;
                            Register* reg = new Register(GetRegister(static_cast<const char*>(token->data), token->data_size));
                            data->base.data.reg = reg;
                            data->base.type = ComplexItem::Type::REGISTER;
                            break;
                        }
                        case ComplexData::Stage::INDEX: {
                            if (data->index.present)
                                error("Invalid Register location", token);
                            data->index.present = true;
                            Register* reg = new Register(GetRegister(static_cast<const char*>(token->data), token->data_size));
                            data->index.data.reg = reg;
                            data->index.type = ComplexItem::Type::REGISTER;
                            break;
                        }
                        case ComplexData::Stage::OFFSET: {
                            if (data->offset.present)
                                error("Invalid Register location", token);
                            data->offset.present = true;
                            Register* reg = new Register(GetRegister(static_cast<const char*>(token->data), token->data_size));
                            data->offset.data.reg = reg;
                            data->offset.type = ComplexItem::Type::REGISTER;
                            current_operand->complete = true;
                            break;
                        }
                        }
                    } else
                        error("Invalid Register location", token);
                } else if (token->type == TokenType::LABEL) {
                    if (current_operand != nullptr && !(current_operand->type == OperandType::POTENTIAL_MEMORY || current_operand->type == OperandType::COMPLEX))
                        error("Invalid label location", token);
                    Label* label = nullptr;
                    // find the label
                    char* name = new char[token->data_size + 1];
                    strncpy(name, static_cast<const char*>(token->data), token->data_size);
                    name[token->data_size] = 0;
                    m_labels.Enumerate([&](Label* i_label) -> bool {
                        if (i_label->name_size < token->data_size) // strncmp can only properly handle strings of equal or greater length.
                            return true;
                        if (strncmp(i_label->name, name, i_label->name_size) == 0) {
                            label = i_label;
                            return false;
                        }
                        return true;
                    });
                    delete[] name;
                    if (label == nullptr)
                        error("Invalid label", token, true);
                    if (current_operand == nullptr) {
                        Operand* operand = new Operand;
                        operand->complete = true;
                        operand->type = OperandType::LABEL;
                        operand->data = label;
                        operand->size = OperandSize::QWORD;
                        static_cast<Instruction*>(current_data->data)->operands.insert(operand);
                        current_operand = nullptr;
                        in_operand = false;
                    } else if (current_operand->type == OperandType::POTENTIAL_MEMORY || current_operand->type == OperandType::COMPLEX) {
                        if (current_operand->type == OperandType::POTENTIAL_MEMORY) {
                            current_operand->type = OperandType::COMPLEX;
                            ComplexData* data = new ComplexData;
                            data->base.present = true;
                            data->base.data.label = label;
                            data->base.type = ComplexItem::Type::LABEL;
                            data->index.present = false;
                            data->offset.present = false;
                            data->stage = ComplexData::Stage::BASE;
                            current_operand->data = data;
                        } else {
                            switch (ComplexData* data = static_cast<ComplexData*>(current_operand->data); data->stage) {
                            case ComplexData::Stage::BASE: {
                                if (data->base.present)
                                    error("Invalid label location", token);
                                data->base.present = true;
                                data->base.data.label = label;
                                data->base.type = ComplexItem::Type::LABEL;
                                break;
                            }
                            case ComplexData::Stage::INDEX: {
                                if (data->index.present)
                                    error("Invalid label location", token);
                                data->index.present = true;
                                data->index.data.label = label;
                                data->index.type = ComplexItem::Type::LABEL;
                                break;
                            }
                            case ComplexData::Stage::OFFSET: {
                                if (data->offset.present)
                                    error("Invalid label location", token);
                                data->offset.present = true;
                                data->offset.data.label = label;
                                data->offset.type = ComplexItem::Type::LABEL;
                                current_operand->complete = true;
                                break;
                            }
                            }
                        }
                    }
                } else if (token->type == TokenType::SUBLABEL) {
                    if (current_operand != nullptr && !(current_operand->type == OperandType::POTENTIAL_MEMORY || current_operand->type == OperandType::COMPLEX))
                        error("Invalid sublabel location", token);
                    Block* block = nullptr;
                    // find the block
                    char* name = new char[token->data_size + 1];
                    strncpy(name, static_cast<const char*>(token->data), token->data_size);
                    name[token->data_size] = 0;
                    if (name[0] == '.')
                        name = &(name[1]);
                    else {
                        delete[] name;
                        error("Invalid sublabel name", token, true);
                    }
                    current_label->blocks.Enumerate([&](Block* i_block) -> bool {
                        if (i_block->name_size < (token->data_size - 1)) // strncmp can only properly handle strings of equal or greater length
                            return true;
                        if (strncmp(i_block->name, name, i_block->name_size) == 0) {
                            block = i_block;
                            return false;
                        }
                        return true;
                    });
                    delete[] reinterpret_cast<char*>(reinterpret_cast<uint64_t>(name) - sizeof(char));
                    if (block == nullptr)
                        error("Invalid sublabel", token, true);
                    if (current_operand == nullptr) {
                        Operand* operand = new Operand;
                        operand->complete = true;
                        operand->type = OperandType::SUBLABEL;
                        operand->data = block;
                        operand->size = OperandSize::QWORD;
                        static_cast<Instruction*>(current_data->data)->operands.insert(operand);
                        current_operand = nullptr;
                        in_operand = false;
                    } else if (current_operand->type == OperandType::POTENTIAL_MEMORY) {
                        current_operand->type = OperandType::COMPLEX;
                        ComplexData* data = new ComplexData;
                        data->base.present = true;
                        data->base.data.sublabel = block;
                        data->base.type = ComplexItem::Type::SUBLABEL;
                        data->index.present = false;
                        data->offset.present = false;
                        data->stage = ComplexData::Stage::BASE;
                        current_operand->data = data;
                    } else if (current_operand->type == OperandType::COMPLEX) {
                        switch (ComplexData* data = static_cast<ComplexData*>(current_operand->data); data->stage) {
                        case ComplexData::Stage::BASE: {
                            if (data->base.present)
                                error("Invalid sublabel location", token);
                            data->base.present = true;
                            data->base.data.sublabel = block;
                            data->base.type = ComplexItem::Type::SUBLABEL;
                            break;
                        }
                        case ComplexData::Stage::INDEX: {
                            if (data->index.present)
                                error("Invalid sublabel location", token);
                            data->index.present = true;
                            data->index.data.sublabel = block;
                            data->index.type = ComplexItem::Type::SUBLABEL;
                            break;
                        }
                        case ComplexData::Stage::OFFSET: {
                            if (data->offset.present)
                                error("Invalid sublabel location", token);
                            data->offset.present = true;
                            data->offset.data.sublabel = block;
                            data->offset.type = ComplexItem::Type::SUBLABEL;
                            current_operand->complete = true;
                            break;
                        }
                        }
                    }
                } else if (token->type == TokenType::OPERATOR) {
                    if (current_operand == nullptr || (current_operand->type != OperandType::COMPLEX && current_operand->type != OperandType::MEMORY))
                        error("Invalid operator location", token);
                    if (current_operand->type == OperandType::MEMORY) {
                        current_operand->type = OperandType::COMPLEX;
                        int64_t* addr = static_cast<int64_t*>(current_operand->data);
                        int64_t imm = *addr;
                        ComplexData* data = new ComplexData;
                        data->base.present = true;
                        if (imm >= INT8_MIN && imm <= INT8_MAX) {
                            data->base.data.imm.size = OperandSize::BYTE;
                            uint8_t* imm8 = new uint8_t;
                            *imm8 = static_cast<uint64_t>(imm) & 0xFF;
                            data->base.data.imm.data = imm8;
                        } else if (imm >= INT16_MIN && imm <= INT16_MAX) {
                            data->base.data.imm.size = OperandSize::WORD;
                            uint16_t* imm16 = new uint16_t;
                            *imm16 = static_cast<uint64_t>(imm) & 0xFFFF;
                            data->base.data.imm.data = imm16;
                        } else if (imm >= INT32_MIN && imm <= INT32_MAX) {
                            data->base.data.imm.size = OperandSize::DWORD;
                            uint32_t* imm32 = new uint32_t;
                            *imm32 = static_cast<uint64_t>(imm) & 0xFFFF'FFFF;
                            data->base.data.imm.data = imm32;
                        } else {
                            data->base.data.imm.size = OperandSize::QWORD;
                            uint64_t* imm64 = new uint64_t;
                            *imm64 = static_cast<uint64_t>(imm);
                            data->base.data.imm.data = imm64;
                        }
                        delete addr;
                        data->base.type = ComplexItem::Type::IMMEDIATE;
                        data->index.present = false;
                        data->offset.present = false;
                        data->stage = ComplexData::Stage::BASE;
                        current_operand->data = data;
                    }
                    ComplexData* data = static_cast<ComplexData*>(current_operand->data);
                    if (size_t name_size = token->data_size; EQUALS(static_cast<const char*>(token->data), "+")) {
                        if (data->stage == ComplexData::Stage::BASE)
                            data->index.present = false;
                        else if (data->stage != ComplexData::Stage::INDEX)
                            error("Invalid operator location", token);
                        data->stage = ComplexData::Stage::OFFSET;
                        data->offset.sign = true;
                    } else if (EQUALS(static_cast<const char*>(token->data), "-")) {
                        if (data->stage == ComplexData::Stage::BASE)
                            data->index.present = false;
                        else if (data->stage != ComplexData::Stage::INDEX)
                            error("Invalid operator location", token);
                        data->stage = ComplexData::Stage::OFFSET;
                        data->offset.sign = false;
                    } else if (EQUALS(static_cast<const char*>(token->data), "*")) {
                        if (data->stage != ComplexData::Stage::BASE) {
                            error("Invalid operator location", token);
                        }
                        data->stage = ComplexData::Stage::INDEX;
                    } else
                        error("Invalid operator", token, true);
                } else
                    error("Invalid Token", token, true);
            }
        } else
            error("Invalid Token", token, true);
        return true;
    });
}

void Parser::Clear() {
    using namespace InsEncoding;
    m_labels.EnumerateReverse([&](Label* label) -> bool {
        if (label == nullptr)
            return false;
        label->blocks.EnumerateReverse([&](Block* block) -> bool {
            if (block == nullptr)
                return false;
            block->data_blocks.EnumerateReverse([&](Data* data) -> bool {
                if (data == nullptr)
                    return false;
                if (data->type) { // instruction
                    Instruction* ins = static_cast<Instruction*>(data->data);
                    if (ins == nullptr)
                        return false;
                    ins->operands.EnumerateReverse([&](Operand* operand) -> bool {
                        if (operand == nullptr)
                            return false;
                        if (operand->type == OperandType::COMPLEX) {
                            ComplexData* complex_data = static_cast<ComplexData*>(operand->data);
                            if (complex_data == nullptr)
                                return false;
                            if (complex_data->base.present) {
                                if (complex_data->base.type == ComplexItem::Type::IMMEDIATE) {
                                    switch (complex_data->base.data.imm.size) {
                                    case OperandSize::BYTE:
                                        delete static_cast<uint8_t*>(complex_data->base.data.imm.data);
                                        break;
                                    case OperandSize::WORD:
                                        delete static_cast<uint16_t*>(complex_data->base.data.imm.data);
                                        break;
                                    case OperandSize::DWORD:
                                        delete static_cast<uint32_t*>(complex_data->base.data.imm.data);
                                        break;
                                    case OperandSize::QWORD:
                                        delete static_cast<uint64_t*>(complex_data->base.data.imm.data);
                                        break;
                                    }
                                } else if (complex_data->base.type == ComplexItem::Type::REGISTER) {
                                    delete complex_data->base.data.reg;
                                }
                                // LABEL and SUBLABEL do not need to be deleted
                            }
                            if (complex_data->index.present) {
                                if (complex_data->index.type == ComplexItem::Type::IMMEDIATE) {
                                    switch (complex_data->index.data.imm.size) {
                                    case OperandSize::BYTE:
                                        delete static_cast<uint8_t*>(complex_data->index.data.imm.data);
                                        break;
                                    case OperandSize::WORD:
                                        delete static_cast<uint16_t*>(complex_data->index.data.imm.data);
                                        break;
                                    case OperandSize::DWORD:
                                        delete static_cast<uint32_t*>(complex_data->index.data.imm.data);
                                        break;
                                    case OperandSize::QWORD:
                                        delete static_cast<uint64_t*>(complex_data->index.data.imm.data);
                                        break;
                                    }
                                } else if (complex_data->index.type == ComplexItem::Type::REGISTER) {
                                    delete complex_data->index.data.reg;
                                }
                                // LABEL and SUBLABEL do not need to be deleted
                            }
                            if (complex_data->offset.present) {
                                if (complex_data->offset.type == ComplexItem::Type::IMMEDIATE) {
                                    switch (complex_data->offset.data.imm.size) {
                                    case OperandSize::BYTE:
                                        delete static_cast<uint8_t*>(complex_data->offset.data.imm.data);
                                        break;
                                    case OperandSize::WORD:
                                        delete static_cast<uint16_t*>(complex_data->offset.data.imm.data);
                                        break;
                                    case OperandSize::DWORD:
                                        delete static_cast<uint32_t*>(complex_data->offset.data.imm.data);
                                        break;
                                    case OperandSize::QWORD:
                                        delete static_cast<uint64_t*>(complex_data->offset.data.imm.data);
                                        break;
                                    }
                                } else if (complex_data->offset.type == ComplexItem::Type::REGISTER) {
                                    delete complex_data->offset.data.reg;
                                }
                                // LABEL and SUBLABEL do not need to be deleted
                            }
                            delete complex_data;
                        } else if (operand->type == OperandType::MEMORY) {
                            delete static_cast<uint64_t*>(operand->data);
                        } else if (operand->type == OperandType::IMMEDIATE) {
                            switch (operand->size) {
                            case OperandSize::BYTE:
                                delete static_cast<uint8_t*>(operand->data);
                                break;
                            case OperandSize::WORD:
                                delete static_cast<uint16_t*>(operand->data);
                                break;
                            case OperandSize::DWORD:
                                delete static_cast<uint32_t*>(operand->data);
                                break;
                            case OperandSize::QWORD:
                                delete static_cast<uint64_t*>(operand->data);
                                break;
                            }
                        } else if (operand->type == OperandType::REGISTER) {
                            delete static_cast<Register*>(operand->data);
                        }
                        delete operand;
                        return true;
                    });
                    ins->operands.clear();
                    delete ins;
                } else {
                    RawData* raw_data = static_cast<RawData*>(data->data);
                    if (raw_data == nullptr)
                        return false;
                    if (raw_data->type == RawDataType::RAW) {
                        switch (raw_data->data_size) {
                        case 1:
                            delete static_cast<uint8_t*>(raw_data->data);
                            break;
                        case 2:
                            delete static_cast<uint16_t*>(raw_data->data);
                            break;
                        case 4:
                            delete static_cast<uint32_t*>(raw_data->data);
                            break;
                        case 8:
                            delete static_cast<uint64_t*>(raw_data->data);
                            break;
                        default:
                            delete[] static_cast<uint8_t*>(raw_data->data);
                            break;
                        }
                    }
                    delete raw_data;
                }
                delete data;
                return true;
            });
            block->data_blocks.clear();
            block->jumps_to_here.EnumerateReverse([&](const uint64_t* jump) -> bool {
                if (jump == nullptr)
                    return false;
                delete jump;
                return true;
            });
            block->jumps_to_here.clear();
            delete block;
            return true;
        });
        label->blocks.clear();
        delete label;
        return true;
    });
    m_labels.clear();
}

void Parser::PrintSections(FILE* fd) const {
    using namespace InsEncoding;
    char* name = nullptr;
    for (uint64_t i = 0; i < m_labels.getCount(); i++) {
        Label* label = m_labels.get(i);
        if (label == nullptr)
            return;
        name = new char[label->name_size + 1];
        strncpy(name, label->name, label->name_size);
        name[label->name_size] = 0;
        fprintf(fd, "Label: \"%s\":\n", name);
        delete[] name;
        for (uint64_t j = 0; j < label->blocks.getCount(); j++) {
            Block* block = label->blocks.get(j);
            if (block == nullptr)
                return;
            name = new char[block->name_size + 1];
            strncpy(name, block->name, block->name_size);
            name[block->name_size] = 0;
            fprintf(fd, "Block: \"%s\":\n", name);
            delete[] name;
            for (uint64_t k = 0; k < block->data_blocks.getCount(); k++) {
                Data* data = block->data_blocks.get(k);
                if (data == nullptr)
                    return;
                if (data->type) { // instruction
                    Instruction* ins = static_cast<Instruction*>(data->data);
                    if (ins == nullptr)
                        return;
                    fprintf(fd, "Instruction: \"%s\":\n", GetInstructionName(ins->GetOpcode()));
                    for (uint64_t l = 0; l < ins->operands.getCount(); l++) {
                        Operand* operand = ins->operands.get(l);
                        if (operand == nullptr)
                            return;
                        char const* operand_size = nullptr;
                        switch (operand->size) {
                        case OperandSize::BYTE:
                            operand_size = "byte";
                            break;
                        case OperandSize::WORD:
                            operand_size = "word";
                            break;
                        case OperandSize::DWORD:
                            operand_size = "dword";
                            break;
                        case OperandSize::QWORD:
                            operand_size = "qword";
                            break;
                        }
                        fprintf(fd, "Operand: size = %s, type = %d, ", operand_size, static_cast<int>(operand->type));
                        switch (operand->type) {
                        case OperandType::REGISTER:
                            fprintf(fd, "Register: \"%s\"\n", GetRegisterName(*static_cast<Register*>(operand->data)));
                            break;
                        case OperandType::MEMORY:
                            fprintf(fd, "Memory address: %#18lx\n", *static_cast<uint64_t*>(operand->data));
                            break;
                        case OperandType::COMPLEX: {
                            ComplexData* complex_data = static_cast<ComplexData*>(operand->data);
                            if (complex_data == nullptr)
                                return;
                            fprintf(fd, "Complex data:\n");
                            if (complex_data->base.present) {
                                fprintf(fd, "Base: ");
                                switch (complex_data->base.type) {
                                case ComplexItem::Type::IMMEDIATE:
                                    switch (complex_data->base.data.imm.size) {
                                    case OperandSize::BYTE:
                                        fprintf(fd, "size = 1, immediate = %#4hhx\n", *static_cast<uint8_t*>(complex_data->base.data.imm.data));
                                        break;
                                    case OperandSize::WORD:
                                        fprintf(fd, "size = 2, immediate = %#6hx\n", *static_cast<uint16_t*>(complex_data->base.data.imm.data));
                                        break;
                                    case OperandSize::DWORD:
                                        fprintf(fd, "size = 4, immediate = %#10x\n", *static_cast<uint32_t*>(complex_data->base.data.imm.data));
                                        break;
                                    case OperandSize::QWORD:
                                        fprintf(fd, "size = 8, immediate = %#18lx\n", *static_cast<uint64_t*>(complex_data->base.data.imm.data));
                                        break;
                                    }
                                    break;
                                case ComplexItem::Type::REGISTER:
                                    fprintf(fd, "Register: \"%s\"\n", GetRegisterName(*complex_data->base.data.reg));
                                    break;
                                case ComplexItem::Type::LABEL: {
                                    size_t size = complex_data->base.data.label->name_size;
                                    char* i_name = new char[size + 1];
                                    strncpy(i_name, complex_data->base.data.label->name, size);
                                    i_name[size] = 0;
                                    fprintf(fd, "Label: \"%s\"\n", i_name);
                                    delete[] i_name;
                                    break;
                                }
                                case ComplexItem::Type::SUBLABEL: {
                                    size_t size = complex_data->base.data.sublabel->name_size;
                                    char* i_name = new char[size + 1];
                                    strncpy(i_name, complex_data->base.data.sublabel->name, size);
                                    i_name[size] = 0;
                                    fprintf(fd, "Sublabel: \"%s\"\n", i_name);
                                    delete[] i_name;
                                    break;
                                }
                                case ComplexItem::Type::UNKNOWN:
                                    break;
                                }
                            }
                            if (complex_data->index.present) {
                                fprintf(fd, "Index: ");
                                switch (complex_data->index.type) {
                                case ComplexItem::Type::IMMEDIATE:
                                    switch (complex_data->index.data.imm.size) {
                                    case OperandSize::BYTE:
                                        fprintf(fd, "size = 1, immediate = %#4hhx\n", *static_cast<uint8_t*>(complex_data->index.data.imm.data));
                                        break;
                                    case OperandSize::WORD:
                                        fprintf(fd, "size = 2, immediate = %#6hx\n", *static_cast<uint16_t*>(complex_data->index.data.imm.data));
                                        break;
                                    case OperandSize::DWORD:
                                        fprintf(fd, "size = 4, immediate = %#10x\n", *static_cast<uint32_t*>(complex_data->index.data.imm.data));
                                        break;
                                    case OperandSize::QWORD:
                                        fprintf(fd, "size = 8, immediate = %#18lx\n", *static_cast<uint64_t*>(complex_data->index.data.imm.data));
                                        break;
                                    }
                                    break;
                                case ComplexItem::Type::REGISTER:
                                    fprintf(fd, "Register: \"%s\"\n", GetRegisterName(*complex_data->index.data.reg));
                                    break;
                                case ComplexItem::Type::LABEL: {
                                    size_t size = complex_data->index.data.label->name_size;
                                    char* i_name = new char[size + 1];
                                    strncpy(i_name, complex_data->index.data.label->name, size);
                                    i_name[size] = 0;
                                    fprintf(fd, "Label: \"%s\"\n", i_name);
                                    delete[] i_name;
                                    break;
                                }
                                case ComplexItem::Type::SUBLABEL: {
                                    size_t size = complex_data->index.data.sublabel->name_size;
                                    char* i_name = new char[size + 1];
                                    strncpy(i_name, complex_data->index.data.sublabel->name, size);
                                    i_name[size] = 0;
                                    fprintf(fd, "Sublabel: \"%s\"\n", i_name);
                                    delete[] i_name;
                                    break;
                                }
                                case ComplexItem::Type::UNKNOWN:
                                    break;
                                }
                            }
                            if (complex_data->offset.present) {
                                fprintf(fd, "Offset: ");
                                switch (complex_data->offset.type) {
                                case ComplexItem::Type::IMMEDIATE:
                                    switch (complex_data->offset.data.imm.size) {
                                    case OperandSize::BYTE:
                                        fprintf(fd, "size = 1, immediate = %#4hhx\n", *static_cast<uint8_t*>(complex_data->offset.data.imm.data));
                                        break;
                                    case OperandSize::WORD:
                                        fprintf(fd, "size = 2, immediate = %#6hx\n", *static_cast<uint16_t*>(complex_data->offset.data.imm.data));
                                        break;
                                    case OperandSize::DWORD:
                                        fprintf(fd, "size = 4, immediate = %#10x\n", *static_cast<uint32_t*>(complex_data->offset.data.imm.data));
                                        break;
                                    case OperandSize::QWORD:
                                        fprintf(fd, "size = 8, immediate = %#18lx\n", *static_cast<uint64_t*>(complex_data->offset.data.imm.data));
                                        break;
                                    }
                                    break;
                                case ComplexItem::Type::REGISTER:
                                    fprintf(fd, "Register: \"%s\", sign = %s\n", GetRegisterName(*complex_data->offset.data.reg), complex_data->offset.sign ? "positive" : "negative");
                                    break;
                                case ComplexItem::Type::LABEL: {
                                    size_t size = complex_data->offset.data.label->name_size;
                                    char* i_name = new char[size + 1];
                                    strncpy(i_name, complex_data->offset.data.label->name, size);
                                    i_name[size] = 0;
                                    fprintf(fd, "Label: \"%s\"\n", i_name);
                                    delete[] i_name;
                                    break;
                                }
                                case ComplexItem::Type::SUBLABEL: {
                                    size_t size = complex_data->offset.data.sublabel->name_size;
                                    char* i_name = new char[size + 1];
                                    strncpy(i_name, complex_data->offset.data.sublabel->name, size);
                                    i_name[size] = 0;
                                    fprintf(fd, "Sublabel: \"%s\"\n", i_name);
                                    delete[] i_name;
                                    break;
                                }
                                case ComplexItem::Type::UNKNOWN:
                                    break;
                                }
                            }
                            break;
                        }
                        case OperandType::IMMEDIATE:
                            switch (operand->size) {
                            case OperandSize::BYTE:
                                fprintf(fd, "size = 1, immediate = %#4hhx\n", *static_cast<uint8_t*>(operand->data));
                                break;
                            case OperandSize::WORD:
                                fprintf(fd, "size = 2, immediate = %#6hx\n", *static_cast<uint16_t*>(operand->data));
                                break;
                            case OperandSize::DWORD:
                                fprintf(fd, "size = 4, immediate = %#10x\n", *static_cast<uint32_t*>(operand->data));
                                break;
                            case OperandSize::QWORD:
                                fprintf(fd, "size = 8, immediate = %#18lx\n", *static_cast<uint64_t*>(operand->data));
                                break;
                            }
                            break;
                        case OperandType::LABEL: {
                            size_t size = static_cast<Label*>(operand->data)->name_size;
                            char* i_name = new char[size + 1];
                            strncpy(i_name, static_cast<Label*>(operand->data)->name, size);
                            i_name[size] = 0;
                            fprintf(fd, "Label: \"%s\"\n", i_name);
                            delete[] i_name;
                            break;
                        }
                        case OperandType::SUBLABEL: {
                            size_t size = static_cast<Block*>(operand->data)->name_size;
                            char* i_name = new char[size + 1];
                            strncpy(i_name, static_cast<Block*>(operand->data)->name, size);
                            i_name[size] = 0;
                            fprintf(fd, "Sublabel: \"%s\"\n", i_name);
                            delete[] i_name;
                            break;
                        }
                        default:
                            fputs("unknown type\n", fd);
                        }
                    }
                } else { // raw data
                    RawData* raw_data = static_cast<RawData*>(data->data);
                    if (raw_data == nullptr || raw_data->data == nullptr)
                        return;
                    fputs("Raw data: ", fd);
                    switch (raw_data->type) {
                    case RawDataType::RAW:
                        fprintf(fd, "size = %lu:\n", raw_data->data_size);
                        for (uint64_t l = 0; l < raw_data->data_size; l++)
                            fprintf(fd, "%#2hhx%c", static_cast<uint8_t*>(raw_data->data)[l], (l % 8) == 7 ? '\n' : ' ');
                        break;
                    case RawDataType::LABEL:
                        fprintf(fd, "Label: \"%s\"\n", static_cast<Label*>(raw_data->data)->name);
                        break;
                    case RawDataType::SUBLABEL:
                        fprintf(fd, "Sublabel: \"%s\"\n", static_cast<Block*>(raw_data->data)->name);
                        break;
                    case RawDataType::ASCII:
                        fprintf(fd, "ASCII: \"%s\"\n", static_cast<char*>(raw_data->data));
                        break;
                    case RawDataType::ASCIIZ:
                        fprintf(fd, "ASCIIZ: \"%s\"\n", static_cast<char*>(raw_data->data));
                        break;
                    case RawDataType::ALIGNMENT:
                        fprintf(fd, "Alignment: %lu\n", *static_cast<uint64_t*>(raw_data->data));
                        break;
                    }
                    fputc('\n', fd);
                }
            }
        }
    }
}

const LinkedList::RearInsertLinkedList<InsEncoding::Label>& Parser::GetLabels() const {
    return m_labels;
}

InsEncoding::Opcode Parser::GetOpcode(const char* name, size_t name_size) {
    using namespace InsEncoding;
    if (!m_opcodeTableInitialised) {
#define INSERT_OPCODE(str, opcode, length) m_opcodes.insert({std::string_view(#str, length), Opcode::opcode})
        INSERT_OPCODE(add, ADD, 3);
        INSERT_OPCODE(mul, MUL, 3);
        INSERT_OPCODE(sub, SUB, 3);
        INSERT_OPCODE(div, DIV, 3);
        INSERT_OPCODE(or, OR, 2);
        INSERT_OPCODE(xor, XOR, 3);
        INSERT_OPCODE(nor, NOR, 3);
        INSERT_OPCODE(and, AND, 3);
        INSERT_OPCODE(nand, NAND, 4);
        INSERT_OPCODE(not, NOT, 3);
        INSERT_OPCODE(cmp, CMP, 3);
        INSERT_OPCODE(inc, INC, 3);
        INSERT_OPCODE(dec, DEC, 3);
        INSERT_OPCODE(shl, SHL, 3);
        INSERT_OPCODE(shr, SHR, 3);
        INSERT_OPCODE(ret, RET, 3);
        INSERT_OPCODE(call, CALL, 4);
        INSERT_OPCODE(jmp, JMP, 3);
        INSERT_OPCODE(jc, JC, 2);
        INSERT_OPCODE(jnc, JNC, 3);
        INSERT_OPCODE(jz, JZ, 2);
        INSERT_OPCODE(jnz, JNZ, 3);
        INSERT_OPCODE(jl, JL, 2);
        INSERT_OPCODE(jnge, JL, 4);
        INSERT_OPCODE(jle, JLE, 3);
        INSERT_OPCODE(jng, JLE, 3);
        INSERT_OPCODE(jnl, JNL, 3);
        INSERT_OPCODE(jge, JNL, 3);
        INSERT_OPCODE(jnle, JNLE, 4);
        INSERT_OPCODE(jg, JNLE, 2);
        INSERT_OPCODE(mov, MOV, 3);
        INSERT_OPCODE(nop, NOP, 3);
        INSERT_OPCODE(hlt, HLT, 3);
        INSERT_OPCODE(push, PUSH, 4);
        INSERT_OPCODE(pop, POP, 3);
        INSERT_OPCODE(pusha, PUSHA, 5);
        INSERT_OPCODE(popa, POPA, 4);
        INSERT_OPCODE(int, INT, 3);
        INSERT_OPCODE(lidt, LIDT, 4);
        INSERT_OPCODE(iret, IRET, 4);
        INSERT_OPCODE(syscall, SYSCALL, 7);
        INSERT_OPCODE(sysret, SYSRET, 6);
        INSERT_OPCODE(enteruser, ENTERUSER, 9);
#undef INSERT_OPCODE
        m_opcodeTableInitialised = true;
    }
    return m_opcodes[std::string_view(name, name_size)];
}

InsEncoding::Register Parser::GetRegister(const char* name, size_t name_size) {
    using namespace InsEncoding;
    if (!m_registerTableInitialised) {
#define INSERT_REGISTER(name, length) m_registers.insert({std::string_view(#name, length), Register::name})
        INSERT_REGISTER(r0, 2);
        INSERT_REGISTER(r1, 2);
        INSERT_REGISTER(r2, 2);
        INSERT_REGISTER(r3, 2);
        INSERT_REGISTER(r4, 2);
        INSERT_REGISTER(r5, 2);
        INSERT_REGISTER(r6, 2);
        INSERT_REGISTER(r7, 2);
        INSERT_REGISTER(r8, 2);
        INSERT_REGISTER(r9, 2);
        INSERT_REGISTER(r10, 3);
        INSERT_REGISTER(r11, 3);
        INSERT_REGISTER(r12, 3);
        INSERT_REGISTER(r13, 3);
        INSERT_REGISTER(r14, 3);
        INSERT_REGISTER(r15, 3);
        INSERT_REGISTER(scp, 3);
        INSERT_REGISTER(sbp, 3);
        INSERT_REGISTER(stp, 3);
        INSERT_REGISTER(cr0, 3);
        INSERT_REGISTER(cr1, 3);
        INSERT_REGISTER(cr2, 3);
        INSERT_REGISTER(cr3, 3);
        INSERT_REGISTER(cr4, 3);
        INSERT_REGISTER(cr5, 3);
        INSERT_REGISTER(cr6, 3);
        INSERT_REGISTER(cr7, 3);
        INSERT_REGISTER(sts, 3);
        INSERT_REGISTER(ip, 2);
#undef INSERT_REGISTER
        m_registerTableInitialised = true;
    }
    return m_registers[std::string_view(name, name_size)];
}

#undef EQUALS

void Parser::error(const char* message, Token* token, bool print_token) {
    printf("Parser error at %s:%zu: %s", token->file_name.c_str(), token->line, message);
    if (print_token)
        printf(": \"%.*s\"", static_cast<int>(token->data_size), static_cast<const char*>(token->data));
    putchar('\n');
    exit(1);
}

const char* Parser::GetInstructionName(InsEncoding::Opcode opcode) {
    using namespace InsEncoding;
#define NAME_CASE(ins) \
    case Opcode::ins:  \
        return #ins;
    switch (opcode) {
        NAME_CASE(PUSH)
        NAME_CASE(POP)
        NAME_CASE(PUSHA)
        NAME_CASE(POPA)
        NAME_CASE(ADD)
        NAME_CASE(MUL)
        NAME_CASE(SUB)
        NAME_CASE(DIV)
        NAME_CASE(OR)
        NAME_CASE(XOR)
        NAME_CASE(NOR)
        NAME_CASE(AND)
        NAME_CASE(NAND)
        NAME_CASE(NOT)
        NAME_CASE(CMP)
        NAME_CASE(INC)
        NAME_CASE(DEC)
        NAME_CASE(SHL)
        NAME_CASE(SHR)
        NAME_CASE(RET)
        NAME_CASE(CALL)
        NAME_CASE(JMP)
        NAME_CASE(JC)
        NAME_CASE(JNC)
        NAME_CASE(JZ)
        NAME_CASE(JNZ)
        NAME_CASE(JL)
        NAME_CASE(JLE)
        NAME_CASE(JNL)
        NAME_CASE(JNLE)
        NAME_CASE(INT)
        NAME_CASE(LIDT)
        NAME_CASE(IRET)
        NAME_CASE(MOV)
        NAME_CASE(NOP)
        NAME_CASE(HLT)
        NAME_CASE(SYSCALL)
        NAME_CASE(SYSRET)
        NAME_CASE(ENTERUSER)
        NAME_CASE(UNKNOWN)
    }
#undef NAME_CASE
    return "UNKNOWN";
}

const char* Parser::GetRegisterName(InsEncoding::Register reg) {
    using namespace InsEncoding;
#define NAME_CASE(reg)  \
    case Register::reg: \
        return #reg;
    switch (reg) {
        NAME_CASE(r0)
        NAME_CASE(r1)
        NAME_CASE(r2)
        NAME_CASE(r3)
        NAME_CASE(r4)
        NAME_CASE(r5)
        NAME_CASE(r6)
        NAME_CASE(r7)
        NAME_CASE(r8)
        NAME_CASE(r9)
        NAME_CASE(r10)
        NAME_CASE(r11)
        NAME_CASE(r12)
        NAME_CASE(r13)
        NAME_CASE(r14)
        NAME_CASE(r15)
        NAME_CASE(scp)
        NAME_CASE(sbp)
        NAME_CASE(stp)
        NAME_CASE(cr0)
        NAME_CASE(cr1)
        NAME_CASE(cr2)
        NAME_CASE(cr3)
        NAME_CASE(cr4)
        NAME_CASE(cr5)
        NAME_CASE(cr6)
        NAME_CASE(cr7)
        NAME_CASE(sts)
        NAME_CASE(ip)
        NAME_CASE(unknown)
    }
#undef NAME_CASE
    return "unknown";
}