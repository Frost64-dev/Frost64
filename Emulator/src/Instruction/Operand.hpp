/*
Copyright (©) 2023-2024  Frosty515

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

#ifndef _OPERAND_HPP
#define _OPERAND_HPP

#include <Register.hpp>

#include <LibArch/Instruction.hpp>

enum class OperandType {
    Register,
    Immediate,
    Memory,
    Complex
};

enum class OperandSize {
    BYTE,
    WORD,
    DWORD,
    QWORD,
    Unknown
};

struct ComplexItem {
    bool present;
    bool sign;
    enum class Type {
        REGISTER,
        IMMEDIATE
    } type;
    union CI_Data {
        Register* reg;
        struct {
            OperandSize size;
            void* data;
        } imm;
    } data;
};

struct ComplexData {
    ComplexItem base;
    ComplexItem index;
    ComplexItem offset;
};

typedef void (*MemoryOperation_t)(uint64_t address, void* data, uint64_t size, uint64_t count, bool write);

class Operand {
public:
    Operand();
    Operand(OperandSize size, OperandType type, ...);
    ~Operand();

    Register* GetRegister();

    OperandType GetType() const;

    OperandSize GetSize() const;

    uint64_t GetOffset() const;

    uint64_t GetAddress() const;

    ComplexData* GetComplexData();

    void PrintInfo() const;

    uint64_t GetValue() const;
    void SetValue(uint64_t value);

private:
    Register* m_register;
    OperandType m_type;
    OperandSize m_size;
    uint64_t m_offset;
    uint64_t m_address;
    ComplexData* m_complexData;
    MemoryOperation_t m_memoryOperation;
};

#endif /* _OPERAND_HPP */