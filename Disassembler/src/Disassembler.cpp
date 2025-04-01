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

#include "Disassembler.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <libarch/Instruction.hpp>
#include <libarch/Operand.hpp>

Disassembler::Disassembler(FileBuffer& buffer) : m_buffer(buffer), m_current_offset(0) {}
Disassembler::~Disassembler() {
}

void Disassembler::Disassemble(std::function<void()> errorCallback) {
    m_errorCallback = std::move(errorCallback);

    while (m_current_offset < m_buffer.GetSize()) {
        if (!InsEncoding::DecodeInstruction(m_buffer, m_current_offset, &m_current_instruction, [](const char* message, void* data){
            Disassembler* dis = static_cast<Disassembler*>(data);
            if (dis != nullptr)
                dis->error(message);
        }, this)) {
            break;
        }
        // Print the instruction
        std::stringstream ss;
        ss << GetInstructionName(m_current_instruction.GetOpcode());
        if (m_current_instruction.operand_count > 0)
            ss << " ";
        for (size_t i = 0; i < m_current_instruction.operand_count; i++) {
            StringifyOperand(m_current_instruction.operands[i], ss);
            if (i != m_current_instruction.operand_count - 1)
                ss << ", ";
        }
        m_instructions.push_back(ss.str());
    }
}

const std::vector<std::string>& Disassembler::GetInstructions() const {
    return m_instructions;
}

[[noreturn]] void Disassembler::error(const char* message) {
    fprintf(stderr, "Error: %s\n", message);
    m_errorCallback();
    exit(1);
}

void Disassembler::PrintCurrentInstruction() const {
    using namespace InsEncoding;
    FILE* fd = stdout;
    fprintf(fd, "Instruction: \"%s\":\n", GetInstructionName(m_current_instruction.GetOpcode()));
    for (size_t i = 0; i < m_current_instruction.operand_count; i++) {
        const Operand* operand = &(m_current_instruction.operands[i]);
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
            fprintf(fd, "Memory address: %#018lx\n", *static_cast<uint64_t*>(operand->data));
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
                        fprintf(fd, "size = 1, immediate = %#04hhx\n", *static_cast<uint8_t*>(complex_data->base.data.imm.data));
                        break;
                    case OperandSize::WORD:
                        fprintf(fd, "size = 2, immediate = %#06hx\n", *static_cast<uint16_t*>(complex_data->base.data.imm.data));
                        break;
                    case OperandSize::DWORD:
                        fprintf(fd, "size = 4, immediate = %#010x\n", *static_cast<uint32_t*>(complex_data->base.data.imm.data));
                        break;
                    case OperandSize::QWORD:
                        fprintf(fd, "size = 8, immediate = %#018lx\n", *static_cast<uint64_t*>(complex_data->base.data.imm.data));
                        break;
                    }
                    break;
                case ComplexItem::Type::REGISTER:
                    fprintf(fd, "Register: \"%s\"\n", GetRegisterName(*complex_data->base.data.reg));
                    break;
                case ComplexItem::Type::UNKNOWN:
                default:
                    break;
                }
            }
            if (complex_data->index.present) {
                fprintf(fd, "Index: ");
                switch (complex_data->index.type) {
                case ComplexItem::Type::IMMEDIATE:
                    switch (complex_data->index.data.imm.size) {
                    case OperandSize::BYTE:
                        fprintf(fd, "size = 1, immediate = %#04hhx\n", *static_cast<uint8_t*>(complex_data->index.data.imm.data));
                        break;
                    case OperandSize::WORD:
                        fprintf(fd, "size = 2, immediate = %#06hx\n", *static_cast<uint16_t*>(complex_data->index.data.imm.data));
                        break;
                    case OperandSize::DWORD:
                        fprintf(fd, "size = 4, immediate = %#010x\n", *static_cast<uint32_t*>(complex_data->index.data.imm.data));
                        break;
                    case OperandSize::QWORD:
                        fprintf(fd, "size = 8, immediate = %#018lx\n", *static_cast<uint64_t*>(complex_data->index.data.imm.data));
                        break;
                    }
                    break;
                case ComplexItem::Type::REGISTER:
                    fprintf(fd, "Register: \"%s\"\n", GetRegisterName(*complex_data->index.data.reg));
                    break;
                case ComplexItem::Type::UNKNOWN:
                default:
                    break;
                }
            }
            if (complex_data->offset.present) {
                fprintf(fd, "Offset: ");
                switch (complex_data->offset.type) {
                case ComplexItem::Type::IMMEDIATE:
                    switch (complex_data->offset.data.imm.size) {
                    case OperandSize::BYTE:
                        fprintf(fd, "size = 1, immediate = %#04hhx\n", *static_cast<uint8_t*>(complex_data->offset.data.imm.data));
                        break;
                    case OperandSize::WORD:
                        fprintf(fd, "size = 2, immediate = %#06hx\n", *static_cast<uint16_t*>(complex_data->offset.data.imm.data));
                        break;
                    case OperandSize::DWORD:
                        fprintf(fd, "size = 4, immediate = %#010x\n", *static_cast<uint32_t*>(complex_data->offset.data.imm.data));
                        break;
                    case OperandSize::QWORD:
                        fprintf(fd, "size = 8, immediate = %#018lx\n", *static_cast<uint64_t*>(complex_data->offset.data.imm.data));
                        break;
                    }
                    break;
                case ComplexItem::Type::REGISTER:
                    fprintf(fd, "Register: \"%s\", sign = %s\n", GetRegisterName(*complex_data->offset.data.reg), complex_data->offset.sign ? "positive" : "negative");
                    break;
                case ComplexItem::Type::UNKNOWN:
                default:
                    break;
                }
            }
            break;
        }
        case OperandType::IMMEDIATE:
            switch (operand->size) {
            case OperandSize::BYTE:
                fprintf(fd, "size = 1, immediate = %#04hhx\n", *static_cast<uint8_t*>(operand->data));
                break;
            case OperandSize::WORD:
                fprintf(fd, "size = 2, immediate = %#06hx\n", *static_cast<uint16_t*>(operand->data));
                break;
            case OperandSize::DWORD:
                fprintf(fd, "size = 4, immediate = %#010x\n", *static_cast<uint32_t*>(operand->data));
                break;
            case OperandSize::QWORD:
                fprintf(fd, "size = 8, immediate = %#018lx\n", *static_cast<uint64_t*>(operand->data));
                break;
            }
            break;
        default:
            fputs("unknown type\n", fd);
        }
    }
    printf("\n");
}

