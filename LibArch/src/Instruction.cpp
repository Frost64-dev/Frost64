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

    SimpleInstruction g_currentInstruction;
    ComplexData g_complexData[2];
    Register g_currentRegisters[6]; // maximum of 6 registers in an instruction
    uint64_t g_rawData[48]; // maximum of 6 in an operand, maximum of 8 bytes each, totalling 48 bytes

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

    Register GetRegisterFromID(RegisterID reg_id, void (*errorHandler)(const char* message, void* data), void* data) {
        Register reg;
        switch (reg_id.type) {
#define REG_CASE(name, num)   \
    case num:                 \
        reg = Register::name; \
        break;
        case 0:
            switch (reg_id.number) {
                REG_CASE(r0, 0)
                REG_CASE(r1, 1)
                REG_CASE(r2, 2)
                REG_CASE(r3, 3)
                REG_CASE(r4, 4)
                REG_CASE(r5, 5)
                REG_CASE(r6, 6)
                REG_CASE(r7, 7)
                REG_CASE(r8, 8)
                REG_CASE(r9, 9)
                REG_CASE(r10, 10)
                REG_CASE(r11, 11)
                REG_CASE(r12, 12)
                REG_CASE(r13, 13)
                REG_CASE(r14, 14)
                REG_CASE(r15, 15)
            default:
                errorHandler("Invalid register number", data);
                __builtin_unreachable();
            }
            break;
        case 1:
            switch (reg_id.number) {
                REG_CASE(scp, 0)
                REG_CASE(sbp, 1)
                REG_CASE(stp, 2)
            default:
                errorHandler("Invalid register number", data);
                __builtin_unreachable();
            }
            break;
        case 2:
            switch (reg_id.number) {
                REG_CASE(cr0, 0)
                REG_CASE(cr1, 1)
                REG_CASE(cr2, 2)
                REG_CASE(cr3, 3)
                REG_CASE(cr4, 4)
                REG_CASE(cr5, 5)
                REG_CASE(cr6, 6)
                REG_CASE(cr7, 7)
                REG_CASE(sts, 8)
                REG_CASE(ip, 9)
            default:
                errorHandler("Invalid register number", data);
                __builtin_unreachable();
            }
            break;
        default:
            errorHandler("Invalid register type", data);
            __builtin_unreachable();
        }
#undef REG_CASE
        return reg;
    }

    uint8_t GetArgCountForOpcode(Opcode opcode) {
        switch (opcode) {
        case Opcode::ADD:
        case Opcode::MUL:
        case Opcode::SUB:
        case Opcode::DIV:
        case Opcode::OR:
        case Opcode::XOR:
        case Opcode::NOR:
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

    bool DecodeInstruction(StreamBuffer& buffer, uint64_t& current_offset, SimpleInstruction* out, void (*error_handler)(const char* message, void* data), void* error_data) {
        if (out == nullptr)
            return false;

        uint8_t raw_opcode;
        buffer.ReadStream8(raw_opcode);
        current_offset++;

        g_currentInstruction.SetOpcode(static_cast<Opcode>(raw_opcode));
        g_currentInstruction.operandCount = 0;

        uint8_t arg_count = GetArgCountForOpcode(g_currentInstruction.GetOpcode());
        if (arg_count == 0) {
            *out = g_currentInstruction;
            return true;
        }

        OperandType operand_types[2];
        OperandSize operand_sizes[2];

        ComplexOperandInfo complex_infos[2];

        if (arg_count == 1) {
            // read 1 byte to get the operand type
            uint8_t raw_operand_info;
            buffer.ReadStream8(raw_operand_info);
            current_offset++;
            StandardOperandInfo* temp_operand_info = reinterpret_cast<StandardOperandInfo*>(&raw_operand_info);

            operand_types[0] = static_cast<OperandType>(temp_operand_info->type);
            operand_sizes[0] = static_cast<OperandSize>(temp_operand_info->size);

            if (operand_types[0] == OperandType::COMPLEX) {
                ComplexOperandInfo complex_info{};
                uint8_t* raw_complex_info = reinterpret_cast<uint8_t*>(&complex_info);
                raw_complex_info[0] = raw_operand_info;
                buffer.ReadStream8(raw_complex_info[1]);
                current_offset += sizeof(ComplexOperandInfo) - 1;
                complex_infos[0] = complex_info;
            }
        } else if (arg_count == 2) {
            // read 1 byte to get the operand types
            uint8_t raw_operand_info;
            buffer.ReadStream8(raw_operand_info);
            current_offset++;

            if (StandardOperandInfo* temp_operand_info = reinterpret_cast<StandardOperandInfo*>(&raw_operand_info); temp_operand_info->type == static_cast<uint8_t>(OperandType::COMPLEX)) {
                ComplexStandardOperandInfo temp_complex_info{};
                uint8_t* raw_complex_info = reinterpret_cast<uint8_t*>(&temp_complex_info);
                raw_complex_info[0] = raw_operand_info;
                buffer.ReadStream16(reinterpret_cast<uint16_t&>(raw_complex_info[1]));
                current_offset += sizeof(ComplexStandardOperandInfo) - 1;
                operand_types[0] = static_cast<OperandType>(temp_complex_info.complex.type);
                operand_sizes[0] = static_cast<OperandSize>(temp_complex_info.complex.size);
                operand_types[1] = static_cast<OperandType>(temp_complex_info.standard.type);
                operand_sizes[1] = static_cast<OperandSize>(temp_complex_info.standard.size);

                complex_infos[0] = temp_complex_info.complex;

                if (operand_types[1] == OperandType::COMPLEX) {
                    ComplexComplexOperandInfo i_temp_complex_info{};
                    uint8_t* i_raw_complex_info = reinterpret_cast<uint8_t*>(&i_temp_complex_info);
                    i_raw_complex_info[0] = raw_complex_info[0];
                    i_raw_complex_info[1] = raw_complex_info[1];
                    i_raw_complex_info[2] = raw_complex_info[2];
                    buffer.ReadStream8(i_raw_complex_info[3]);
                    current_offset += sizeof(ComplexComplexOperandInfo) - sizeof(ComplexStandardOperandInfo);
                    complex_infos[1] = i_temp_complex_info.second;
                }
            } else {
                StandardStandardOperandInfo* temp_standard_info = reinterpret_cast<StandardStandardOperandInfo*>(&raw_operand_info);
                operand_types[0] = static_cast<OperandType>(temp_standard_info->firstType);
                operand_sizes[0] = static_cast<OperandSize>(temp_standard_info->firstSize);
                operand_types[1] = static_cast<OperandType>(temp_standard_info->secondType);
                operand_sizes[1] = static_cast<OperandSize>(temp_standard_info->secondSize);

                if (operand_types[1] == OperandType::COMPLEX) {
                    StandardComplexOperandInfo temp_complex_info{};
                    uint8_t* raw_complex_info = reinterpret_cast<uint8_t*>(&temp_complex_info);
                    raw_complex_info[0] = raw_operand_info;
                    buffer.ReadStream16(reinterpret_cast<uint16_t&>(raw_complex_info[1]));
                    current_offset += sizeof(StandardComplexOperandInfo) - 1;
                    complex_infos[1] = temp_complex_info.complex;
                    operand_sizes[1] = static_cast<OperandSize>(temp_complex_info.complex.size);
                }
            }
        } else
            error_handler("Invalid argument count", error_data);

        for (uint8_t i = 0; i < arg_count; i++) {
            OperandType operand_type = operand_types[i];
            OperandSize operand_size = operand_sizes[i];

            Operand operand(operand_type, operand_size, nullptr);

            switch (operand_type) {
            case OperandType::COMPLEX: {
                ComplexOperandInfo complex_info = complex_infos[i];
                ComplexData* complex = &g_complexData[i];
                complex->base.present = complex_info.basePresent;
                if (complex->base.present) {
                    complex->base.type = complex_info.baseType == 0 ? ComplexItem::Type::REGISTER : ComplexItem::Type::IMMEDIATE;
                    if (complex->base.type == ComplexItem::Type::IMMEDIATE) {
                        complex->base.data.imm.size = static_cast<OperandSize>(complex_info.baseSize);
                        complex->base.data.imm.data = &g_rawData[i * 3];
                        switch (complex_info.baseSize) {
#define SIZE_CASE(bytes, bits) \
        case bytes: \
            buffer.ReadStream##bits(*reinterpret_cast<uint##bits##_t*>(complex->base.data.imm.data)); \
            break
                        SIZE_CASE(0, 8);
                        SIZE_CASE(1, 16);
                        SIZE_CASE(2, 32);
                        SIZE_CASE(3, 64);
#undef SIZE_CASE
                        }
                        current_offset += 1 << complex_info.baseSize;
                    } else if (complex->base.type == ComplexItem::Type::REGISTER) {
                        RegisterID reg_id{};
                        buffer.ReadStream8(reinterpret_cast<uint8_t&>(reg_id));
                        current_offset += sizeof(RegisterID);
                        complex->base.data.reg = &g_currentRegisters[i * 3];
                        *complex->base.data.reg = GetRegisterFromID(reg_id, error_handler, error_data);
                    }
                }
                complex->index.present = complex_info.indexPresent;
                if (complex->index.present) {
                    complex->index.type = complex_info.indexType == 0 ? ComplexItem::Type::REGISTER : ComplexItem::Type::IMMEDIATE;
                    if (complex->index.type == ComplexItem::Type::IMMEDIATE) {
                        complex->index.data.imm.size = static_cast<OperandSize>(complex_info.indexSize);
                        complex->index.data.imm.data = &g_rawData[i * 3 + 1];
                        switch (complex_info.indexSize) {
#define SIZE_CASE(bytes, bits) \
        case bytes: \
            buffer.ReadStream##bits(*reinterpret_cast<uint##bits##_t*>(complex->index.data.imm.data)); \
            break
                        SIZE_CASE(0, 8);
                        SIZE_CASE(1, 16);
                        SIZE_CASE(2, 32);
                        SIZE_CASE(3, 64);
#undef SIZE_CASE
                        }
                        current_offset += 1 << complex_info.indexSize;
                    } else if (complex->index.type == ComplexItem::Type::REGISTER) {
                        RegisterID reg_id{};
                        buffer.ReadStream8(reinterpret_cast<uint8_t&>(reg_id));
                        current_offset += sizeof(RegisterID);
                        complex->index.data.reg = &g_currentRegisters[i * 3 + 1];
                        *complex->index.data.reg = GetRegisterFromID(reg_id, error_handler, error_data);
                    }
                }
                complex->offset.present = complex_info.offsetPresent;
                if (complex->offset.present) {
                    complex->offset.type = complex_info.offsetType == 0 ? ComplexItem::Type::REGISTER : ComplexItem::Type::IMMEDIATE;
                    if (complex->offset.type == ComplexItem::Type::IMMEDIATE) {
                        complex->offset.data.imm.size = static_cast<OperandSize>(complex_info.offsetSize);
                        complex->offset.data.imm.data = &g_rawData[i * 3 + 2];
                        switch (complex_info.offsetSize) {
#define SIZE_CASE(bytes, bits) \
        case bytes: \
            buffer.ReadStream##bits(*reinterpret_cast<uint##bits##_t*>(complex->offset.data.imm.data)); \
            break
                        SIZE_CASE(0, 8);
                        SIZE_CASE(1, 16);
                        SIZE_CASE(2, 32);
                        SIZE_CASE(3, 64);
#undef SIZE_CASE
                        }
                        current_offset += 1 << complex_info.offsetSize;
                    } else if (complex->offset.type == ComplexItem::Type::REGISTER) {
                        RegisterID reg_id{};
                        buffer.ReadStream8(reinterpret_cast<uint8_t&>(reg_id));
                        current_offset += sizeof(RegisterID);
                        complex->offset.data.reg = &g_currentRegisters[i * 3 + 2];
                        *complex->offset.data.reg = GetRegisterFromID(reg_id, error_handler, error_data);
                        complex->offset.sign = complex_info.offsetSize;
                    }
                }
                operand.data = complex;
                break;
            }
            case OperandType::REGISTER: {
                RegisterID reg_id{};
                buffer.ReadStream8(reinterpret_cast<uint8_t&>(reg_id));
                current_offset += sizeof(RegisterID);
                Register* reg = &g_currentRegisters[i * 3];
                *reg = GetRegisterFromID(reg_id, error_handler, error_data);
                operand.data = reg;
                break;
            }
            case OperandType::MEMORY: {
                uint8_t* mem_data = reinterpret_cast<uint8_t*>(&g_rawData[i * 3]);
                buffer.ReadStream64(*reinterpret_cast<uint64_t*>(mem_data));
                current_offset += 8;
                operand.data = mem_data;
                break;
            }
            case OperandType::IMMEDIATE: {
                uint8_t* imm_data = reinterpret_cast<uint8_t*>(&g_rawData[i * 3]);
                switch (static_cast<uint8_t>(operand_size)) {
#define SIZE_CASE(bytes, bits) \
        case bytes: \
            buffer.ReadStream##bits(*reinterpret_cast<uint##bits##_t*>(imm_data)); \
            break
                SIZE_CASE(0, 8);
                SIZE_CASE(1, 16);
                SIZE_CASE(2, 32);
                SIZE_CASE(3, 64);
#undef SIZE_CASE
                }
                current_offset += 1 << static_cast<uint8_t>(operand_size);
                operand.data = imm_data;
                break;
            }
            default:
                error_handler("Invalid operand type", error_data);
            }

            g_currentInstruction.operands[g_currentInstruction.operandCount] = operand;
            g_currentInstruction.operandCount++;
        }

        *out = g_currentInstruction;
        return true;
    }

#pragma GCC diagnostic pop

    size_t EncodeInstruction(Instruction* instruction, uint8_t* data, size_t data_size, uint64_t global_offset) {
        Buffer buffer;
        uint64_t current_offset = 0;

        if (instruction->operands.getCount() > 2)
            EncodingError("Instruction has more than 2 operands", instruction);

        uint8_t arg_count = GetArgCountForOpcode(instruction->GetOpcode());
        if (instruction->operands.getCount() != arg_count)
            EncodingError("Invalid number of arguments for instruction", instruction);

        uint8_t opcode = static_cast<uint8_t>(instruction->GetOpcode());

        buffer.Write(current_offset, &opcode, 1);
        current_offset++;

        Operand* operands[2] = {nullptr, nullptr};
        for (uint64_t l = 0; l < instruction->operands.getCount(); l++)
            operands[l] = instruction->operands.get(l);

        if (arg_count == 1) {
            if (operands[0]->type == OperandType::COMPLEX) {
                ComplexData* complex = static_cast<ComplexData*>(operands[0]->data);
                ComplexOperandInfo info{};
                info.type = static_cast<uint8_t>(OperandType::COMPLEX);
                info.size = static_cast<uint8_t>(operands[0]->size);
                info.baseType = complex->base.type == ComplexItem::Type::REGISTER ? 0 : 1;
                info.baseSize = complex->base.type == ComplexItem::Type::REGISTER ? 0 : ((complex->base.type == ComplexItem::Type::LABEL || complex->base.type == ComplexItem::Type::SUBLABEL) ? 3 : static_cast<uint8_t>(complex->base.data.imm.size));
                info.basePresent = complex->base.present;
                info.indexType = complex->index.type == ComplexItem::Type::REGISTER ? 0 : 1;
                info.indexSize = complex->index.type == ComplexItem::Type::REGISTER ? 0 : ((complex->index.type == ComplexItem::Type::LABEL || complex->index.type == ComplexItem::Type::SUBLABEL) ? 3 : static_cast<uint8_t>(complex->index.data.imm.size));
                info.indexPresent = complex->index.present;
                info.offsetType = complex->offset.type == ComplexItem::Type::REGISTER ? 0 : 1;
                info.offsetSize = complex->offset.type == ComplexItem::Type::REGISTER ? complex->offset.sign : ((complex->offset.type == ComplexItem::Type::LABEL || complex->offset.type == ComplexItem::Type::SUBLABEL) ? 3 : static_cast<uint8_t>(complex->offset.data.imm.size));
                info.offsetPresent = complex->offset.present;
                buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&info), sizeof(ComplexOperandInfo));
                current_offset += sizeof(ComplexOperandInfo);
            } else {
                StandardOperandInfo info{};
                if (operands[0]->type == OperandType::LABEL || operands[0]->type == OperandType::SUBLABEL) {
                    info.type = static_cast<uint8_t>(OperandType::IMMEDIATE);
                    info.size = 3;
                } else {
                    info.type = static_cast<uint8_t>(operands[0]->type);
                    info.size = static_cast<uint8_t>(operands[0]->size);
                }
                info._padding = 0;
                buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&info), sizeof(StandardOperandInfo));
                current_offset += sizeof(StandardOperandInfo);
            }
        } else if (arg_count == 2) {
            if ((operands[0]->type == OperandType::COMPLEX && operands[1]->type != OperandType::COMPLEX) || (operands[1]->type == OperandType::COMPLEX && operands[0]->type != OperandType::COMPLEX)) {
                // start with a StandardComplexOperandInfo, and swap at the end if needed
                StandardComplexOperandInfo info{};
                {
                    if (Operand* temp = operands[operands[0]->type != OperandType::COMPLEX ? 0 : 1]; temp->type == OperandType::LABEL || temp->type == OperandType::SUBLABEL) {
                        info.standard.type = static_cast<uint8_t>(OperandType::IMMEDIATE);
                        info.standard.size = 3;
                    } else {
                        info.standard.type = static_cast<uint8_t>(temp->type);
                        info.standard.size = static_cast<uint8_t>(temp->size);
                    }
                }
                info.standard._padding = 0;
                ComplexData* complex = static_cast<ComplexData*>(operands[operands[0]->type == OperandType::COMPLEX ? 0 : 1]->data);
                info.complex.type = static_cast<uint8_t>(OperandType::COMPLEX);
                info.complex.size = static_cast<uint8_t>(operands[operands[0]->type == OperandType::COMPLEX ? 0 : 1]->size);
                info.complex.baseType = complex->base.type == ComplexItem::Type::REGISTER ? 0 : 1;
                info.complex.baseSize = complex->base.type == ComplexItem::Type::REGISTER ? 0 : ((complex->base.type == ComplexItem::Type::LABEL || complex->base.type == ComplexItem::Type::SUBLABEL) ? 3 : static_cast<uint8_t>(complex->base.data.imm.size));
                info.complex.basePresent = complex->base.present;
                info.complex.indexType = complex->index.type == ComplexItem::Type::REGISTER ? 0 : 1;
                info.complex.indexSize = complex->index.type == ComplexItem::Type::REGISTER ? 0 : ((complex->index.type == ComplexItem::Type::LABEL || complex->index.type == ComplexItem::Type::SUBLABEL) ? 3 : static_cast<uint8_t>(complex->index.data.imm.size));
                info.complex.indexPresent = complex->index.present;
                info.complex.offsetType = complex->offset.type == ComplexItem::Type::REGISTER ? 0 : 1;
                info.complex.offsetSize = complex->offset.type == ComplexItem::Type::REGISTER ? complex->offset.sign : ((complex->offset.type == ComplexItem::Type::LABEL || complex->offset.type == ComplexItem::Type::SUBLABEL) ? 3 : static_cast<uint8_t>(complex->offset.data.imm.size));
                info.complex.offsetPresent = complex->offset.present;
                if (operands[0]->type == OperandType::COMPLEX) {
                    ComplexStandardOperandInfo temp{};
                    temp.complex = info.complex;
                    temp.standard = info.standard;
                    buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&temp), sizeof(ComplexStandardOperandInfo));
                    current_offset += sizeof(ComplexStandardOperandInfo);
                } else {
                    info.standard._padding = info.complex.type;
                    buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&info), sizeof(StandardComplexOperandInfo));
                    current_offset += sizeof(StandardComplexOperandInfo);
                }
            } else if (operands[0]->type == OperandType::COMPLEX && operands[1]->type == OperandType::COMPLEX) {
                ComplexComplexOperandInfo info{};
                ComplexData* complex0 = static_cast<ComplexData*>(operands[0]->data);
                ComplexData* complex1 = static_cast<ComplexData*>(operands[1]->data);
                info.first.type = static_cast<uint8_t>(OperandType::COMPLEX);
                info.first.size = static_cast<uint8_t>(operands[0]->size);
                info.first.baseType = complex0->base.type == ComplexItem::Type::REGISTER ? 0 : 1;
                info.first.baseSize = complex0->base.type == ComplexItem::Type::REGISTER ? 0 : ((complex0->base.type == ComplexItem::Type::LABEL || complex0->base.type == ComplexItem::Type::SUBLABEL) ? 3 : static_cast<uint8_t>(complex0->base.data.imm.size));
                info.first.basePresent = complex0->base.present;
                info.first.indexType = complex0->index.type == ComplexItem::Type::REGISTER ? 0 : 1;
                info.first.indexSize = complex0->index.type == ComplexItem::Type::REGISTER ? 0 : ((complex0->index.type == ComplexItem::Type::LABEL || complex0->index.type == ComplexItem::Type::SUBLABEL) ? 3 : static_cast<uint8_t>(complex0->index.data.imm.size));
                info.first.indexPresent = complex0->index.present;
                info.first.offsetType = complex0->offset.type == ComplexItem::Type::REGISTER ? 0 : 1;
                info.first.offsetSize = complex0->offset.type == ComplexItem::Type::REGISTER ? complex0->offset.sign : ((complex0->offset.type == ComplexItem::Type::LABEL || complex0->offset.type == ComplexItem::Type::SUBLABEL) ? 3 : static_cast<uint8_t>(complex0->offset.data.imm.size));
                info.first.offsetPresent = complex0->offset.present;
                info.second.type = static_cast<uint8_t>(OperandType::COMPLEX);
                info.second.size = static_cast<uint8_t>(operands[1]->size);
                info.second.baseType = complex1->base.type == ComplexItem::Type::REGISTER ? 0 : 1;
                info.second.baseSize = complex1->base.type == ComplexItem::Type::REGISTER ? 0 : ((complex1->base.type == ComplexItem::Type::LABEL || complex1->base.type == ComplexItem::Type::SUBLABEL) ? 3 : static_cast<uint8_t>(complex1->base.data.imm.size));
                info.second.basePresent = complex1->base.present;
                info.second.indexType = complex1->index.type == ComplexItem::Type::REGISTER ? 0 : 1;
                info.second.indexSize = complex1->index.type == ComplexItem::Type::REGISTER ? 0 : ((complex1->index.type == ComplexItem::Type::LABEL || complex1->index.type == ComplexItem::Type::SUBLABEL) ? 3 : static_cast<uint8_t>(complex1->index.data.imm.size));
                info.second.indexPresent = complex1->index.present;
                info.second.offsetType = complex1->offset.type == ComplexItem::Type::REGISTER ? 0 : 1;
                info.second.offsetSize = complex1->offset.type == ComplexItem::Type::REGISTER ? complex1->offset.sign : ((complex1->offset.type == ComplexItem::Type::LABEL || complex1->offset.type == ComplexItem::Type::SUBLABEL) ? 3 : static_cast<uint8_t>(complex1->offset.data.imm.size));
                info.second.offsetPresent = complex1->offset.present;
                buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&info), sizeof(ComplexComplexOperandInfo));
                current_offset += sizeof(ComplexComplexOperandInfo);
            } else if (operands[0]->type != OperandType::COMPLEX && operands[1]->type != OperandType::COMPLEX) {
                StandardStandardOperandInfo info{};
                if (operands[0]->type == OperandType::LABEL || operands[0]->type == OperandType::SUBLABEL) {
                    info.firstType = static_cast<uint8_t>(OperandType::IMMEDIATE);
                    info.firstSize = 3;
                } else {
                    info.firstType = static_cast<uint8_t>(operands[0]->type);
                    info.firstSize = static_cast<uint8_t>(operands[0]->size);
                }
                if (operands[1]->type == OperandType::LABEL || operands[1]->type == OperandType::SUBLABEL) {
                    info.secondType = static_cast<uint8_t>(OperandType::IMMEDIATE);
                    info.secondSize = 3;
                } else {
                    info.secondType = static_cast<uint8_t>(operands[1]->type);
                    info.secondSize = static_cast<uint8_t>(operands[1]->size);
                }
                buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&info), sizeof(StandardStandardOperandInfo));
                current_offset += sizeof(StandardStandardOperandInfo);
            } else
                EncodingError("Invalid operand combination", instruction);
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
                            buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&reg_id), sizeof(RegisterID));
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
                            uint64_t* offset = new uint64_t;
                            *offset = current_offset + global_offset;
                            label->blocks.get(0)->jumpsToHere.insert(offset);
                            uint64_t temp_offset = 0xDEAD'BEEF'DEAD'BEEF;
                            buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&temp_offset), 8);
                            current_offset += 8;
                            break;
                        }
                        case ComplexItem::Type::SUBLABEL: {
                            Block* block = item->data.sublabel;
                            uint64_t* offset = new uint64_t;
                            *offset = current_offset + global_offset;
                            block->jumpsToHere.insert(offset);
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
                uint64_t* offset = new uint64_t;
                *offset = current_offset + global_offset;
                i_block->jumpsToHere.insert(offset);
                uint64_t temp_offset = 0xDEAD'BEEF'DEAD'BEEF;
                buffer.Write(current_offset, reinterpret_cast<uint8_t*>(&temp_offset), 8);
                current_offset += 8;
            } else if (operand->type == OperandType::SUBLABEL) {
                Block* i_block = static_cast<Block*>(operand->data);
                uint64_t* offset = new uint64_t;
                *offset = current_offset + global_offset;
                i_block->jumpsToHere.insert(offset);
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