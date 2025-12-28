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

#include "Instruction.hpp"

#include <csignal>
#include <cstdlib>
#include <cstring>

#include "Operand.hpp"

namespace InsEncoding {

    struct DecodeData {
        SimpleInstruction instruction;
        ComplexData complexData[3];
        Register currentRegisters[9]; // maximum of 9 registers in an instruction
        uint64_t rawData[9]; // maximum of 9 in an operand
    };

    DecodeData g_decodeDataPool[128]; // NOTE: same size as emulator instruction cache
    DecodeData* g_currentDecodeData;

    Instruction::Instruction()
        : m_opcode(Opcode::UNKNOWN), m_fileName(), m_line(0) {
    }

    Instruction::Instruction(Opcode opcode, const std::string& file_name, size_t line)
        : m_opcode(opcode), m_fileName(file_name), m_line(line) {
    }

    Instruction::~Instruction() {
    }

    void Instruction::SetOpcode(Opcode opcode) {
        m_opcode = opcode;
    }

    Opcode Instruction::GetOpcode() const {
        return m_opcode;
    }

    const std::string& Instruction::GetFileName() const {
        return m_fileName;
    }

    size_t Instruction::GetLine() const {
        return m_line;
    }

    SimpleInstruction::SimpleInstruction()
        : operandCount(0), m_opcode(Opcode::UNKNOWN) {
    }

    SimpleInstruction::SimpleInstruction(Opcode opcode)
        : operandCount(0), m_opcode(opcode) {
    }

    SimpleInstruction::~SimpleInstruction() {
    }

    void SimpleInstruction::SetOpcode(Opcode opcode) {
        m_opcode = opcode;
    }

    Opcode SimpleInstruction::GetOpcode() const {
        return m_opcode;
    }

    [[noreturn]] void EncodingError(const char* message, Instruction* ins) {
        printf("Encoding Error at %s:%zu: %s\n", ins->GetFileName().c_str(), ins->GetLine(), message);
        exit(1);
    }

    RegisterID GetRegisterID(Register reg, Instruction* ins) {
        RegisterID reg_id{};
#define REG_CASE(name, group, num) \
    case Register::name:           \
        reg_id.type = group;       \
        reg_id.number = num;       \
        break;
        switch (reg) {
            REG_CASE(r0, 0, 0)
            REG_CASE(r1, 0, 1)
            REG_CASE(r2, 0, 2)
            REG_CASE(r3, 0, 3)
            REG_CASE(r4, 0, 4)
            REG_CASE(r5, 0, 5)
            REG_CASE(r6, 0, 6)
            REG_CASE(r7, 0, 7)
            REG_CASE(r8, 0, 8)
            REG_CASE(r9, 0, 9)
            REG_CASE(r10, 0, 10)
            REG_CASE(r11, 0, 11)
            REG_CASE(r12, 0, 12)
            REG_CASE(r13, 0, 13)
            REG_CASE(r14, 0, 14)
            REG_CASE(r15, 0, 15)
            REG_CASE(scp, 1, 0)
            REG_CASE(sbp, 1, 1)
            REG_CASE(stp, 1, 2)
            REG_CASE(cr0, 2, 0)
            REG_CASE(cr1, 2, 1)
            REG_CASE(cr2, 2, 2)
            REG_CASE(cr3, 2, 3)
            REG_CASE(cr4, 2, 4)
            REG_CASE(cr5, 2, 5)
            REG_CASE(cr6, 2, 6)
            REG_CASE(cr7, 2, 7)
            REG_CASE(sts, 2, 8)
            REG_CASE(ip, 2, 9)
        default:
            EncodingError("Invalid register type", ins);
        }
#undef REG_CASE
        return reg_id;
    }

    // implemented in assembly for maximum speed
    extern "C" Register GetRegisterFromID(RegisterID reg_id, void (*errorHandler)(const char* message, void* data), void* data);

    uint8_t GetArgCountForOpcode(Opcode opcode) {
        switch (opcode) {
        case Opcode::MUL:
        case Opcode::DIV:
        case Opcode::SMUL:
        case Opcode::SDIV:
            return 3;
        case Opcode::ADD:
        case Opcode::SUB:
        case Opcode::OR:
        case Opcode::NOR:
        case Opcode::XOR:
        case Opcode::XNOR:
        case Opcode::AND:
        case Opcode::NAND:
        case Opcode::CMP:
        case Opcode::SHL:
        case Opcode::SHR:
        case Opcode::MOV:
            return 2;
        case Opcode::INC:
        case Opcode::DEC:
        case Opcode::CALL:
        case Opcode::JMP:
        case Opcode::JC:
        case Opcode::JNC:
        case Opcode::JZ:
        case Opcode::JNZ:
        case Opcode::JL:
        case Opcode::JLE:
        case Opcode::JNL:
        case Opcode::JNLE:
        case Opcode::ENTERUSER:
        case Opcode::PUSH:
        case Opcode::POP:
        case Opcode::INT:
        case Opcode::LIDT:
        case Opcode::NOT:
            return 1;
        case Opcode::HLT:
        case Opcode::NOP:
        case Opcode::SYSRET:
        case Opcode::SYSCALL:
        case Opcode::RET:
        case Opcode::PUSHA:
        case Opcode::POPA:
        case Opcode::IRET:
        default:
            return 0;
        }
    }