const char* Disassembler::GetInstructionName(InsEncoding::Opcode opcode) {
    using namespace InsEncoding;
#define NAME_CASE(ins, lower_ins) \
    case Opcode::ins:  \
        return #lower_ins;
    switch (opcode) {
        NAME_CASE(PUSH, push)
        NAME_CASE(POP, pop)
        NAME_CASE(PUSHA, pusha)
        NAME_CASE(POPA, popa)
        NAME_CASE(ADD, add)
        NAME_CASE(MUL, mul)
        NAME_CASE(SUB, sub)
        NAME_CASE(DIV, div)
        NAME_CASE(OR, or)
        NAME_CASE(XOR, xor)
        NAME_CASE(NOR, nor)
        NAME_CASE(AND, and)
        NAME_CASE(NAND, nand)
        NAME_CASE(NOT, not)
        NAME_CASE(CMP, cmp)
        NAME_CASE(INC, inc)
        NAME_CASE(DEC, dec)
        NAME_CASE(SHL, shl)
        NAME_CASE(SHR, shr)
        NAME_CASE(RET, ret)
        NAME_CASE(CALL, call)
        NAME_CASE(JMP, jmp)
        NAME_CASE(JC, jc)
        NAME_CASE(JNC, jnc)
        NAME_CASE(JZ, jz)
        NAME_CASE(JNZ, jnz)
        NAME_CASE(JL, jl)
        NAME_CASE(JLE, jle)
        NAME_CASE(JNL, jnl)
        NAME_CASE(JNLE, jnle)
        NAME_CASE(INT, int)
        NAME_CASE(LIDT, lidt)
        NAME_CASE(IRET, iret)
        NAME_CASE(MOV, mov)
        NAME_CASE(NOP, nop)
        NAME_CASE(HLT, hlt)
        NAME_CASE(SYSCALL, syscall)
        NAME_CASE(SYSRET, sysret)
        NAME_CASE(ENTERUSER, enteruser)
        NAME_CASE(UNKNOWN, unknown)
    }
#undef NAME_CASE
    return "UNKNOWN";
}

