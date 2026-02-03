/*
Copyright (©) 2023-2026  Frosty515

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

#include "Operand.hpp"

#include <stdarg.h>
#include <stdio.h>

#include <Emulator.hpp>
#include <Exceptions.hpp>

Operand::Operand()
    : m_cpu(nullptr), m_type(OperandType::Register), m_size(OperandSize::Unknown) {
}

Operand::Operand(Emulator::CPUState* cpu, OperandSize size, OperandType type)
    : m_cpu(cpu), m_type(type), m_size(size) {

}

Operand::~Operand() {
}

OperandType Operand::GetType() const {
    return m_type;
}

OperandSize Operand::GetSize() const {
    return m_size;
}

RegisterOperand::RegisterOperand() : m_register(nullptr) {

}

RegisterOperand::RegisterOperand(Emulator::CPUState* cpu, OperandSize size, Register* reg) : Operand(cpu, size, OperandType::Register), m_register(reg) {

}

uint64_t RegisterOperand::GetValue() const {
    return m_register->GetValue(m_size);
}

void RegisterOperand::SetValue(uint64_t value) {
    if (!m_register->SetValue(value, m_size))
        m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
}

void RegisterOperand::PrintInfo() const {
    char const* size;
    switch (m_size) {
    case OperandSize::BYTE:
        size = "BYTE";
        break;
    case OperandSize::WORD:
        size = "WORD";
        break;
    case OperandSize::DWORD:
        size = "DWORD";
        break;
    case OperandSize::QWORD:
        size = "QWORD";
        break;
    default:
        size = "Unknown";
        break;
    }
    printf("Register: %s, size = %s", m_register->GetName(), size);
}

ImmediateOperand::ImmediateOperand() : m_immediate(0) {

}

ImmediateOperand::ImmediateOperand(Emulator::CPUState* cpu, OperandSize size, uint64_t immediate) : Operand(cpu, size, OperandType::Immediate), m_immediate(immediate) {

}

uint64_t ImmediateOperand::GetValue() const {
    return m_immediate;
}

void ImmediateOperand::SetValue(uint64_t) {
    m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
}

void ImmediateOperand::PrintInfo() const {
    char const* size;
    switch (m_size) {
    case OperandSize::BYTE:
        size = "BYTE";
        break;
    case OperandSize::WORD:
        size = "WORD";
        break;
    case OperandSize::DWORD:
        size = "DWORD";
        break;
    case OperandSize::QWORD:
        size = "QWORD";
        break;
    default:
        size = "Unknown";
        break;
    }
    printf("Immediate: value = 0x%lx, size = %s", m_immediate, size);
}

MemoryOperand::MemoryOperand() : m_address(0), m_mmu(nullptr) {

}

MemoryOperand::MemoryOperand(Emulator::CPUState* cpu, OperandSize size, uint64_t address, MMU* mmu) : Operand(cpu, size, OperandType::Memory), m_address(address), m_mmu(mmu) {

}

uint64_t MemoryOperand::GetValue() const {
    uint64_t value = 0;
    switch (m_size) {
    case OperandSize::BYTE:
        value = m_mmu->read8(m_address);
        break;
    case OperandSize::WORD:
        value = m_mmu->read16(m_address);
        break;
    case OperandSize::DWORD:
        value = m_mmu->read32(m_address);
        break;
    case OperandSize::QWORD:
        value = m_mmu->read64(m_address);
        break;
    default:
        m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
    }
    return value;
}

void MemoryOperand::SetValue(uint64_t value) {
    switch (m_size) {
    case OperandSize::BYTE:
        m_mmu->write8(m_address, static_cast<uint8_t>(value));
        break;
    case OperandSize::WORD:
        m_mmu->write16(m_address, static_cast<uint16_t>(value));
        break;
    case OperandSize::DWORD:
        m_mmu->write32(m_address, static_cast<uint32_t>(value));
        break;
    case OperandSize::QWORD:
        m_mmu->write64(m_address, value);
        break;
    default:
        m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
    }
}

void MemoryOperand::PrintInfo() const {
    char const* size;
    switch (m_size) {
    case OperandSize::BYTE:
        size = "BYTE";
        break;
    case OperandSize::WORD:
        size = "WORD";
        break;
    case OperandSize::DWORD:
        size = "DWORD";
        break;
    case OperandSize::QWORD:
        size = "QWORD";
        break;
    default:
        size = "Unknown";
        break;
    }
    printf("Memory: %#016lx, size = %s", m_address, size);
}

ComplexOperand::ComplexOperand() : m_complexData(nullptr), m_mmu(nullptr) {

}

ComplexOperand::ComplexOperand(Emulator::CPUState* cpu, OperandSize size, ComplexData* complexData, MMU* mmu) : Operand(cpu, size, OperandType::Complex), m_complexData(complexData), m_mmu(mmu) {

}

uint64_t ComplexOperand::GetValue() const {
    int64_t base = 0;
    if (m_complexData->base.present) {
        if (m_complexData->base.type == ComplexItem::Type::REGISTER)
            base = m_complexData->base.data.reg->GetValue();
        else {
            switch (m_complexData->base.data.imm.size) {
            case OperandSize::BYTE:
                base = *static_cast<int8_t*>(m_complexData->base.data.imm.data);
                break;
            case OperandSize::WORD:
                base = *static_cast<int16_t*>(m_complexData->base.data.imm.data);
                break;
            case OperandSize::DWORD:
                base = *static_cast<int32_t*>(m_complexData->base.data.imm.data);
                break;
            case OperandSize::QWORD:
                base = *static_cast<int64_t*>(m_complexData->base.data.imm.data);
                break;
            default:
                m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            }
        }
    }
    int64_t index = 0;
    if (m_complexData->index.present) {
        if (!m_complexData->base.present)
            base = 1;
        if (m_complexData->index.type == ComplexItem::Type::REGISTER)
            index = m_complexData->index.data.reg->GetValue();
        else {
            switch (m_complexData->index.data.imm.size) {
            case OperandSize::BYTE:
                index = *static_cast<int8_t*>(m_complexData->index.data.imm.data);
                break;
            case OperandSize::WORD:
                index = *static_cast<int16_t*>(m_complexData->index.data.imm.data);
                break;
            case OperandSize::DWORD:
                index = *static_cast<int32_t*>(m_complexData->index.data.imm.data);
                break;
            case OperandSize::QWORD:
                index = *static_cast<int64_t*>(m_complexData->index.data.imm.data);
                break;
            default:
                m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            }
        }
    } else if (m_complexData->base.present)
        index = 1;
    int64_t offset = 0;
    if (m_complexData->offset.present) {
        if (m_complexData->offset.type == ComplexItem::Type::REGISTER) {
            offset = m_complexData->offset.data.reg->GetValue();
            if (!m_complexData->offset.sign)
                offset = -offset;
        } else {
            switch (m_complexData->offset.data.imm.size) {
            case OperandSize::BYTE:
                offset = *static_cast<int8_t*>(m_complexData->offset.data.imm.data);
                break;
            case OperandSize::WORD:
                offset = *static_cast<int16_t*>(m_complexData->offset.data.imm.data);
                break;
            case OperandSize::DWORD:
                offset = *static_cast<int32_t*>(m_complexData->offset.data.imm.data);
                break;
            case OperandSize::QWORD:
                offset = *static_cast<int64_t*>(m_complexData->offset.data.imm.data);
                break;
            default:
                m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            }
        }
    }
    uint64_t value = 0;
    switch (m_size) {
    case OperandSize::BYTE:
        value = m_mmu->read8(base * index + offset);
        break;
    case OperandSize::WORD:
        value = m_mmu->read16(base * index + offset);
        break;
    case OperandSize::DWORD:
        value = m_mmu->read32(base * index + offset);
        break;
    case OperandSize::QWORD:
        value = m_mmu->read64(base * index + offset);
        break;
    default:
        m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
    }
    return value;
}

void ComplexOperand::SetValue(uint64_t value) {
    int64_t base = 0;
    if (m_complexData->base.present) {
        if (m_complexData->base.type == ComplexItem::Type::REGISTER)
            base = m_complexData->base.data.reg->GetValue();
        else {
            switch (m_complexData->base.data.imm.size) {
            case OperandSize::BYTE:
                base = *static_cast<uint8_t*>(m_complexData->base.data.imm.data);
                break;
            case OperandSize::WORD:
                base = *static_cast<uint16_t*>(m_complexData->base.data.imm.data);
                break;
            case OperandSize::DWORD:
                base = *static_cast<uint32_t*>(m_complexData->base.data.imm.data);
                break;
            case OperandSize::QWORD:
                base = *static_cast<uint64_t*>(m_complexData->base.data.imm.data);
                break;
            default:
                m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            }
        }
    }
    int64_t index = 0;
    if (m_complexData->index.present) {
        if (!m_complexData->base.present)
            base = 1;
        if (m_complexData->index.type == ComplexItem::Type::REGISTER)
            index = m_complexData->index.data.reg->GetValue();
        else {
            switch (m_complexData->index.data.imm.size) {
            case OperandSize::BYTE:
                index = *static_cast<uint8_t*>(m_complexData->index.data.imm.data);
                break;
            case OperandSize::WORD:
                index = *static_cast<uint16_t*>(m_complexData->index.data.imm.data);
                break;
            case OperandSize::DWORD:
                index = *static_cast<uint32_t*>(m_complexData->index.data.imm.data);
                break;
            case OperandSize::QWORD:
                index = *static_cast<uint64_t*>(m_complexData->index.data.imm.data);
                break;
            default:
                m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            }
        }
    } else if (m_complexData->base.present)
        index = 1;
    int64_t offset = 0;
    if (m_complexData->offset.present) {
        if (m_complexData->offset.type == ComplexItem::Type::REGISTER) {
            offset = m_complexData->offset.data.reg->GetValue();
            if (!m_complexData->offset.sign)
                offset = -offset;
        } else {
            switch (m_complexData->offset.data.imm.size) {
            case OperandSize::BYTE:
                offset = *static_cast<uint8_t*>(m_complexData->offset.data.imm.data);
                break;
            case OperandSize::WORD:
                offset = *static_cast<uint16_t*>(m_complexData->offset.data.imm.data);
                break;
            case OperandSize::DWORD:
                offset = *static_cast<uint32_t*>(m_complexData->offset.data.imm.data);
                break;
            case OperandSize::QWORD:
                offset = *static_cast<uint64_t*>(m_complexData->offset.data.imm.data);
                break;
            default:
                m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            }
        }
    }
    switch (m_size) {
    case OperandSize::BYTE:
        m_mmu->write8(base * index + offset, static_cast<uint8_t>(value));
        break;
    case OperandSize::WORD:
        m_mmu->write16(base * index + offset, static_cast<uint16_t>(value));
        break;
    case OperandSize::DWORD:
        m_mmu->write32(base * index + offset, static_cast<uint32_t>(value));
        break;
    case OperandSize::QWORD:
        m_mmu->write64(base * index + offset, value);
        break;
    default:
        m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
    }
}

void ComplexOperand::PrintInfo() const {
    uint64_t base = 0;
    bool base_type = false; // false = register, true = immediate
    Register* base_reg = nullptr;
    bool base_present = m_complexData->base.present;
    if (base_present) {
        if (m_complexData->base.type == ComplexItem::Type::REGISTER) {
            base_type = false;
            base_reg = m_complexData->base.data.reg;
        } else {
            switch (m_complexData->base.data.imm.size) {
            case OperandSize::BYTE:
                base = *static_cast<uint8_t*>(m_complexData->base.data.imm.data);
                break;
            case OperandSize::WORD:
                base = *static_cast<uint16_t*>(m_complexData->base.data.imm.data);
                break;
            case OperandSize::DWORD:
                base = *static_cast<uint32_t*>(m_complexData->base.data.imm.data);
                break;
            case OperandSize::QWORD:
                base = *static_cast<uint64_t*>(m_complexData->base.data.imm.data);
                break;
            default:
                m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            }
            base_type = true;
        }
    }
    uint64_t index = 0;
    bool index_type = false; // false = register, true = immediate
    Register* index_reg = nullptr;
    bool index_present = m_complexData->index.present;
    if (index_present) {
        if (m_complexData->index.type == ComplexItem::Type::REGISTER) {
            index_type = false;
            index_reg = m_complexData->index.data.reg;
        } else {
            switch (m_complexData->index.data.imm.size) {
            case OperandSize::BYTE:
                index = *static_cast<uint8_t*>(m_complexData->index.data.imm.data);
                break;
            case OperandSize::WORD:
                index = *static_cast<uint16_t*>(m_complexData->index.data.imm.data);
                break;
            case OperandSize::DWORD:
                index = *static_cast<uint32_t*>(m_complexData->index.data.imm.data);
                break;
            case OperandSize::QWORD:
                index = *static_cast<uint64_t*>(m_complexData->index.data.imm.data);
                break;
            default:
                m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            }
            index_type = true;
        }
    }
    uint64_t offset = 0;
    bool offset_type = false; // false = register, true = immediate
    Register* offset_reg = nullptr;
    bool offset_sign = m_complexData->offset.sign;
    bool offset_present = m_complexData->offset.present;
    if (offset_present) {
        if (m_complexData->offset.type == ComplexItem::Type::REGISTER) {
            offset_type = false;
            offset_reg = m_complexData->offset.data.reg;
        } else {
            switch (m_complexData->offset.data.imm.size) {
            case OperandSize::BYTE:
                offset = *static_cast<uint8_t*>(m_complexData->offset.data.imm.data);
                break;
            case OperandSize::WORD:
                offset = *static_cast<uint16_t*>(m_complexData->offset.data.imm.data);
                break;
            case OperandSize::DWORD:
                offset = *static_cast<uint32_t*>(m_complexData->offset.data.imm.data);
                break;
            case OperandSize::QWORD:
                offset = *static_cast<uint64_t*>(m_complexData->offset.data.imm.data);
                break;
            default:
                m_cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            }
            offset_type = true;
        }
    }
    char const* size;
    switch (m_size) {
    case OperandSize::BYTE:
        size = "BYTE";
        break;
    case OperandSize::WORD:
        size = "WORD";
        break;
    case OperandSize::DWORD:
        size = "DWORD";
        break;
    case OperandSize::QWORD:
        size = "QWORD";
        break;
    default:
        size = "Unknown";
        break;
    }
    printf("Complex: size = %s", size);
    if (base_present) {
        if (base_type)
            printf(" Base=%#016lx", base);
        else
            printf(" Base=%s", base_reg->GetName());
    }
    if (index_present) {
        if (index_type)
            printf(" Index=%#016lx", index);
        else
            printf(" Index=%s", index_reg->GetName());
    }
    if (offset_present) {
        if (offset_type)
            printf(" Offset=%#016lx", offset);
        else
            printf(" Offset=%c%s", offset_sign ? '+' : '-', offset_reg->GetName());
    }
}