    const char* GetInstructionName(Opcode opcode) {
    #define NAME_CASE(ins) \
        case Opcode::ins:  \
            return #ins;
        switch (opcode) {
            NAME_CASE(PUSH)
            NAME_CASE(POP)
            NAME_CASE(PUSHA)
            NAME_CASE(POPA)
            NAME_CASE(ADD)
            NAME_CASE(SUB)
            NAME_CASE(MUL)
            NAME_CASE(DIV)
            NAME_CASE(SMUL)
            NAME_CASE(SDIV)
            NAME_CASE(OR)
            NAME_CASE(XOR)
            NAME_CASE(XNOR)
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

    CompactOperandType GetCompactOperandTypeFromComplex(ComplexItem::Type* types, bool basePresent, bool indexPresent, bool offsetPresent, Instruction* instruction) {
        CompactOperandType type = CompactOperandType::RESERVED;
        if (!basePresent)
            EncodingError("Complex operand must have at least a base", instruction);
        if (indexPresent) {
            if (types[0] != ComplexItem::Type::REGISTER)
                EncodingError("Base of complex operand with index must be a register", instruction);
            if (offsetPresent) {
                // base, index, offset
                if (types[1] == ComplexItem::Type::REGISTER && types[2] == ComplexItem::Type::REGISTER)
                    type = CompactOperandType::MEM_BASE_IDX_OFF_REG;
                else if (types[1] == ComplexItem::Type::REGISTER && types[2] == ComplexItem::Type::IMMEDIATE)
                    type = CompactOperandType::MEM_BASE_IDX_OFF_REG2_IMM;
                else if (types[1] == ComplexItem::Type::IMMEDIATE && types[2] == ComplexItem::Type::REGISTER)
                    type = CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM_REG;
                else if (types[1] == ComplexItem::Type::IMMEDIATE && types[2] == ComplexItem::Type::IMMEDIATE)
                    type = CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2;
                else
                    EncodingError("Invalid complex operand type", instruction);
            }
            else {
                // base, index
                if (types[1] == ComplexItem::Type::REGISTER)
                    type = CompactOperandType::MEM_BASE_IDX_REG;
                else if (types[1] == ComplexItem::Type::IMMEDIATE)
                    type = CompactOperandType::MEM_BASE_IDX_REG_IMM;
                else
                    EncodingError("Invalid complex operand type", instruction);
            }
        }
        else if (offsetPresent) {
            // base, offset
            if (types[0] == ComplexItem::Type::IMMEDIATE) {
                if (types[2] == ComplexItem::Type::REGISTER)
                    type = CompactOperandType::MEM_BASE_OFF_IMM_REG;
                else if (types[2] == ComplexItem::Type::IMMEDIATE)
                    type = CompactOperandType::MEM_BASE_OFF_IMM2;
                else
                    EncodingError("Invalid complex operand type", instruction);
            }
            else if (types[2] == ComplexItem::Type::REGISTER)
                type = CompactOperandType::MEM_BASE_OFF_REG;
            else if (types[2] == ComplexItem::Type::IMMEDIATE)
                type = CompactOperandType::MEM_BASE_OFF_REG_IMM;
            else
                EncodingError("Invalid complex operand type", instruction);
        }
        else {
            // base only
            if (types[0] == ComplexItem::Type::REGISTER)
                type = CompactOperandType::MEM_BASE_REG;
            else if (types[0] == ComplexItem::Type::IMMEDIATE)
                type = CompactOperandType::MEM_BASE_IMM;
            else
                EncodingError("Invalid complex operand type", instruction);
        }
        return type;
    }

#define READ_4BIT_COT(value) (static_cast<CompactOperandType>(static_cast<uint8_t>(value) & 0x0F))


    void ConvertCompactToComplex(ComplexData* out, BasicOperandInfo* basic, ExtendedOperandInfo* extended, bool isExtended) {
        // fill in all the data for the complex data structure
        CompactOperandType type = READ_4BIT_COT(isExtended ? extended->type : basic->type);
        OperandSize imm0Size = isExtended ? extended->imm0Size : basic->imm0Size;
        OperandSize imm1Size = isExtended ? extended->imm1Size : OperandSize::BYTE; // not used in basic
        if (type > CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2 || type < CompactOperandType::MEM_BASE_REG || type == CompactOperandType::MEM_BASE_IMM) {
            out->base.present = false;
            out->index.present = false;
            out->offset.present = false;
            out->base.sign = false;
            out->index.sign = false;
            out->offset.sign = false;
            out->base.type = ComplexItem::Type::UNKNOWN;
            out->index.type = ComplexItem::Type::UNKNOWN;
            out->offset.type = ComplexItem::Type::UNKNOWN;
            out->stage = ComplexData::Stage::BASE;
            return;
        }
        out->base.present = true;
        switch (type) {
        case CompactOperandType::MEM_BASE_REG:
            out->base.type = ComplexItem::Type::REGISTER;
            out->index.present = false;
            out->offset.present = false;
            break;
        case CompactOperandType::MEM_BASE_OFF_REG:
            out->base.type = ComplexItem::Type::REGISTER;
            out->index.present = false;
            out->offset.present = true;
            out->offset.type = ComplexItem::Type::REGISTER;
            break;
        case CompactOperandType::MEM_BASE_OFF_REG_IMM:
            out->base.type = ComplexItem::Type::REGISTER;
            out->index.present = false;
            out->offset.present = true;
            out->offset.type = ComplexItem::Type::IMMEDIATE;
            out->offset.data.imm.size = imm0Size;
            break;
        case CompactOperandType::MEM_BASE_OFF_IMM_REG:
            out->base.type = ComplexItem::Type::IMMEDIATE;
            out->base.data.imm.size = imm0Size;
            out->index.present = false;
            out->offset.present = true;
            out->offset.type = ComplexItem::Type::REGISTER;
            break;
        case CompactOperandType::MEM_BASE_OFF_IMM2:
            out->base.type = ComplexItem::Type::IMMEDIATE;
            out->base.data.imm.size = imm0Size;
            out->index.present = false;
            out->offset.present = true;
            out->offset.type = ComplexItem::Type::IMMEDIATE;
            out->offset.data.imm.size = imm1Size;
            break;
        case CompactOperandType::MEM_BASE_IDX_REG:
            out->base.type = ComplexItem::Type::REGISTER;
            out->index.present = true;
            out->index.type = ComplexItem::Type::REGISTER;
            out->offset.present = false;
            break;
        case CompactOperandType::MEM_BASE_IDX_REG_IMM:
            out->base.type = ComplexItem::Type::REGISTER;
            out->index.present = true;
            out->index.type = ComplexItem::Type::IMMEDIATE;
            out->index.data.imm.size = imm0Size;
            out->offset.present = false;
            break;
        case CompactOperandType::MEM_BASE_IDX_OFF_REG:
            out->base.type = ComplexItem::Type::REGISTER;
            out->index.present = true;
            out->index.type = ComplexItem::Type::REGISTER;
            out->offset.present = true;
            out->offset.type = ComplexItem::Type::REGISTER;
            break;
        case CompactOperandType::MEM_BASE_IDX_OFF_REG2_IMM:
            out->base.type = ComplexItem::Type::REGISTER;
            out->index.present = true;
            out->index.type = ComplexItem::Type::REGISTER;
            out->offset.present = true;
            out->offset.type = ComplexItem::Type::IMMEDIATE;
            out->offset.data.imm.size = imm0Size;
            break;
        case CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM_REG:
            out->base.type = ComplexItem::Type::REGISTER;
            out->index.present = true;
            out->index.type = ComplexItem::Type::IMMEDIATE;
            out->index.data.imm.size = imm0Size;
            out->offset.present = true;
            out->offset.type = ComplexItem::Type::REGISTER;
            break;
        case CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2:
            out->base.type = ComplexItem::Type::REGISTER;
            out->index.present = true;
            out->index.type = ComplexItem::Type::IMMEDIATE;
            out->index.data.imm.size = imm0Size;
            out->offset.present = true;
            out->offset.type = ComplexItem::Type::IMMEDIATE;
            out->offset.data.imm.size = imm1Size;
            break;
        default:
            break;
        }

    }


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

    bool DecodeInstruction(StreamBuffer& buffer, uint64_t& currentOffset, SimpleInstruction* out, int operandDataBuffersOffset, void (*error_handler)(const char* message, void* data), void* error_data) {
        if (out == nullptr)
            return false;

        g_currentDecodeData = &g_decodeDataPool[operandDataBuffersOffset];

        uint8_t rawOpcode;
        buffer.ReadStream8(rawOpcode);
        currentOffset++;

        g_currentDecodeData->instruction.SetOpcode(static_cast<Opcode>(rawOpcode));
        g_currentDecodeData->instruction.operandCount = 0;

        uint8_t argCount = GetArgCountForOpcode(g_currentDecodeData->instruction.GetOpcode());
        if (argCount == 0) {
            *out = g_currentDecodeData->instruction;
            return true;
        }

        CompactOperandType compactOperandTypes[3];
        OperandType operandTypes[3];
        OperandSize operandSizes[3];

        BasicOperandInfo basicInfos[3];
        ExtendedOperandInfo extendedInfos[3];

        bool isExtendedOperand[3] = {false, false, false};

        if (argCount == 1) {
            buffer.ReadStream8(*reinterpret_cast<uint8_t*>(&basicInfos[0]));
            currentOffset += sizeof(uint8_t);

            if (READ_4BIT_COT(basicInfos[0].type) == CompactOperandType::MEM_BASE_OFF_IMM2 || READ_4BIT_COT(basicInfos[0].type) == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2) {
                memcpy(&extendedInfos[0], &basicInfos[0], sizeof(BasicOperandInfo));
                buffer.ReadStream8(*(reinterpret_cast<uint8_t*>(&extendedInfos[0]) + sizeof(BasicOperandInfo)));
                currentOffset += sizeof(uint8_t);
                isExtendedOperand[0] = true;
            }
            compactOperandTypes[0] = READ_4BIT_COT(basicInfos[0].type);
            operandTypes[0] = CONVERT_COMPACT_TO_OPERAND(compactOperandTypes[0]);
            operandSizes[0] = basicInfos[0].size;
        } else if (argCount == 2 || argCount == 3) {
            buffer.ReadStream8(*reinterpret_cast<uint8_t*>(&basicInfos[0]));
            currentOffset += sizeof(uint8_t);

            bool triple = false;

            if (READ_4BIT_COT(basicInfos[0].type) == CompactOperandType::MEM_BASE_OFF_IMM2 || READ_4BIT_COT(basicInfos[0].type) == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2) {
                memcpy(&extendedInfos[0], &basicInfos[0], sizeof(BasicOperandInfo));
                buffer.ReadStream8(*(reinterpret_cast<uint8_t*>(&extendedInfos[0]) + sizeof(BasicOperandInfo)));
                currentOffset += sizeof(uint8_t);
                isExtendedOperand[0] = true;
                // need to check highest 4 bits of the ExtendedOperandInfo to see if the next operand is extended
                if (CompactOperandType nextType = static_cast<CompactOperandType>(*(reinterpret_cast<uint8_t*>(&extendedInfos[0]) + sizeof(BasicOperandInfo)) >> 4); nextType == CompactOperandType::MEM_BASE_OFF_IMM2 || nextType == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2) {
                    DoubleExtendedOperandInfo doubleExtendedInfo;
                    memcpy(&doubleExtendedInfo, &extendedInfos[0], sizeof(ExtendedOperandInfo));
                    buffer.ReadStream8(*(reinterpret_cast<uint8_t*>(&doubleExtendedInfo) + sizeof(ExtendedOperandInfo)));
                    currentOffset += sizeof(uint8_t);
                    extendedInfos[1].type = doubleExtendedInfo.second.type;
                    extendedInfos[1].size = doubleExtendedInfo.second.size;
                    extendedInfos[1].imm0Size = doubleExtendedInfo.second.imm0Size;
                    extendedInfos[1].imm1Size = doubleExtendedInfo.second.imm1Size;
                    isExtendedOperand[1] = true;
                }
                else if (argCount == 3 && (extendedInfos[0].reserved & 1 << 1) > 0) { // need to check the highest reserved bit of the ExtendedOperandInfo to see if the next operand is triple extended
                    TripleExtendedOperandInfo tripleExtendedInfo;
                    memcpy(&tripleExtendedInfo, &extendedInfos[0], sizeof(ExtendedOperandInfo));
                    buffer.ReadStream8(*(reinterpret_cast<uint8_t*>(&tripleExtendedInfo) + sizeof(ExtendedOperandInfo)));
                    currentOffset += sizeof(uint8_t);
                    basicInfos[1].type = tripleExtendedInfo.second.type;
                    basicInfos[1].size = tripleExtendedInfo.second.size;
                    basicInfos[1].imm0Size = tripleExtendedInfo.second.imm0Size;
                    isExtendedOperand[1] = false;
                    extendedInfos[2].type = tripleExtendedInfo.third.type;
                    extendedInfos[2].size = tripleExtendedInfo.third.size;
                    extendedInfos[2].imm0Size = tripleExtendedInfo.third.imm0Size;
                    extendedInfos[2].imm1Size = tripleExtendedInfo.third.imm1Size; // not used in triple extended
                    isExtendedOperand[2] = true;
                    compactOperandTypes[2] = basicInfos[2].type;
                    operandSizes[2] = basicInfos[2].size;
                    operandTypes[2] = CONVERT_COMPACT_TO_OPERAND(compactOperandTypes[2]);
                    triple = true;
                }
            }

            buffer.ReadStream8(*reinterpret_cast<uint8_t*>(&basicInfos[1]));
            currentOffset += sizeof(uint8_t);
            if (READ_4BIT_COT(basicInfos[1].type) == CompactOperandType::MEM_BASE_OFF_IMM2 || READ_4BIT_COT(basicInfos[1].type) == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2) {
                memcpy(&extendedInfos[1], &basicInfos[1], sizeof(BasicOperandInfo));
                buffer.ReadStream8(*(reinterpret_cast<uint8_t*>(&extendedInfos[1]) + sizeof(BasicOperandInfo)));
                currentOffset += sizeof(uint8_t);
                isExtendedOperand[1] = true;
            }

            compactOperandTypes[0] = static_cast<CompactOperandType>(static_cast<uint8_t>(isExtendedOperand[0] ? extendedInfos[0].type : basicInfos[0].type) & 0x0F);
            operandSizes[0] = isExtendedOperand[0] ? extendedInfos[0].size : basicInfos[0].size;
            compactOperandTypes[1] = static_cast<CompactOperandType>(static_cast<uint8_t>(isExtendedOperand[1] ? extendedInfos[1].type : basicInfos[1].type) & 0x0F);
            operandSizes[1] = isExtendedOperand[1] ? extendedInfos[1].size : basicInfos[1].size;

            if (argCount == 3 && !triple) {
                if (!isExtendedOperand[0] && isExtendedOperand[1]) {
                    // need to check highest 4 bits of the ExtendedOperandInfo to see if the next operand is extended
                    if (CompactOperandType nextType = static_cast<CompactOperandType>(*(reinterpret_cast<uint8_t*>(&extendedInfos[1]) + sizeof(BasicOperandInfo)) >> 4); nextType == CompactOperandType::MEM_BASE_OFF_IMM2 || nextType == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2) {
                        DoubleExtendedOperandInfo doubleExtendedInfo;
                        memcpy(&doubleExtendedInfo, &extendedInfos[1], sizeof(ExtendedOperandInfo));
                        buffer.ReadStream8(*(reinterpret_cast<uint8_t*>(&doubleExtendedInfo) + sizeof(ExtendedOperandInfo)));
                        currentOffset += sizeof(uint8_t);
                        extendedInfos[2].type = doubleExtendedInfo.second.type;
                        extendedInfos[2].size = doubleExtendedInfo.second.size;
                        extendedInfos[2].imm0Size = doubleExtendedInfo.second.imm0Size;
                        extendedInfos[2].imm1Size = doubleExtendedInfo.second.imm1Size;
                        isExtendedOperand[2] = true;
                    }
                }

                buffer.ReadStream8(*reinterpret_cast<uint8_t*>(&basicInfos[2]));
                currentOffset += sizeof(uint8_t);
                if (READ_4BIT_COT(basicInfos[2].type) == CompactOperandType::MEM_BASE_OFF_IMM2 || READ_4BIT_COT(basicInfos[2].type) == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2) {
                    memcpy(&extendedInfos[2], &basicInfos[2], sizeof(BasicOperandInfo));
                    buffer.ReadStream8(*(reinterpret_cast<uint8_t*>(&extendedInfos[2]) + sizeof(BasicOperandInfo)));
                    currentOffset += sizeof(uint8_t);
                    isExtendedOperand[2] = true;
                }
                compactOperandTypes[2] = READ_4BIT_COT(isExtendedOperand[2] ? extendedInfos[2].type : basicInfos[2].type);
                operandSizes[2] = isExtendedOperand[2] ? extendedInfos[2].size : basicInfos[2].size;
                operandTypes[2] = CONVERT_COMPACT_TO_OPERAND(compactOperandTypes[2]);
            }
            operandTypes[0] = CONVERT_COMPACT_TO_OPERAND(compactOperandTypes[0]);
            operandTypes[1] = CONVERT_COMPACT_TO_OPERAND(compactOperandTypes[1]);
        } else
            error_handler("Invalid argument count", error_data);

        for (uint8_t i = 0; i < argCount; i++) {
            OperandType operandType = operandTypes[i];
            OperandSize operandSize = operandSizes[i];

            Operand operand(operandType, operandSize, nullptr);

            switch (operandType) {
            case OperandType::COMPLEX: {
                ComplexData* complex = &g_currentDecodeData->complexData[i];
                ConvertCompactToComplex(complex, &basicInfos[i], &extendedInfos[i], isExtendedOperand[i]);
                if (complex->base.present) {
                    if (complex->base.type == ComplexItem::Type::IMMEDIATE) {
                        complex->base.data.imm.data = &g_currentDecodeData->rawData[i * 3];
                        switch (complex->base.data.imm.size) {
#define SIZE_CASE(size, bits) \
        case OperandSize::size: \
            buffer.ReadStream##bits(*reinterpret_cast<uint##bits##_t*>(complex->base.data.imm.data)); \
            break
                        SIZE_CASE(BYTE, 8);
                        SIZE_CASE(WORD, 16);
                        SIZE_CASE(DWORD, 32);
                        SIZE_CASE(QWORD, 64);
#undef SIZE_CASE
                        }
                        currentOffset += 1 << static_cast<uint8_t>(complex->base.data.imm.size);
                    } else if (complex->base.type == ComplexItem::Type::REGISTER) {
                        RegisterID reg_id{};
                        buffer.ReadStream8(reinterpret_cast<uint8_t&>(reg_id));
                        currentOffset += sizeof(RegisterID);
                        complex->base.data.reg = &g_currentDecodeData->currentRegisters[i * 3];
                        *complex->base.data.reg = GetRegisterFromID(reg_id, error_handler, error_data);
                    }
                }
                if (complex->index.present) {
                    if (complex->index.type == ComplexItem::Type::IMMEDIATE) {
                        complex->index.data.imm.data = &g_currentDecodeData->rawData[i * 3 + 1];
                        switch (complex->index.data.imm.size) {
#define SIZE_CASE(size, bits) \
        case OperandSize::size: \
            buffer.ReadStream##bits(*reinterpret_cast<uint##bits##_t*>(complex->index.data.imm.data)); \
            break
                        SIZE_CASE(BYTE, 8);
                        SIZE_CASE(WORD, 16);
                        SIZE_CASE(DWORD, 32);
                        SIZE_CASE(QWORD, 64);
#undef SIZE_CASE
                        }
                        currentOffset += 1 << static_cast<uint8_t>(complex->index.data.imm.size);
                    } else if (complex->index.type == ComplexItem::Type::REGISTER) {
                        RegisterID reg_id{};
                        buffer.ReadStream8(reinterpret_cast<uint8_t&>(reg_id));
                        currentOffset += sizeof(RegisterID);
                        complex->index.data.reg = &g_currentDecodeData->currentRegisters[i * 3 + 1];
                        *complex->index.data.reg = GetRegisterFromID(reg_id, error_handler, error_data);
                    }
                }
                if (complex->offset.present) {
                    if (complex->offset.type == ComplexItem::Type::IMMEDIATE) {
                        complex->offset.data.imm.data = &g_currentDecodeData->rawData[i * 3 + 2];
                        switch (complex->offset.data.imm.size) {
#define SIZE_CASE(size, bits) \
        case OperandSize::size: \
            buffer.ReadStream##bits(*reinterpret_cast<uint##bits##_t*>(complex->offset.data.imm.data)); \
            break
                        SIZE_CASE(BYTE, 8);
                        SIZE_CASE(WORD, 16);
                        SIZE_CASE(DWORD, 32);
                        SIZE_CASE(QWORD, 64);
#undef SIZE_CASE
                        }
                        currentOffset += 1 << static_cast<uint8_t>(complex->offset.data.imm.size);
                    } else if (complex->offset.type == ComplexItem::Type::REGISTER) {
                        RegisterID reg_id{};
                        buffer.ReadStream8(reinterpret_cast<uint8_t&>(reg_id));
                        currentOffset += sizeof(RegisterID);
                        complex->offset.data.reg = &g_currentDecodeData->currentRegisters[i * 3 + 2];
                        complex->offset.sign = reg_id.type & 1 << 3; // sign is stored in the highest bit of the type
                        reg_id.type &= ~(1 << 3); // clear the sign bit
                        *complex->offset.data.reg = GetRegisterFromID(reg_id, error_handler, error_data);
                    }
                }
                operand.data = complex;
                break;
            }
            case OperandType::REGISTER: {
                RegisterID reg_id{};
                buffer.ReadStream8(reinterpret_cast<uint8_t&>(reg_id));
                currentOffset += sizeof(RegisterID);
                Register* reg = &g_currentDecodeData->currentRegisters[i * 3];
                *reg = GetRegisterFromID(reg_id, error_handler, error_data);
                operand.data = reg;
                break;
            }
            case OperandType::MEMORY: {
                uint8_t* mem_data = reinterpret_cast<uint8_t*>(&g_currentDecodeData->rawData[i * 3]);
                buffer.ReadStream64(*reinterpret_cast<uint64_t*>(mem_data));
                currentOffset += 8;
                operand.data = mem_data;
                break;
            }
            case OperandType::IMMEDIATE: {
                uint8_t* imm_data = reinterpret_cast<uint8_t*>(&g_currentDecodeData->rawData[i * 3]);
                switch (operandSize) {
#define SIZE_CASE(size, bits) \
        case OperandSize::size: \
            buffer.ReadStream##bits(*reinterpret_cast<uint##bits##_t*>(imm_data)); \
            break
                SIZE_CASE(BYTE, 8);
                SIZE_CASE(WORD, 16);
                SIZE_CASE(DWORD, 32);
                SIZE_CASE(QWORD, 64);
#undef SIZE_CASE
                }
                currentOffset += 1 << static_cast<uint8_t>(operandSize);
                operand.data = imm_data;
                break;
            }
            default:
                error_handler("Invalid operand type", error_data);
            }

            g_currentDecodeData->instruction.operands[g_currentDecodeData->instruction.operandCount] = operand;
            g_currentDecodeData->instruction.operandCount++;
        }

        *out = g_currentDecodeData->instruction;
        return true;
    }

#pragma GCC diagnostic pop

    size_t EncodeInstruction(Instruction* instruction, uint8_t* data, size_t data_size, uint64_t global_offset, DataSection* dataSection) {
        Buffer buffer;
        uint64_t current_offset = 0;

        if (instruction->operands.getCount() > 3)
            EncodingError("Instruction has more than 3 operands", instruction);

        uint8_t arg_count = GetArgCountForOpcode(instruction->GetOpcode());
        if (instruction->operands.getCount() != arg_count)
            EncodingError("Invalid number of arguments for instruction", instruction);

        uint8_t opcode = static_cast<uint8_t>(instruction->GetOpcode());

        buffer.Write(current_offset, &opcode, 1);
        current_offset++;

        Operand* operands[3] = {nullptr, nullptr, nullptr};
        for (uint64_t l = 0; l < instruction->operands.getCount(); l++)
            operands[l] = instruction->operands.get(l);

        if (arg_count == 1) {
            if (operands[0]->type == OperandType::COMPLEX) {
                ComplexData* complex = static_cast<ComplexData*>(operands[0]->data);
                // need to detect how many immediates there are to know what size to use. Also helpful to determine exact type
                ComplexItem::Type types[3] = {ComplexItem::Type::REGISTER, ComplexItem::Type::REGISTER, ComplexItem::Type::REGISTER};
                types[0] = complex->base.present ? (complex->base.type == ComplexItem::Type::REGISTER ? ComplexItem::Type::REGISTER : ComplexItem::Type::IMMEDIATE) : ComplexItem::Type::REGISTER;
                types[1] = complex->index.present ? (complex->index.type == ComplexItem::Type::REGISTER ? ComplexItem::Type::REGISTER : ComplexItem::Type::IMMEDIATE) : ComplexItem::Type::REGISTER;
                types[2] = complex->offset.present ? (complex->offset.type == ComplexItem::Type::REGISTER ? ComplexItem::Type::REGISTER : ComplexItem::Type::IMMEDIATE) : ComplexItem::Type::REGISTER;
                CompactOperandType type = GetCompactOperandTypeFromComplex(types, complex->base.present, complex->index.present, complex->offset.present, instruction);
                if (type == CompactOperandType::RESERVED)
                    EncodingError("Invalid complex operand type", instruction);
                if (type == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2 || type == CompactOperandType::MEM_BASE_OFF_IMM2) {
                    // Extended operand type, needs 2 bytes
                    ExtendedOperandInfo ext_info;
                    ext_info.type = type;
                    ext_info.size = operands[0]->size;
                    if (type == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2)
                        ext_info.imm0Size = complex->index.type == ComplexItem::Type::IMMEDIATE ? complex->index.data.imm.size : OperandSize::QWORD;
                    else
                        ext_info.imm0Size = complex->base.type == ComplexItem::Type::IMMEDIATE ? complex->base.data.imm.size : OperandSize::QWORD;
                    ext_info.imm1Size = complex->offset.type == ComplexItem::Type::IMMEDIATE ? complex->offset.data.imm.size : OperandSize::QWORD;
                    ext_info.reserved = 0;
                    buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&ext_info), sizeof(ExtendedOperandInfo));
                    current_offset += sizeof(ExtendedOperandInfo);
                }
                else {
                    BasicOperandInfo info;
                    info.type = type;
                    info.size = operands[0]->size;
                    if (type == CompactOperandType::MEM_BASE_IMM || type == CompactOperandType::MEM_BASE_OFF_IMM_REG)
                        info.imm0Size = complex->base.type == ComplexItem::Type::IMMEDIATE ? complex->base.data.imm.size : OperandSize::QWORD;
                    else if (type == CompactOperandType::MEM_BASE_IDX_REG_IMM || type == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM_REG)
                        info.imm0Size = complex->index.type == ComplexItem::Type::IMMEDIATE ? complex->index.data.imm.size : OperandSize::QWORD;
                    else if (type == CompactOperandType::MEM_BASE_OFF_REG_IMM || type == CompactOperandType::MEM_BASE_IDX_OFF_REG2_IMM)
                        info.imm0Size = complex->offset.type == ComplexItem::Type::IMMEDIATE ? complex->offset.data.imm.size : OperandSize::QWORD;
                    else if (!(type == CompactOperandType::MEM_BASE_REG || type == CompactOperandType::MEM_BASE_OFF_REG || type == CompactOperandType::MEM_BASE_IDX_REG || type == CompactOperandType::MEM_BASE_IDX_OFF_REG))
                        EncodingError("Invalid complex operand type", instruction);
                    buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&info), sizeof(BasicOperandInfo));
                    current_offset += sizeof(BasicOperandInfo);
                }
            } else if (operands[0]->type == OperandType::MEMORY) {
                BasicOperandInfo info;
                info.type = CompactOperandType::MEM_BASE_IMM;
                info.size = operands[0]->size;
                info.imm0Size = OperandSize::QWORD; // memory address is always 64 bits
                buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&info), sizeof(BasicOperandInfo));
                current_offset += sizeof(BasicOperandInfo);
            } else {
                BasicOperandInfo info;

                if (operands[0]->type == OperandType::LABEL || operands[0]->type == OperandType::SUBLABEL)
                    info.type = CompactOperandType::IMM;
                else if (operands[0]->type == OperandType::REGISTER || operands[0]->type == OperandType::IMMEDIATE)
                    info.type = static_cast<CompactOperandType>(operands[0]->type);
                else
                    EncodingError("Invalid operand type for single-argument instruction", instruction);

                info.size = operands[0]->size;
                info.imm0Size = operands[0]->size; // no point in doing a check, will only be read if type is IMM
                buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&info), sizeof(BasicOperandInfo));
                current_offset += sizeof(BasicOperandInfo);
            }
        }
        else if (arg_count == 2 || arg_count == 3) {
            CompactOperandType types[3] = {CompactOperandType::RESERVED, CompactOperandType::RESERVED, CompactOperandType::RESERVED};
            union {
                BasicOperandInfo info;
                ExtendedOperandInfo extendedInfo;
            } infos[3];
            for (uint8_t i = 0; i < arg_count; i++) {
                if (operands[i]->type == OperandType::COMPLEX) {
                    ComplexData* complex = static_cast<ComplexData*>(operands[i]->data);
                    ComplexItem::Type item_types[3] = {ComplexItem::Type::REGISTER, ComplexItem::Type::REGISTER, ComplexItem::Type::REGISTER};
                    item_types[0] = complex->base.present ? (complex->base.type == ComplexItem::Type::REGISTER ? ComplexItem::Type::REGISTER : ComplexItem::Type::IMMEDIATE) : ComplexItem::Type::REGISTER;
                    item_types[1] = complex->index.present ? (complex->index.type == ComplexItem::Type::REGISTER ? ComplexItem::Type::REGISTER : ComplexItem::Type::IMMEDIATE) : ComplexItem::Type::REGISTER;
                    item_types[2] = complex->offset.present ? (complex->offset.type == ComplexItem::Type::REGISTER ? ComplexItem::Type::REGISTER : ComplexItem::Type::IMMEDIATE) : ComplexItem::Type::REGISTER;
                    types[i] = GetCompactOperandTypeFromComplex(item_types, complex->base.present, complex->index.present, complex->offset.present, instruction);
                    if (types[i] == CompactOperandType::RESERVED)
                        EncodingError("Invalid complex operand type", instruction);
                    if (types[i] == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2 || types[i] == CompactOperandType::MEM_BASE_OFF_IMM2) {
                        infos[i].extendedInfo = {};
                        infos[i].extendedInfo.type = types[i];
                        infos[i].extendedInfo.size = operands[i]->size;
                        if (types[i] == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2)
                            infos[i].extendedInfo.imm0Size = complex->index.type == ComplexItem::Type::IMMEDIATE ? complex->index.data.imm.size : OperandSize::QWORD;
                        else
                            infos[i].extendedInfo.imm0Size = complex->base.type == ComplexItem::Type::IMMEDIATE ? complex->base.data.imm.size : OperandSize::QWORD;
                        infos[i].extendedInfo.imm1Size = complex->offset.type == ComplexItem::Type::IMMEDIATE ? complex->offset.data.imm.size : OperandSize::QWORD;
                        infos[i].extendedInfo.reserved = 0;
                    } else {
                        infos[i].info = {};
                        infos[i].info.type = types[i];
                        infos[i].info.size = operands[i]->size;
                        if (types[i] == CompactOperandType::MEM_BASE_IMM || types[i] == CompactOperandType::MEM_BASE_OFF_IMM_REG)
                            infos[i].info.imm0Size = complex->base.type == ComplexItem::Type::IMMEDIATE ? complex->base.data.imm.size : OperandSize::QWORD;
                        else if (types[i] == CompactOperandType::MEM_BASE_IDX_REG_IMM || types[i] == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM_REG)
                            infos[i].info.imm0Size = complex->index.type == ComplexItem::Type::IMMEDIATE ? complex->index.data.imm.size : OperandSize::QWORD;
                        else if (types[i] == CompactOperandType::MEM_BASE_OFF_REG_IMM || types[i] == CompactOperandType::MEM_BASE_IDX_OFF_REG2_IMM)
                            infos[i].info.imm0Size = complex->offset.type == ComplexItem::Type::IMMEDIATE ? complex->offset.data.imm.size : OperandSize::QWORD;
                        else if (!(types[i] == CompactOperandType::MEM_BASE_REG || types[i] == CompactOperandType::MEM_BASE_OFF_REG || types[i] == CompactOperandType::MEM_BASE_IDX_REG || types[i] == CompactOperandType::MEM_BASE_IDX_OFF_REG))
                            EncodingError("Invalid complex operand type", instruction);
                    }
                } else if (operands[i]->type == OperandType::MEMORY) {
                    infos[i].info = {};
                    infos[i].info.type = CompactOperandType::MEM_BASE_IMM;
                    infos[i].info.size = operands[i]->size;
                    infos[i].info.imm0Size = OperandSize::QWORD; // memory address is always 64 bits
                } else {
                    infos[i].info = {};
                    if (operands[i]->type == OperandType::LABEL || operands[i]->type == OperandType::SUBLABEL)
                        infos[i].info.type = CompactOperandType::IMM;
                    else if (operands[i]->type == OperandType::REGISTER || operands[i]->type == OperandType::IMMEDIATE)
                        infos[i].info.type = static_cast<CompactOperandType>(operands[i]->type);
                    else
                        EncodingError("Invalid operand type for multi-argument instruction", instruction);
                    infos[i].info.size = operands[i]->size;
                    infos[i].info.imm0Size = operands[i]->size; // no point in doing a check, will only be read if type is IMM
                    types[i] = infos[i].info.type;
                }
            }
            if (arg_count == 2) {
                // 4 cases: both basic, first basic + second extended, first extended + second basic, both extended
                // only extended if type is MEM_BASE_IDX_OFF_REG_IMM2. can write individually for all cases except both extended
                if (!(types[0] == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2 && types[1] == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2) && !(types[0] == CompactOperandType::MEM_BASE_OFF_IMM2 && types[1] == CompactOperandType::MEM_BASE_OFF_IMM2)) {
                    // not both extended, can write individually
                    size_t sizes[2] = {types[0] == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2 || types[0] == CompactOperandType::MEM_BASE_OFF_IMM2 ? sizeof(ExtendedOperandInfo) : sizeof(BasicOperandInfo),
                                       types[1] == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2 || types[1] == CompactOperandType::MEM_BASE_OFF_IMM2 ? sizeof(ExtendedOperandInfo) : sizeof(BasicOperandInfo)};
                    for (uint8_t i = 0; i < 2; i++) {
                        buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&infos[i]), sizes[i]);
                        current_offset += sizes[i];
                    }
                }
                else {
                    // both extended, need to convert to a combined struct
                    DoubleExtendedOperandInfo combined_info{};
                    combined_info.first.type = infos[0].extendedInfo.type;
                    combined_info.first.size = infos[0].extendedInfo.size;
                    combined_info.first.imm0Size = infos[0].extendedInfo.imm0Size;
                    combined_info.first.imm1Size = infos[0].extendedInfo.imm1Size;
                    combined_info.first.reserved = 0;
                    combined_info.second.type = infos[1].extendedInfo.type;
                    combined_info.second.size = infos[1].extendedInfo.size;
                    combined_info.second.imm0Size = infos[1].extendedInfo.imm0Size;
                    combined_info.second.imm1Size = infos[1].extendedInfo.imm1Size;
                    combined_info.second.reserved = 0;
                    buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&combined_info), sizeof(DoubleExtendedOperandInfo));
                    current_offset += sizeof(DoubleExtendedOperandInfo);
                }
            } else {
                /* 8 cases:
                 * All 3 basic - just write 3 basic infos in a row
                 * First & second basic, third extended - write all in a row
                 * First basic, second extended, third basic - write all in a row
                 * First basic, second and third extended - need to combine second and third extended into a double extended struct, write first basic then combined
                 * First extended, second and third basic - write all in a row
                 * First extended, second basic, third extended - need to combine all 3 into 1 struct
                 * First and second extended, third basic - need to combine first and second extended into a double extended struct, write combined then third basic
                 * All 3 extended - need to combine first and second into a double extended struct, write combined then third extended
                 *
                 * This results in 3 options - either write all 3 individually, combine to into a struct, or combine all 3 into a struct
                 */
#define BASIC !=
#define EXTENDED ==
#define CHECK_TYPES(type1, type2, type3) ((types[0] type1 CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2 \
                                          && types[1] type2 CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2 \
                                          && types[2] type3 CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2) \
                                          || (types[0] type1 CompactOperandType::MEM_BASE_OFF_IMM2 \
                                              && types[1] type2 CompactOperandType::MEM_BASE_OFF_IMM2 \
                                              && types[2] type3 CompactOperandType::MEM_BASE_OFF_IMM2))

                if (CHECK_TYPES(BASIC, BASIC, BASIC) || CHECK_TYPES(BASIC, BASIC, EXTENDED) || CHECK_TYPES(BASIC, EXTENDED, BASIC) || CHECK_TYPES(EXTENDED, BASIC, BASIC)) {
                    // case 1, write all 3 individually
                    size_t sizes[3] = {types[0] == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2 || types[0] == CompactOperandType::MEM_BASE_OFF_IMM2 ? sizeof(ExtendedOperandInfo) : sizeof(BasicOperandInfo),
                                       types[1] == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2 || types[1] == CompactOperandType::MEM_BASE_OFF_IMM2 ? sizeof(ExtendedOperandInfo) : sizeof(BasicOperandInfo),
                                       types[2] == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2 || types[2] == CompactOperandType::MEM_BASE_OFF_IMM2 ? sizeof(ExtendedOperandInfo) : sizeof(BasicOperandInfo)};
                    for (uint8_t i = 0; i < 3; i++) {
                        buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&infos[i]), sizes[i]);
                        current_offset += sizes[i];
                    }
                }
                else if (CHECK_TYPES(BASIC, EXTENDED, EXTENDED)) {
                    // case 2, need to combine second and third into a double extended struct
                    DoubleExtendedOperandInfo combined_info{};
                    combined_info.first.type = infos[1].extendedInfo.type;
                    combined_info.first.size = infos[1].extendedInfo.size;
                    combined_info.first.imm0Size = infos[1].extendedInfo.imm0Size;
                    combined_info.first.imm1Size = infos[1].extendedInfo.imm1Size;
                    combined_info.first.reserved = 0;
                    combined_info.second.type = infos[2].extendedInfo.type;
                    combined_info.second.size = infos[2].extendedInfo.size;
                    combined_info.second.imm0Size = infos[2].extendedInfo.imm0Size;
                    combined_info.second.imm1Size = infos[2].extendedInfo.imm1Size;
                    combined_info.second.reserved = 0;
                    buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&infos[0].info), sizeof(BasicOperandInfo));
                    current_offset += sizeof(BasicOperandInfo);
                    buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&combined_info), sizeof(DoubleExtendedOperandInfo));
                    current_offset += sizeof(DoubleExtendedOperandInfo);
                }
                else if (CHECK_TYPES(EXTENDED, BASIC, EXTENDED)) {
                    // case 3, need to combine all 3 into a triple extended struct
                    TripleExtendedOperandInfo combined_info{};
                    combined_info.first.type = infos[0].extendedInfo.type;
                    combined_info.first.size = infos[0].extendedInfo.size;
                    combined_info.first.imm0Size = infos[0].extendedInfo.imm0Size;
                    combined_info.first.imm1Size = infos[0].extendedInfo.imm1Size;
                    combined_info.first.reserved = 0;
                    combined_info.first.isTripleExtended = 1;
                    combined_info.second.type = infos[1].info.type;
                    combined_info.second.size = infos[1].info.size;
                    combined_info.second.imm0Size = infos[1].info.imm0Size;
                    combined_info.third.type = infos[2].extendedInfo.type;
                    combined_info.third.size = infos[2].extendedInfo.size;
                    combined_info.third.imm0Size = infos[2].extendedInfo.imm0Size;
                    combined_info.third.imm1Size = infos[2].extendedInfo.imm1Size;
                    combined_info.third.reserved = 0;
                    buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&combined_info), sizeof(TripleExtendedOperandInfo));
                    current_offset += sizeof(TripleExtendedOperandInfo);
                }
                else if (CHECK_TYPES(EXTENDED, EXTENDED, BASIC) || CHECK_TYPES(EXTENDED, EXTENDED, EXTENDED)) {
                    // case 4, need to combine first and second into a double extended struct
                    DoubleExtendedOperandInfo combined_info{};
                    combined_info.first.type = infos[0].extendedInfo.type;
                    combined_info.first.size = infos[0].extendedInfo.size;
                    combined_info.first.imm0Size = infos[0].extendedInfo.imm0Size;
                    combined_info.first.imm1Size = infos[0].extendedInfo.imm1Size;
                    combined_info.first.reserved = 0;
                    combined_info.second.type = infos[1].extendedInfo.type;
                    combined_info.second.size = infos[1].extendedInfo.size;
                    combined_info.second.imm0Size = infos[1].extendedInfo.imm0Size;
                    combined_info.second.imm1Size = infos[1].extendedInfo.imm1Size;
                    combined_info.second.reserved = 0;
                    buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&combined_info), sizeof(DoubleExtendedOperandInfo));
                    current_offset += sizeof(DoubleExtendedOperandInfo);
                    if (types[2] == CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2 || types[2] == CompactOperandType::MEM_BASE_OFF_IMM2) {
                        buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&infos[2].extendedInfo), sizeof(ExtendedOperandInfo));
                        current_offset += sizeof(ExtendedOperandInfo);
                    }
                    else {
                        buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&infos[2].info), sizeof(BasicOperandInfo));
                        current_offset += sizeof(BasicOperandInfo);
                    }
                }
                else {
                    EncodingError("Invalid combination of operand types", instruction);
                }

