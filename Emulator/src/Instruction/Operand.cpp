/*
Copyright (©) 2023-2025  Frosty515

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

#include "Exceptions.hpp"

Operand::Operand()
    : m_register(nullptr), m_type(OperandType::Register), m_size(OperandSize::Unknown), m_offset(0), m_address(0), m_complexData(nullptr), m_memoryOperation(nullptr) {
}

Operand::Operand(OperandSize size, Register* reg)
    : m_register(reg), m_type(OperandType::Register), m_size(size) {
}

Operand::Operand(OperandSize size, uint64_t immediate)
    :  m_type(OperandType::Immediate), m_size(size), m_offset(immediate) {
}

Operand::Operand(OperandSize size, uint64_t address, MemoryOperation_t operation)
    : m_type(OperandType::Memory), m_size(size), m_address(address), m_memoryOperation(operation) {
}

Operand::Operand(OperandSize size, ComplexData* complexData, MemoryOperation_t operation)
    : m_type(OperandType::Complex), m_size(size), m_complexData(complexData), m_memoryOperation(operation) {
}

Operand::~Operand() {
}

Register* Operand::GetRegister() {
    return m_register;
}

OperandType Operand::GetType() const {
    return m_type;
}

OperandSize Operand::GetSize() const {
    return m_size;
}

uint64_t Operand::GetOffset() const {
    return m_offset;
}

uint64_t Operand::GetAddress() const {
    return m_address;
}

ComplexData* Operand::GetComplexData() {
    return m_complexData;
}

void Operand::PrintInfo() const {
    switch (m_type) {
    case OperandType::Register:
        printf("Register: %s", m_register->GetName());
        break;
    case OperandType::Immediate: {
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
        printf("Immediate: value = 0x%lx, size = %s", m_offset, size);
        break;
    }
    case OperandType::Memory: {
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
        printf("Memory: %#16lx, size = %s", m_address, size);
        break;
    }
    case OperandType::Complex: {
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
                    g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
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
                    g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
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
                    g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
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
                printf(" Base=%#16lx", base);
            else
                printf(" Base=%s", base_reg->GetName());
        }
        if (index_present) {
            if (index_type)
                printf(" Index=%#16lx", index);
            else
                printf(" Index=%s", index_reg->GetName());
        }
        if (offset_present) {
            if (offset_type)
                printf(" Offset=%#16lx", offset);
            else
                printf(" Offset=%c%s", offset_sign ? '+' : '-', offset_reg->GetName());
        }
        break;
    }
    }
}

uint64_t Operand::GetValue() const {
    switch (m_type) {
    case OperandType::Register:
        return m_register->GetValue(m_size);
    case OperandType::Immediate:
        return m_offset;
    case OperandType::Memory: {
        uint64_t value = 0;
        m_memoryOperation(m_address, &value, 1 << static_cast<uint8_t>(m_size), 1, false);
        return value;
    }
    case OperandType::Complex: {
        uint64_t base = 0;
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
                    g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
                }
            }
        }
        uint64_t index = 0;
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
                    g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
                }
            }
        } else if (m_complexData->base.present)
            index = 1;
        uint64_t offset = 0;
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
                    g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
                }
            }
        }
        uint64_t value = 0;
        m_memoryOperation(base * index + offset, &value, 1 << static_cast<uint8_t>(m_size), 1, false);
        return value;
    }
    default:
        return 0;
    }
}

void Operand::SetValue(uint64_t value) {
    switch (m_type) {
    case OperandType::Register:
        if (!m_register->SetValue(value, m_size))
            g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
        break;
    case OperandType::Immediate:
        g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
    case OperandType::Memory:
        m_memoryOperation(m_address, &value, 1 << static_cast<uint8_t>(m_size), 1, true);
        break;
    case OperandType::Complex: {
        uint64_t base = 0;
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
                    g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
                }
            }
        }
        uint64_t index = 0;
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
                    g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
                }
            }
        } else if (m_complexData->base.present)
            index = 1;
        uint64_t offset = 0;
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
                    g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
                }
            }
        }
        m_memoryOperation(base * index + offset, &value, 1 << static_cast<uint8_t>(m_size), 1, true);
        break;
    }
    }
}
