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

#include "Parser.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <LibArch/Instruction.hpp>
#include <LibArch/Operand.hpp>

Parser::Parser()
    : m_baseAddress(0), m_opcodeTableInitialised(false), m_registerTableInitialised(false) {
}

Parser::~Parser() {
}

#define EQUALS(str1, str2) (strlen(str2) == nameSize && strncmp(str1, str2, nameSize) == 0)

void Parser::parse(const LinkedList::RearInsertLinkedList<Token>& tokens) {
    using namespace InsEncoding;
    Label* currentLabel = nullptr;
    Block* currentBlock = nullptr;
    Data* currentData = nullptr;
    Operand* currentOperand = nullptr;
    bool inDirective = false;
    bool inInstruction = true;
    bool inOperand = false;

    m_baseAddress = 0;
    bool baseAddressSet = false;
    bool baseAddressParsed = false;

    Label* label = new Label;
    label->name = const_cast<char*>("");
    label->nameSize = 0;
    m_labels.insert(label);
    Block* block = new Block;
    block->name = const_cast<char*>("");
    block->nameSize = 0;
    label->blocks.insert(block);

    // First scan for labels
    tokens.Enumerate([&](Token* token) -> void {
        if (token->type == TokenType::BLABEL) {
            Label* label = new Label;
            label->name = static_cast<char*>(token->data);
            label->nameSize = token->dataSize - sizeof(char); // remove the colon at the end
            m_labels.insert(label);
            currentLabel = label;

            Block* block = new Block;
            block->name = const_cast<char*>("");
            block->nameSize = 0;
            currentLabel->blocks.insert(block);
            currentBlock = block;
            inInstruction = false;
        } else if (token->type == TokenType::BSUBLABEL) {
            Block* block = new Block;
            block->name = reinterpret_cast<char*>(reinterpret_cast<uint64_t>(token->data) + sizeof(char)); // remove the dot at the start
            block->nameSize = token->dataSize - 2 * sizeof(char); // remove the dot at the start and colon at the end
            currentLabel->blocks.insert(block);
            currentBlock = block;
            inInstruction = false;
        }
    });

    currentLabel = label;
    currentBlock = block;

    tokens.Enumerate([&](Token* token, uint64_t index) -> bool {
#ifdef ASSEMBLER_DEBUG
        printf("Token: \"%.*s\", index = %lu, type = %lu\n", static_cast<int>(token->dataSize), static_cast<char*>(token->data), index, static_cast<unsigned long int>(token->type));
#else
        (void)index;
#endif

        if (inDirective) {
            if (token->type == TokenType::NUMBER && baseAddressParsed && !baseAddressSet) {
                m_baseAddress = strtoull(static_cast<const char*>(token->data), nullptr, 0);
                baseAddressSet = true;
                inDirective = false;
                return true;
            }
            if (static_cast<RawData*>(currentData->data)->type == RawDataType::ASCII || static_cast<RawData*>(currentData->data)->type == RawDataType::ASCIIZ) {
                if (token->type == TokenType::STRING) {
                    RawData* rawData = static_cast<RawData*>(currentData->data);
                    std::string outStr;
                    // need to loop through the string and resolve escape sequences
                    for (uint64_t i = 1; i < (token->dataSize - 1) /* remove the start and end quotes */; i++) {
                        if (static_cast<const char*>(token->data)[i] == '\\') {
                            i++;
                            switch (static_cast<const char*>(token->data)[i]) {
                            case 'n':
                                outStr += '\n';
                                break;
                            case 't':
                                outStr += '\t';
                                break;
                            case 'r':
                                outStr += '\r';
                                break;
                            case '0':
                                outStr += '\0';
                                break;
                            case '\\':
                                outStr += '\\';
                                break;
                            case '\'':
                                outStr += '\'';
                                break;
                            case '\"':
                                outStr += '\"';
                                break;
                            case 'x': {
                                // next 2 chars are hex
                                uint8_t hex = 0;
                                for (uint8_t j = 0; j < 2; j++) {
                                    hex *= 16;
                                    i++;
                                    if (i + 1 >= token->dataSize)
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
                                outStr += static_cast<char>(hex);
                                break;
                            }
                            default:
                                error("Invalid escape sequence", token);
                            }
                        } else
                            outStr += static_cast<const char*>(token->data)[i];
                    }
                    uint64_t strLen = outStr.size();
                    rawData->dataSize = strLen + (rawData->type == RawDataType::ASCIIZ ? 1 : 0);
                    rawData->data = new char[rawData->dataSize];
                    memcpy(rawData->data, outStr.c_str(), strLen);
                    if (rawData->type == RawDataType::ASCIIZ)
                        static_cast<char*>(rawData->data)[strLen] = 0;
                } else
                    error("Invalid token after directive", token, true);
                inDirective = false;
                return true;
            }
            switch (token->type) {
            case TokenType::NUMBER:
                switch (static_cast<RawData*>(currentData->data)->dataSize) {
                case 1: { // byte
                    uint8_t* data = new uint8_t;
                    *data = strtol(static_cast<const char*>(token->data), nullptr, 0) & 0xFF;
                    static_cast<RawData*>(currentData->data)->data = data;
                    break;
                }
                case 2: { // word
                    uint16_t* data = new uint16_t;
                    *data = strtol(static_cast<const char*>(token->data), nullptr, 0) & 0xFFFF;
                    static_cast<RawData*>(currentData->data)->data = data;
                    break;
                }
                case 4: { // dword
                    uint32_t* data = new uint32_t;
                    *data = strtoul(static_cast<const char*>(token->data), nullptr, 0) & 0xFFFF'FFFF;
                    static_cast<RawData*>(currentData->data)->data = data;
                    break;
                }
                case 8: { // qword
                    uint64_t* data = new uint64_t;
                    *data = strtoull(static_cast<const char*>(token->data), nullptr, 0);
                    static_cast<RawData*>(currentData->data)->data = data;
                    break;
                }
                default:
                    error("Invalid data size for directive", token);
                    break;
                }
                if (static_cast<RawData*>(currentData->data)->type != RawDataType::ALIGNMENT)
                    static_cast<RawData*>(currentData->data)->type = RawDataType::RAW;
                break;
            case TokenType::LABEL: {
                if (currentOperand != nullptr)
                    error("Invalid label location", token);
                Label* label = nullptr;
                // find the label
                char* name = new char[token->dataSize + 1];
                strncpy(name, static_cast<const char*>(token->data), token->dataSize);
                name[token->dataSize] = 0;
                m_labels.Enumerate([&](Label* i_label) -> bool {
                    if (i_label->nameSize < token->dataSize) // strncmp can only properly handle strings of equal or greater length.
                        return true;
                    if (strncmp(i_label->name, name, i_label->nameSize) == 0) {
                        label = i_label;
                        return false;
                    }
                    return true;
                });
                delete[] name;
                if (label == nullptr)
                    error("Invalid label", token, true);
                static_cast<RawData*>(currentData->data)->type = RawDataType::LABEL;
                static_cast<RawData*>(currentData->data)->data = label;
                break;
            }
            case TokenType::SUBLABEL: {
                if (currentOperand != nullptr)
                    error("Invalid sublabel location", token);
                Block* block = nullptr;
                // find the block
                char* name = new char[token->dataSize + 1];
                strncpy(name, static_cast<const char*>(token->data), token->dataSize);
                name[token->dataSize] = 0;
                if (name[0] == '.')
                    name = &(name[1]);
                else {
                    delete[] name;
                    error("Invalid sublabel name", token, true);
                }
                currentLabel->blocks.Enumerate([&](Block* i_block) -> bool {
                    if (i_block->nameSize < (token->dataSize - 1)) // strncmp can only properly handle strings of equal or greater length
                        return true;
                    if (strncmp(i_block->name, name, i_block->nameSize) == 0) {
                        block = i_block;
                        return false;
                    }
                    return true;
                });
                delete[] reinterpret_cast<char*>(reinterpret_cast<uint64_t>(name) - sizeof(char));
                if (block == nullptr)
                    error("Invalid sublabel", token, true);
                static_cast<RawData*>(currentData->data)->type = RawDataType::SUBLABEL;
                static_cast<RawData*>(currentData->data)->data = block;
                break;
            }
            default:
                error("Invalid token after directive", token);
                break;
            }
            inDirective = false;
        } else if (token->type == TokenType::COMMA) {
            if (!inInstruction)
                error("Comma (',') outside of instruction.", token);
            inOperand = true;
        } else if (token->type == TokenType::DIRECTIVE) {
            if (inOperand)
                error("Directive inside operand", token);
            if (strncmp(static_cast<char*>(token->data), "org", token->dataSize) == 0) {
                if (baseAddressSet)
                    error("Multiple base addresses", token);
                baseAddressSet = false;
                baseAddressParsed = true;
                inDirective = true;
                inInstruction = false;
                return true;
            }
            Data* data = new Data;
            RawData* rawData = new RawData;
            data->data = rawData;
            data->type = false;
            rawData->fileName = token->fileName;
            rawData->line = token->line;
            if (strncmp(static_cast<char*>(token->data), "db", token->dataSize) == 0)
                static_cast<RawData*>(data->data)->dataSize = 1;
            else if (strncmp(static_cast<char*>(token->data), "dw", token->dataSize) == 0)
                static_cast<RawData*>(data->data)->dataSize = 2;
            else if (strncmp(static_cast<char*>(token->data), "dd", token->dataSize) == 0)
                static_cast<RawData*>(data->data)->dataSize = 4;
            else if (strncmp(static_cast<char*>(token->data), "dq", token->dataSize) == 0)
                static_cast<RawData*>(data->data)->dataSize = 8;
            else if (strncmp(static_cast<char*>(token->data), "align", token->dataSize) == 0) {
                static_cast<RawData*>(data->data)->dataSize = 8;
                static_cast<RawData*>(data->data)->type = RawDataType::ALIGNMENT;
            }
            else if (strncmp(static_cast<char*>(token->data), "ascii", token->dataSize) == 0)
                static_cast<RawData*>(data->data)->type = RawDataType::ASCII;
            else if (strncmp(static_cast<char*>(token->data), "asciiz", token->dataSize) == 0)
                static_cast<RawData*>(data->data)->type = RawDataType::ASCIIZ;
            else
                error("Invalid directive", token);
            static_cast<RawData*>(data->data)->data = nullptr;
            currentBlock->dataBlocks.insert(data);
            currentData = data;
            inDirective = true;
            inInstruction = false;
        } else if (token->type == TokenType::BLABEL) {
            if (inInstruction && inOperand)
                error("Label inside operand", token);
            // find the label
            Label* label = nullptr;
            char* name = new char[token->dataSize + 1];
            strncpy(name, static_cast<const char*>(token->data), token->dataSize);
            name[token->dataSize] = 0;
            m_labels.Enumerate([&](Label* i_label) -> bool {
                if (i_label->nameSize < (token->dataSize - 1)) // strncmp can only properly handle strings of equal or greater length. -1 to remove the colon
                    return true;
                if (strncmp(i_label->name, name, i_label->nameSize) == 0) {
                    label = i_label;
                    return false;
                }
                return true;
            });
            delete[] name;
            currentLabel = label;
            if (currentLabel == nullptr)
                error("Invalid label", token, true);
            currentBlock = currentLabel->blocks.get(0);
            inInstruction = false;
        } else if (token->type == TokenType::BSUBLABEL) {
            if (inInstruction && inOperand)
                error("Sublabel inside operand", token);
            if (currentLabel == nullptr)
                error("Invalid sublabel", token);
            // find the block
            Block* block = nullptr;
            char* name = new char[token->dataSize + 1];
            strncpy(name, static_cast<const char*>(token->data), token->dataSize);
            name[token->dataSize] = 0;
            if (name[0] == '.')
                name = &(name[1]);
            else {
                delete[] name;
                error("Invalid sublabel name", token, true);
            }
            currentLabel->blocks.Enumerate([&](Block* i_block) -> bool {
                if (i_block->nameSize < (token->dataSize - 2)) // strncmp can only properly handle strings of equal or greater length
                    return true;
                if (strncmp(i_block->name, name, i_block->nameSize) == 0) {
                    block = i_block;
                    return false;
                }
                return true;
            });
            delete[] reinterpret_cast<char*>(reinterpret_cast<uint64_t>(name) - sizeof(char));
            currentBlock = block;
            if (currentBlock == nullptr)
                error("Invalid sublabel", token, true);
            inInstruction = false;
        } else if (token->type == TokenType::INSTRUCTION) {
            if (inOperand)
                error("Instruction inside operand", token);
            Data* data = new Data;
            Instruction* instruction = new Instruction(GetOpcode(static_cast<const char*>(token->data), token->dataSize), token->fileName, token->line);
            data->type = true;
            data->data = instruction;
            currentBlock->dataBlocks.insert(data);
            currentData = data;
            if (uint64_t nameSize = token->dataSize; EQUALS(static_cast<const char*>(token->data), "ret") || EQUALS(static_cast<const char*>(token->data), "nop") || EQUALS(static_cast<const char*>(token->data), "hlt") || EQUALS(static_cast<const char*>(token->data), "pusha") || EQUALS(static_cast<const char*>(token->data), "popa") || EQUALS(static_cast<const char*>(token->data), "iret") || EQUALS(static_cast<const char*>(token->data), "syscall") || EQUALS(static_cast<const char*>(token->data), "sysret")) {
                inInstruction = false;
                inOperand = false;
            } else {
                inInstruction = true;
                inOperand = true;
            }
        } else if (inInstruction) {
            if (inOperand) {
                if (token->type == TokenType::SIZE) {
                    if (currentOperand != nullptr)
                        error("Invalid size location", token);
                    Operand* operand = new Operand;
                    operand->complete = false;
                    operand->type = OperandType::UNKNOWN;
                    static_cast<Instruction*>(currentData->data)->operands.insert(operand);
                    currentOperand = operand;
                    if (size_t nameSize = token->dataSize; EQUALS(static_cast<const char*>(token->data), "byte"))
                        currentOperand->size = OperandSize::BYTE;
                    else if (EQUALS(static_cast<const char*>(token->data), "word"))
                        currentOperand->size = OperandSize::WORD;
                    else if (EQUALS(static_cast<const char*>(token->data), "dword"))
                        currentOperand->size = OperandSize::DWORD;
                    else if (EQUALS(static_cast<const char*>(token->data), "qword"))
                        currentOperand->size = OperandSize::QWORD;
                    else
                        error("Invalid size", token);
                } else if (token->type == TokenType::LBRACKET) {
                    if (currentOperand == nullptr) {
                        Operand* operand = new Operand;
                        operand->size = OperandSize::QWORD;
                        operand->complete = false;
                        static_cast<Instruction*>(currentData->data)->operands.insert(operand);
                        currentOperand = operand;
                    }
                    currentOperand->type = OperandType::POTENTIAL_MEMORY;
                } else if (token->type == TokenType::RBRACKET) {
                    if (currentOperand == nullptr || !(currentOperand->type == OperandType::COMPLEX || currentOperand->type == OperandType::MEMORY))
                        error("Invalid operand", token);
                    if (!(currentOperand->complete) && currentOperand->type != OperandType::COMPLEX)
                        error("Invalid operand", token);
                    currentOperand = nullptr;
                    inOperand = false;
                } else if (token->type == TokenType::NUMBER) {
                    if (currentOperand == nullptr) { // must be immediate
                        Operand* operand = new Operand;
                        operand->complete = false;
                        static_cast<Instruction*>(currentData->data)->operands.insert(operand);
                        currentOperand = operand;
                        currentOperand->type = OperandType::IMMEDIATE;
                        if (long imm = static_cast<long>(strtoull(static_cast<const char*>(token->data), nullptr, 0)); imm >= INT8_MIN && imm <= INT8_MAX) {
                            currentOperand->size = OperandSize::BYTE;
                            uint8_t* imm8 = new uint8_t;
                            *imm8 = static_cast<uint64_t>(imm) & 0xFF;
                            currentOperand->data = imm8;
                        } else if (imm >= INT16_MIN && imm <= INT16_MAX) {
                            currentOperand->size = OperandSize::WORD;
                            uint16_t* imm16 = new uint16_t;
                            *imm16 = static_cast<uint64_t>(imm) & 0xFFFF;
                            currentOperand->data = imm16;
                        } else if (imm >= INT32_MIN && imm <= INT32_MAX) {
                            currentOperand->size = OperandSize::DWORD;
                            uint32_t* imm32 = new uint32_t;
                            *imm32 = static_cast<uint64_t>(imm) & 0xFFFF'FFFF;
                            currentOperand->data = imm32;
                        } else {
                            currentOperand->size = OperandSize::QWORD;
                            uint64_t* imm64 = new uint64_t;
                            *imm64 = static_cast<uint64_t>(imm);
                            currentOperand->data = imm64;
                        }
                        currentOperand->complete = true;
                        currentOperand = nullptr;
                        inOperand = false;
                    } else if (currentOperand->type == OperandType::POTENTIAL_MEMORY) { // must be memory
                        uint64_t* addr = new uint64_t;
                        *addr = static_cast<uint64_t>(strtoull(static_cast<const char*>(token->data), nullptr, 0));
                        currentOperand->data = addr;
                        currentOperand->type = OperandType::MEMORY;
                        currentOperand->complete = true;
                    } else if (currentOperand->type == OperandType::COMPLEX || currentOperand->type == OperandType::MEMORY) { // only other option is complex
                        if (currentOperand->type == OperandType::MEMORY) {
                            currentOperand->type = OperandType::COMPLEX;
                            ComplexData* data = new ComplexData;
                            data->base.present = true;
                            data->base.data.imm.size = OperandSize::QWORD;
                            data->base.data.imm.data = currentOperand->data;
                            data->base.type = ComplexItem::Type::IMMEDIATE;
                            data->index.present = false;
                            data->offset.present = false;
                            data->stage = ComplexData::Stage::BASE;
                            currentOperand->data = data;
                        }
                        ComplexData* data = static_cast<ComplexData*>(currentOperand->data);
                        long imm = strtoll(static_cast<const char*>(token->data), nullptr, 0);
                        void* i_data;
                        OperandSize dataSize;
                        bool negative = imm < 0;
                        if (imm >= INT8_MIN && imm <= INT8_MAX) {
                            dataSize = OperandSize::BYTE;
                            uint8_t* imm8 = new uint8_t;
                            *imm8 = static_cast<uint64_t>(imm) & 0xFF;
                            i_data = imm8;
                        } else if (imm >= INT16_MIN && imm <= INT16_MAX) {
                            dataSize = OperandSize::WORD;
                            uint16_t* imm16 = new uint16_t;
                            *imm16 = static_cast<uint64_t>(imm) & 0xFFFF;
                            i_data = imm16;
                        } else if (imm >= INT32_MIN && imm <= INT32_MAX) {
                            dataSize = OperandSize::DWORD;
                            uint32_t* imm32 = new uint32_t;
                            *imm32 = static_cast<uint64_t>(imm) & 0xFFFF'FFFF;
                            i_data = imm32;
                        } else {
                            dataSize = OperandSize::QWORD;
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
                                data->offset.data.imm.size = dataSize;
                                data->offset.data.imm.data = i_data;
                                data->offset.type = ComplexItem::Type::IMMEDIATE;
                                currentOperand->complete = true;
                            } else {
                                data->base.present = true;
                                data->base.data.imm.size = dataSize;
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
                                data->offset.data.imm.size = dataSize;
                                data->offset.data.imm.data = i_data;
                                data->offset.type = ComplexItem::Type::IMMEDIATE;
                                currentOperand->complete = true;
                            } else {
                                data->index.present = true;
                                data->index.data.imm.size = dataSize;
                                data->index.data.imm.data = i_data;
                                data->index.type = ComplexItem::Type::IMMEDIATE;
                            }
                            break;
                        }
                        case ComplexData::Stage::OFFSET: {
                            if (data->offset.present)
                                error("Invalid immediate location", token);
                            data->offset.present = true;
                            data->offset.data.imm.size = dataSize;
                            data->offset.data.imm.data = i_data;
                            data->offset.type = ComplexItem::Type::IMMEDIATE;
                            currentOperand->complete = true;
                            break;
                        }
                        default:
                            error("Invalid immediate location", token);
                        }
                    } else
                        error("Invalid immediate location", token);
                } else if (token->type == TokenType::REGISTER) {
                    if (currentOperand == nullptr || currentOperand->type == OperandType::UNKNOWN) { // must be just a register
                        if (currentOperand == nullptr) {
                            Operand* operand = new Operand;
                            operand->complete = false;
                            operand->size = OperandSize::QWORD;
                            static_cast<Instruction*>(currentData->data)->operands.insert(operand);
                            currentOperand = operand;
                        }
                        currentOperand->type = OperandType::REGISTER;
                        Register* reg = new Register(GetRegister(static_cast<const char*>(token->data), token->dataSize));
                        currentOperand->data = reg;
                        currentOperand->complete = true;
                        currentOperand = nullptr;
                        inOperand = false;
                    } else if (currentOperand->type == OperandType::POTENTIAL_MEMORY) {
                        currentOperand->type = OperandType::COMPLEX;
                        ComplexData* data = new ComplexData;
                        data->base.present = true;
                        Register* reg = new Register(GetRegister(static_cast<const char*>(token->data), token->dataSize));
                        data->base.data.reg = reg;
                        data->base.type = ComplexItem::Type::REGISTER;
                        data->index.present = false;
                        data->index.data.raw = 0;
                        data->index.type = ComplexItem::Type::UNKNOWN;
                        data->offset.present = false;
                        data->offset.data.raw = 0;
                        data->offset.type = ComplexItem::Type::UNKNOWN;
                        data->stage = ComplexData::Stage::BASE;
                        currentOperand->data = data;
                    } else if (currentOperand->type == OperandType::COMPLEX) {
                        switch (ComplexData* data = static_cast<ComplexData*>(currentOperand->data); data->stage) {
                        case ComplexData::Stage::BASE: {
                            if (data->base.present)
                                error("Invalid Register location", token);
                            data->base.present = true;
                            Register* reg = new Register(GetRegister(static_cast<const char*>(token->data), token->dataSize));
                            data->base.data.reg = reg;
                            data->base.type = ComplexItem::Type::REGISTER;
                            break;
                        }
                        case ComplexData::Stage::INDEX: {
                            if (data->index.present)
                                error("Invalid Register location", token);
                            data->index.present = true;
                            Register* reg = new Register(GetRegister(static_cast<const char*>(token->data), token->dataSize));
                            data->index.data.reg = reg;
                            data->index.type = ComplexItem::Type::REGISTER;
                            break;
                        }
                        case ComplexData::Stage::OFFSET: {
                            if (data->offset.present)
                                error("Invalid Register location", token);
                            data->offset.present = true;
                            Register* reg = new Register(GetRegister(static_cast<const char*>(token->data), token->dataSize));
                            data->offset.data.reg = reg;
                            data->offset.type = ComplexItem::Type::REGISTER;
                            currentOperand->complete = true;
                            break;
                        }
                        }
                    } else
                        error("Invalid Register location", token);
                } else if (token->type == TokenType::LABEL) {
                    if (currentOperand != nullptr && !(currentOperand->type == OperandType::POTENTIAL_MEMORY || currentOperand->type == OperandType::COMPLEX))
                        error("Invalid label location", token);
                    Label* label = nullptr;
                    // find the label
                    char* name = new char[token->dataSize + 1];
                    strncpy(name, static_cast<const char*>(token->data), token->dataSize);
                    name[token->dataSize] = 0;
                    m_labels.Enumerate([&](Label* i_label) -> bool {
                        if (i_label->nameSize < token->dataSize) // strncmp can only properly handle strings of equal or greater length.
                            return true;
                        if (strncmp(i_label->name, name, i_label->nameSize) == 0) {
                            label = i_label;
                            return false;
                        }
                        return true;
                    });
                    delete[] name;
                    if (label == nullptr)
                        error("Invalid label", token, true);
                    if (currentOperand == nullptr) {
                        Operand* operand = new Operand;
                        operand->complete = true;
                        operand->type = OperandType::LABEL;
                        operand->data = label;
                        operand->size = OperandSize::QWORD;
                        static_cast<Instruction*>(currentData->data)->operands.insert(operand);
                        currentOperand = nullptr;
                        inOperand = false;
                    } else if (currentOperand->type == OperandType::POTENTIAL_MEMORY || currentOperand->type == OperandType::COMPLEX) {
                        if (currentOperand->type == OperandType::POTENTIAL_MEMORY) {
                            currentOperand->type = OperandType::COMPLEX;
                            ComplexData* data = new ComplexData;
                            data->base.present = true;
                            data->base.data.label = label;
                            data->base.type = ComplexItem::Type::LABEL;
                            data->index.present = false;
                            data->offset.present = false;
                            data->stage = ComplexData::Stage::BASE;
                            currentOperand->data = data;
                        } else {
                            switch (ComplexData* data = static_cast<ComplexData*>(currentOperand->data); data->stage) {
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
                                currentOperand->complete = true;
                                break;
                            }
                            }
                        }
                    }
                } else if (token->type == TokenType::SUBLABEL) {
                    if (currentOperand != nullptr && !(currentOperand->type == OperandType::POTENTIAL_MEMORY || currentOperand->type == OperandType::COMPLEX))
                        error("Invalid sublabel location", token);
                    Block* block = nullptr;
                    // find the block
                    char* name = new char[token->dataSize + 1];
                    strncpy(name, static_cast<const char*>(token->data), token->dataSize);
                    name[token->dataSize] = 0;
                    if (name[0] == '.')
                        name = &(name[1]);
                    else {
                        delete[] name;
                        error("Invalid sublabel name", token, true);
                    }
                    currentLabel->blocks.Enumerate([&](Block* i_block) -> bool {
                        if (i_block->nameSize < (token->dataSize - 1)) // strncmp can only properly handle strings of equal or greater length
                            return true;
                        if (strncmp(i_block->name, name, i_block->nameSize) == 0) {
                            block = i_block;
                            return false;
                        }
                        return true;
                    });
                    delete[] reinterpret_cast<char*>(reinterpret_cast<uint64_t>(name) - sizeof(char));
                    if (block == nullptr)
                        error("Invalid sublabel", token, true);
                    if (currentOperand == nullptr) {
                        Operand* operand = new Operand;
                        operand->complete = true;
                        operand->type = OperandType::SUBLABEL;
                        operand->data = block;
                        operand->size = OperandSize::QWORD;
                        static_cast<Instruction*>(currentData->data)->operands.insert(operand);
                        currentOperand = nullptr;
                        inOperand = false;
                    } else if (currentOperand->type == OperandType::POTENTIAL_MEMORY) {
                        currentOperand->type = OperandType::COMPLEX;
                        ComplexData* data = new ComplexData;
                        data->base.present = true;
                        data->base.data.sublabel = block;
                        data->base.type = ComplexItem::Type::SUBLABEL;
                        data->index.present = false;
                        data->offset.present = false;
                        data->stage = ComplexData::Stage::BASE;
                        currentOperand->data = data;
                    } else if (currentOperand->type == OperandType::COMPLEX) {
                        switch (ComplexData* data = static_cast<ComplexData*>(currentOperand->data); data->stage) {
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
                            currentOperand->complete = true;
                            break;
                        }
                        }
                    }
                } else if (token->type == TokenType::OPERATOR) {
                    if (currentOperand == nullptr || (currentOperand->type != OperandType::COMPLEX && currentOperand->type != OperandType::MEMORY))
                        error("Invalid operator location", token);
                    if (currentOperand->type == OperandType::MEMORY) {
                        currentOperand->type = OperandType::COMPLEX;
                        int64_t* addr = static_cast<int64_t*>(currentOperand->data);
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
                        currentOperand->data = data;
                    }
                    ComplexData* data = static_cast<ComplexData*>(currentOperand->data);
                    if (size_t nameSize = token->dataSize; EQUALS(static_cast<const char*>(token->data), "+")) {
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
            block->dataBlocks.EnumerateReverse([&](Data* data) -> bool {
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
                            ComplexData* complexData = static_cast<ComplexData*>(operand->data);
                            if (complexData == nullptr)
                                return false;
                            if (complexData->base.present) {
                                if (complexData->base.type == ComplexItem::Type::IMMEDIATE) {
                                    switch (complexData->base.data.imm.size) {
                                    case OperandSize::BYTE:
                                        delete static_cast<uint8_t*>(complexData->base.data.imm.data);
                                        break;
                                    case OperandSize::WORD:
                                        delete static_cast<uint16_t*>(complexData->base.data.imm.data);
                                        break;
                                    case OperandSize::DWORD:
                                        delete static_cast<uint32_t*>(complexData->base.data.imm.data);
                                        break;
                                    case OperandSize::QWORD:
                                        delete static_cast<uint64_t*>(complexData->base.data.imm.data);
                                        break;
                                    }
                                } else if (complexData->base.type == ComplexItem::Type::REGISTER) {
                                    delete complexData->base.data.reg;
                                }
                                // LABEL and SUBLABEL do not need to be deleted
                            }
                            if (complexData->index.present) {
                                if (complexData->index.type == ComplexItem::Type::IMMEDIATE) {
                                    switch (complexData->index.data.imm.size) {
                                    case OperandSize::BYTE:
                                        delete static_cast<uint8_t*>(complexData->index.data.imm.data);
                                        break;
                                    case OperandSize::WORD:
                                        delete static_cast<uint16_t*>(complexData->index.data.imm.data);
                                        break;
                                    case OperandSize::DWORD:
                                        delete static_cast<uint32_t*>(complexData->index.data.imm.data);
                                        break;
                                    case OperandSize::QWORD:
                                        delete static_cast<uint64_t*>(complexData->index.data.imm.data);
                                        break;
                                    }
                                } else if (complexData->index.type == ComplexItem::Type::REGISTER) {
                                    delete complexData->index.data.reg;
                                }
                                // LABEL and SUBLABEL do not need to be deleted
                            }
                            if (complexData->offset.present) {
                                if (complexData->offset.type == ComplexItem::Type::IMMEDIATE) {
                                    switch (complexData->offset.data.imm.size) {
                                    case OperandSize::BYTE:
                                        delete static_cast<uint8_t*>(complexData->offset.data.imm.data);
                                        break;
                                    case OperandSize::WORD:
                                        delete static_cast<uint16_t*>(complexData->offset.data.imm.data);
                                        break;
                                    case OperandSize::DWORD:
                                        delete static_cast<uint32_t*>(complexData->offset.data.imm.data);
                                        break;
                                    case OperandSize::QWORD:
                                        delete static_cast<uint64_t*>(complexData->offset.data.imm.data);
                                        break;
                                    }
                                } else if (complexData->offset.type == ComplexItem::Type::REGISTER) {
                                    delete complexData->offset.data.reg;
                                }
                                // LABEL and SUBLABEL do not need to be deleted
                            }
                            delete complexData;
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
                    RawData* rawData = static_cast<RawData*>(data->data);
                    if (rawData == nullptr)
                        return false;
                    if (rawData->type == RawDataType::RAW) {
                        switch (rawData->dataSize) {
                        case 1:
                            delete static_cast<uint8_t*>(rawData->data);
                            break;
                        case 2:
                            delete static_cast<uint16_t*>(rawData->data);
                            break;
                        case 4:
                            delete static_cast<uint32_t*>(rawData->data);
                            break;
                        case 8:
                            delete static_cast<uint64_t*>(rawData->data);
                            break;
                        default:
                            delete[] static_cast<uint8_t*>(rawData->data);
                            break;
                        }
                    }
                    delete rawData;
                }
                delete data;
                return true;
            });
            block->dataBlocks.clear();
            block->jumpsToHere.EnumerateReverse([&](const uint64_t* jump) -> bool {
                if (jump == nullptr)
                    return false;
                delete jump;
                return true;
            });
            block->jumpsToHere.clear();
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
        name = new char[label->nameSize + 1];
        strncpy(name, label->name, label->nameSize);
        name[label->nameSize] = 0;
        fprintf(fd, "Label: \"%s\":\n", name);
        delete[] name;
        for (uint64_t j = 0; j < label->blocks.getCount(); j++) {
            Block* block = label->blocks.get(j);
            if (block == nullptr)
                return;
            name = new char[block->nameSize + 1];
            strncpy(name, block->name, block->nameSize);
            name[block->nameSize] = 0;
            fprintf(fd, "Block: \"%s\":\n", name);
            delete[] name;
            for (uint64_t k = 0; k < block->dataBlocks.getCount(); k++) {
                Data* data = block->dataBlocks.get(k);
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
                        char const* operandSize = nullptr;
                        switch (operand->size) {
                        case OperandSize::BYTE:
                            operandSize = "byte";
                            break;
                        case OperandSize::WORD:
                            operandSize = "word";
                            break;
                        case OperandSize::DWORD:
                            operandSize = "dword";
                            break;
                        case OperandSize::QWORD:
                            operandSize = "qword";
                            break;
                        }
                        fprintf(fd, "Operand: size = %s, type = %d, ", operandSize, static_cast<int>(operand->type));
                        switch (operand->type) {
                        case OperandType::REGISTER:
                            fprintf(fd, "Register: \"%s\"\n", GetRegisterName(*static_cast<Register*>(operand->data)));
                            break;
                        case OperandType::MEMORY:
                            fprintf(fd, "Memory address: %#18lx\n", *static_cast<uint64_t*>(operand->data));
                            break;
                        case OperandType::COMPLEX: {
                            ComplexData* complexData = static_cast<ComplexData*>(operand->data);
                            if (complexData == nullptr)
                                return;
                            fprintf(fd, "Complex data:\n");
                            if (complexData->base.present) {
                                fprintf(fd, "Base: ");
                                switch (complexData->base.type) {
                                case ComplexItem::Type::IMMEDIATE:
                                    switch (complexData->base.data.imm.size) {
                                    case OperandSize::BYTE:
                                        fprintf(fd, "size = 1, immediate = %#4hhx\n", *static_cast<uint8_t*>(complexData->base.data.imm.data));
                                        break;
                                    case OperandSize::WORD:
                                        fprintf(fd, "size = 2, immediate = %#6hx\n", *static_cast<uint16_t*>(complexData->base.data.imm.data));
                                        break;
                                    case OperandSize::DWORD:
                                        fprintf(fd, "size = 4, immediate = %#10x\n", *static_cast<uint32_t*>(complexData->base.data.imm.data));
                                        break;
                                    case OperandSize::QWORD:
                                        fprintf(fd, "size = 8, immediate = %#18lx\n", *static_cast<uint64_t*>(complexData->base.data.imm.data));
                                        break;
                                    }
                                    break;
                                case ComplexItem::Type::REGISTER:
                                    fprintf(fd, "Register: \"%s\"\n", GetRegisterName(*complexData->base.data.reg));
                                    break;
                                case ComplexItem::Type::LABEL: {
                                    size_t size = complexData->base.data.label->nameSize;
                                    char* i_name = new char[size + 1];
                                    strncpy(i_name, complexData->base.data.label->name, size);
                                    i_name[size] = 0;
                                    fprintf(fd, "Label: \"%s\"\n", i_name);
                                    delete[] i_name;
                                    break;
                                }
                                case ComplexItem::Type::SUBLABEL: {
                                    size_t size = complexData->base.data.sublabel->nameSize;
                                    char* i_name = new char[size + 1];
                                    strncpy(i_name, complexData->base.data.sublabel->name, size);
                                    i_name[size] = 0;
                                    fprintf(fd, "Sublabel: \"%s\"\n", i_name);
                                    delete[] i_name;
                                    break;
                                }
                                case ComplexItem::Type::UNKNOWN:
                                    break;
                                }
                            }
                            if (complexData->index.present) {
                                fprintf(fd, "Index: ");
                                switch (complexData->index.type) {
                                case ComplexItem::Type::IMMEDIATE:
                                    switch (complexData->index.data.imm.size) {
                                    case OperandSize::BYTE:
                                        fprintf(fd, "size = 1, immediate = %#4hhx\n", *static_cast<uint8_t*>(complexData->index.data.imm.data));
                                        break;
                                    case OperandSize::WORD:
                                        fprintf(fd, "size = 2, immediate = %#6hx\n", *static_cast<uint16_t*>(complexData->index.data.imm.data));
                                        break;
                                    case OperandSize::DWORD:
                                        fprintf(fd, "size = 4, immediate = %#10x\n", *static_cast<uint32_t*>(complexData->index.data.imm.data));
                                        break;
                                    case OperandSize::QWORD:
                                        fprintf(fd, "size = 8, immediate = %#18lx\n", *static_cast<uint64_t*>(complexData->index.data.imm.data));
                                        break;
                                    }
                                    break;
                                case ComplexItem::Type::REGISTER:
                                    fprintf(fd, "Register: \"%s\"\n", GetRegisterName(*complexData->index.data.reg));
                                    break;
                                case ComplexItem::Type::LABEL: {
                                    size_t size = complexData->index.data.label->nameSize;
                                    char* i_name = new char[size + 1];
                                    strncpy(i_name, complexData->index.data.label->name, size);
                                    i_name[size] = 0;
                                    fprintf(fd, "Label: \"%s\"\n", i_name);
                                    delete[] i_name;
                                    break;
                                }
                                case ComplexItem::Type::SUBLABEL: {
                                    size_t size = complexData->index.data.sublabel->nameSize;
                                    char* i_name = new char[size + 1];
                                    strncpy(i_name, complexData->index.data.sublabel->name, size);
                                    i_name[size] = 0;
                                    fprintf(fd, "Sublabel: \"%s\"\n", i_name);
                                    delete[] i_name;
                                    break;
                                }
                                case ComplexItem::Type::UNKNOWN:
                                    break;
                                }
                            }
                            if (complexData->offset.present) {
                                fprintf(fd, "Offset: ");
                                switch (complexData->offset.type) {
                                case ComplexItem::Type::IMMEDIATE:
                                    switch (complexData->offset.data.imm.size) {
                                    case OperandSize::BYTE:
                                        fprintf(fd, "size = 1, immediate = %#4hhx\n", *static_cast<uint8_t*>(complexData->offset.data.imm.data));
                                        break;
                                    case OperandSize::WORD:
                                        fprintf(fd, "size = 2, immediate = %#6hx\n", *static_cast<uint16_t*>(complexData->offset.data.imm.data));
                                        break;
                                    case OperandSize::DWORD:
                                        fprintf(fd, "size = 4, immediate = %#10x\n", *static_cast<uint32_t*>(complexData->offset.data.imm.data));
                                        break;
                                    case OperandSize::QWORD:
                                        fprintf(fd, "size = 8, immediate = %#18lx\n", *static_cast<uint64_t*>(complexData->offset.data.imm.data));
                                        break;
                                    }
                                    break;
                                case ComplexItem::Type::REGISTER:
                                    fprintf(fd, "Register: \"%s\", sign = %s\n", GetRegisterName(*complexData->offset.data.reg), complexData->offset.sign ? "positive" : "negative");
                                    break;
                                case ComplexItem::Type::LABEL: {
                                    size_t size = complexData->offset.data.label->nameSize;
                                    char* i_name = new char[size + 1];
                                    strncpy(i_name, complexData->offset.data.label->name, size);
                                    i_name[size] = 0;
                                    fprintf(fd, "Label: \"%s\"\n", i_name);
                                    delete[] i_name;
                                    break;
                                }
                                case ComplexItem::Type::SUBLABEL: {
                                    size_t size = complexData->offset.data.sublabel->nameSize;
                                    char* i_name = new char[size + 1];
                                    strncpy(i_name, complexData->offset.data.sublabel->name, size);
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
                            size_t size = static_cast<Label*>(operand->data)->nameSize;
                            char* i_name = new char[size + 1];
                            strncpy(i_name, static_cast<Label*>(operand->data)->name, size);
                            i_name[size] = 0;
                            fprintf(fd, "Label: \"%s\"\n", i_name);
                            delete[] i_name;
                            break;
                        }
                        case OperandType::SUBLABEL: {
                            size_t size = static_cast<Block*>(operand->data)->nameSize;
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
                    RawData* rawData = static_cast<RawData*>(data->data);
                    if (rawData == nullptr || rawData->data == nullptr)
                        return;
                    fputs("Raw data: ", fd);
                    switch (rawData->type) {
                    case RawDataType::RAW:
                        fprintf(fd, "size = %lu:\n", rawData->dataSize);
                        for (uint64_t l = 0; l < rawData->dataSize; l++)
                            fprintf(fd, "%#2hhx%c", static_cast<uint8_t*>(rawData->data)[l], (l % 8) == 7 ? '\n' : ' ');
                        break;
                    case RawDataType::LABEL:
                        fprintf(fd, "Label: \"%s\"\n", static_cast<Label*>(rawData->data)->name);
                        break;
                    case RawDataType::SUBLABEL:
                        fprintf(fd, "Sublabel: \"%s\"\n", static_cast<Block*>(rawData->data)->name);
                        break;
                    case RawDataType::ASCII:
                        fprintf(fd, "ASCII: \"%s\"\n", static_cast<char*>(rawData->data));
                        break;
                    case RawDataType::ASCIIZ:
                        fprintf(fd, "ASCIIZ: \"%s\"\n", static_cast<char*>(rawData->data));
                        break;
                    case RawDataType::ALIGNMENT:
                        fprintf(fd, "Alignment: %lu\n", *static_cast<uint64_t*>(rawData->data));
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

InsEncoding::Opcode Parser::GetOpcode(const char* name, size_t nameSize) {
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
    return m_opcodes[std::string_view(name, nameSize)];
}

InsEncoding::Register Parser::GetRegister(const char* name, size_t nameSize) {
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
    return m_registers[std::string_view(name, nameSize)];
}

#undef EQUALS

void Parser::error(const char* message, Token* token, bool printToken) {
    printf("Parser error at %s:%zu: %s", token->fileName.c_str(), token->line, message);
    if (printToken)
        printf(": \"%.*s\"", static_cast<int>(token->dataSize), static_cast<const char*>(token->data));
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