#undef CHECK_TYPES
#undef EXTENDED
#undef BASIC
            }
        } else if (arg_count > 3) {
            EncodingError("Too many arguments for instruction", instruction);
        }

        for (uint64_t l = 0; l < instruction->operands.getCount(); l++) {
            if (Operand* operand = instruction->operands.get(l); operand->type == OperandType::REGISTER) {
                Register* reg = static_cast<Register*>(operand->data);
                RegisterID reg_id = GetRegisterID(*reg, instruction);
                buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&reg_id), sizeof(RegisterID));
                current_offset += sizeof(RegisterID);
            } else if (operand->type == OperandType::MEMORY) {
                buffer.Write(current_offset, static_cast<uint8_t*>(operand->data), 8);
                current_offset += 8;
            } else if (operand->type == OperandType::IMMEDIATE) {
                switch (operand->size) {
                case OperandSize::BYTE:
                    buffer.Write(current_offset, static_cast<uint8_t*>(operand->data), 1);
                    current_offset += 1;
                    break;
                case OperandSize::WORD:
                    buffer.Write(current_offset, static_cast<uint8_t*>(operand->data), 2);
                    current_offset += 2;
                    break;
                case OperandSize::DWORD:
                    buffer.Write(current_offset, static_cast<uint8_t*>(operand->data), 4);
                    current_offset += 4;
                    break;
                case OperandSize::QWORD:
                    buffer.Write(current_offset, static_cast<uint8_t*>(operand->data), 8);
                    current_offset += 8;
                    break;
                default:
                    EncodingError("Invalid operand size", instruction);
                }
            } else if (operand->type == OperandType::COMPLEX) {
                ComplexData* complex = static_cast<ComplexData*>(operand->data);
                ComplexItem* items[3] = {&complex->base, &complex->index, &complex->offset};
                for (uint8_t i = 0; i < 3; i++) {
                    if (ComplexItem* item = items[i]; item->present) {
                        switch (item->type) {
                        case ComplexItem::Type::REGISTER: {
                            Register* reg = item->data.reg;
                            RegisterID reg_id = GetRegisterID(*reg, instruction);
                            uint8_t* reg_data = reinterpret_cast<uint8_t*>(&reg_id);
                            if (i == 2 && item->sign) // offset register with sign, need to set the highest bit
                                reg_data[0] |= 0x80;
                            buffer.Write(current_offset, reg_data, sizeof(RegisterID));
                            current_offset += sizeof(RegisterID);
                            break;
                        }
                        case ComplexItem::Type::IMMEDIATE: {
                            switch (item->data.imm.size) {
                            case OperandSize::BYTE:
                                buffer.Write(current_offset, static_cast<uint8_t*>(item->data.imm.data), 1);
                                current_offset += 1;
                                break;
                            case OperandSize::WORD:
                                buffer.Write(current_offset, static_cast<uint8_t*>(item->data.imm.data), 2);
                                current_offset += 2;
                                break;
                            case OperandSize::DWORD:
                                buffer.Write(current_offset, static_cast<uint8_t*>(item->data.imm.data), 4);
                                current_offset += 4;
                                break;
                            case OperandSize::QWORD:
                                buffer.Write(current_offset, static_cast<uint8_t*>(item->data.imm.data), 8);
                                current_offset += 8;
                                break;
                            default:
                                EncodingError("Invalid immediate size", instruction);
                            }
                            break;
                        }
                        case ComplexItem::Type::LABEL: {
                            Label* label = item->data.label;
                            Block::Jump* jump = new Block::Jump;
                            jump->offset = current_offset + global_offset;
                            jump->section = dataSection;
                            label->blocks.get(0)->jumpsToHere.insert(jump);
                            uint64_t temp_offset = 0xDEAD'BEEF'DEAD'BEEF;
                            buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&temp_offset), 8);
                            current_offset += 8;
                            break;
                        }
                        case ComplexItem::Type::SUBLABEL: {
                            Block* block = item->data.sublabel;
                            Block::Jump* jump = new Block::Jump;
                            jump->offset = current_offset + global_offset;
                            jump->section = dataSection;
                            block->jumpsToHere.insert(jump);
                            uint64_t temp_offset = 0xDEAD'BEEF'DEAD'BEEF;
                            buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&temp_offset), 8);
                            current_offset += 8;
                            break;
                        }
                        default:
                            EncodingError("Invalid complex item type", instruction);
                        }
                    }
                }
            } else if (operand->type == OperandType::LABEL) {
                Label* i_label = static_cast<Label*>(operand->data);
                Block* i_block = i_label->blocks.get(0);
                Block::Jump* jump = new Block::Jump;
                jump->offset = current_offset + global_offset;
                jump->section = dataSection;
                i_block->jumpsToHere.insert(jump);
                uint64_t temp_offset = 0xDEAD'BEEF'DEAD'BEEF;
                buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&temp_offset), 8);
                current_offset += 8;
            } else if (operand->type == OperandType::SUBLABEL) {
                Block* i_block = static_cast<Block*>(operand->data);
                Block::Jump* jump = new Block::Jump;
                jump->offset = current_offset + global_offset;
                jump->section = dataSection;
                i_block->jumpsToHere.insert(jump);
                uint64_t temp_offset = 0xDEAD'BEEF'DEAD'BEEF;
                buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&temp_offset), 8);
                current_offset += 8;
            } else {
                EncodingError("Invalid operand type", instruction);
            }
        }

        buffer.Read(0, data, data_size);
        if (current_offset > data_size)
            EncodingError("Data buffer overflow", instruction);
        return current_offset;
    }

} // namespace InsEncoding