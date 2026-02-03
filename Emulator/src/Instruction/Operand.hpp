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

#ifndef _OPERAND_HPP
#define _OPERAND_HPP

#include <Register.hpp>

#include <MMU/MMU.hpp>

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

class Operand {
public:
    Operand();
    Operand(Emulator::CPUState* cpu, OperandSize size, OperandType type);
    virtual ~Operand();

    OperandType GetType() const;
    OperandSize GetSize() const;

    virtual uint64_t GetValue() const = 0;
    virtual void SetValue(uint64_t value) = 0;

    virtual void PrintInfo() const = 0;

protected:
    Emulator::CPUState* m_cpu;
    OperandType m_type;
    OperandSize m_size;
};

class RegisterOperand : public Operand {
public:
    RegisterOperand();
    RegisterOperand(Emulator::CPUState* cpu, OperandSize size, Register* reg);

    uint64_t GetValue() const override;
    void SetValue(uint64_t value) override;

    void PrintInfo() const override;

private:
    Register* m_register;
};

class ImmediateOperand : public Operand {
public:
    ImmediateOperand();
    ImmediateOperand(Emulator::CPUState* cpu, OperandSize size, uint64_t immediate);

    uint64_t GetValue() const override;
    void SetValue(uint64_t value) override;

    void PrintInfo() const override;

private:
    uint64_t m_immediate;
};

class MemoryOperand : public Operand {
public:
    MemoryOperand();
    MemoryOperand(Emulator::CPUState* cpu, OperandSize size, uint64_t address, MMU* mmu);

    uint64_t GetValue() const override;
    void SetValue(uint64_t value) override;

    void PrintInfo() const override;

private:
    uint64_t m_address;
    MMU* m_mmu;
};

class ComplexOperand : public Operand {
public:
    ComplexOperand();
    ComplexOperand(Emulator::CPUState* cpu, OperandSize size, ComplexData* complexData, MMU* mmu);

    uint64_t GetValue() const override;
    void SetValue(uint64_t value) override;

    void PrintInfo() const override;

private:
    ComplexData* m_complexData;
    MMU* m_mmu;
};

#endif /* _OPERAND_HPP */