const char* Disassembler::GetRegisterName(InsEncoding::Register reg) {
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

void Disassembler::StringifyOperand(const InsEncoding::Operand& operand, std::stringstream& ss) {
    /*
    Operand formats:
    Register: <size> <register>
    Immediate: <immediate>
    Memory: <size> [<address>]
    Complex: <size> [<base> * <index> + <offset>]
    */
    using namespace InsEncoding;
    if (operand.type == OperandType::REGISTER || operand.type == OperandType::MEMORY || operand.type == OperandType::COMPLEX) {
        switch (operand.size) {
        case OperandSize::BYTE:
            ss << "BYTE ";
            break;
        case OperandSize::WORD:
            ss << "WORD ";
            break;
        case OperandSize::DWORD:
            ss << "DWORD ";
            break;
        case OperandSize::QWORD:
            if (operand.type != OperandType::REGISTER)
                ss << "QWORD ";
            break;
        }
    }

    if (operand.type == OperandType::MEMORY || operand.type == OperandType::COMPLEX)
        ss << "[";

    switch (operand.type) {
    case OperandType::REGISTER:
        ss << GetRegisterName(*static_cast<Register*>(operand.data));
        break;
    case OperandType::IMMEDIATE: {
        uint64_t imm_value = 0;
        switch (operand.size) {
        case OperandSize::BYTE:
            imm_value = *static_cast<uint8_t*>(operand.data);
            break;
        case OperandSize::WORD:
            imm_value = *static_cast<uint16_t*>(operand.data);
            break;
        case OperandSize::DWORD:
            imm_value = *static_cast<uint32_t*>(operand.data);
            break;
        case OperandSize::QWORD:
            imm_value = *static_cast<uint64_t*>(operand.data);
            break;
        }
        ss << "0x" << std::hex << imm_value;
        break;
    }
    case OperandType::MEMORY:
        ss << "0x" << std::hex << *static_cast<uint64_t*>(operand.data);
        break;
    case OperandType::COMPLEX: {
        ComplexData* complex_data = static_cast<ComplexData*>(operand.data);
        if (complex_data->base.present) {
            switch (complex_data->base.type) {
            case ComplexItem::Type::IMMEDIATE: {
                uint64_t imm_value = 0;
                switch (complex_data->base.data.imm.size) {
                case OperandSize::BYTE:
                    imm_value = *static_cast<uint8_t*>(complex_data->base.data.imm.data);
                    break;
                case OperandSize::WORD:
                    imm_value = *static_cast<uint16_t*>(complex_data->base.data.imm.data);
                    break;
                case OperandSize::DWORD:
                    imm_value = *static_cast<uint32_t*>(complex_data->base.data.imm.data);
                    break;
                case OperandSize::QWORD:
                    imm_value = *static_cast<uint64_t*>(complex_data->base.data.imm.data);
                    break;
                }
                ss << "0x" << std::hex << imm_value;
                break;
            }
            case ComplexItem::Type::REGISTER:
                ss << GetRegisterName(*complex_data->base.data.reg);
                break;
            default:
                break;
            }
        }
        if (complex_data->index.present) {
            ss << " * ";
            switch (complex_data->index.type) {
            case ComplexItem::Type::IMMEDIATE: {
                uint64_t imm_value = 0;
                switch (complex_data->index.data.imm.size) {
                case OperandSize::BYTE:
                    imm_value = *static_cast<uint8_t*>(complex_data->index.data.imm.data);
                    break;
                case OperandSize::WORD:
                    imm_value = *static_cast<uint16_t*>(complex_data->index.data.imm.data);
                    break;
                case OperandSize::DWORD:
                    imm_value = *static_cast<uint32_t*>(complex_data->index.data.imm.data);
                    break;
                case OperandSize::QWORD:
                    imm_value = *static_cast<uint64_t*>(complex_data->index.data.imm.data);
                    break;
                }
                ss << "0x" << std::hex << imm_value;
                break;
            }
            case ComplexItem::Type::REGISTER:
                ss << GetRegisterName(*complex_data->index.data.reg);
                break;
            default:
                break;
            }
        }
        if (complex_data->offset.present) {
            ss << " + ";
            switch (complex_data->offset.type) {
            case ComplexItem::Type::IMMEDIATE: {
                uint64_t imm_value = 0;
                switch (complex_data->offset.data.imm.size) {
                case OperandSize::BYTE:
                    imm_value = *static_cast<uint8_t*>(complex_data->offset.data.imm.data);
                    break;
                case OperandSize::WORD:
                    imm_value = *static_cast<uint16_t*>(complex_data->offset.data.imm.data);
                    break;
                case OperandSize::DWORD:
                    imm_value = *static_cast<uint32_t*>(complex_data->offset.data.imm.data);
                    break;
                case OperandSize::QWORD:
                    imm_value = *static_cast<uint64_t*>(complex_data->offset.data.imm.data);
                    break;
                }
                ss << "0x" << std::hex << imm_value;
                break;
            }
            case ComplexItem::Type::REGISTER:
                ss << GetRegisterName(*complex_data->offset.data.reg);
                break;
            default:
                break;
            }
        }
        break;
    }
    default:
        break;
    }

    if (operand.type == OperandType::MEMORY || operand.type == OperandType::COMPLEX)
        ss << "]";
}