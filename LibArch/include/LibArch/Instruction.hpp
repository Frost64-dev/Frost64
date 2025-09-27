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

#ifndef _LIBARCH_INSTRUCTION_HPP
#define _LIBARCH_INSTRUCTION_HPP

#include <iterator>
#include <string>

#include <Common/DataStructures/Buffer.hpp>
#include <Common/DataStructures/LinkedList.hpp>

#include "Operand.hpp"

namespace InsEncoding {
    enum class Opcode {
        ADD = 0x0,
        SUB,
        MUL,
        DIV,
        SMUL,
        SDIV,
        OR,
        NOR,
        XOR,
        XNOR,
        AND,
        NAND,
        NOT,
        SHL,
        SHR,
        CMP,
        INC,
        DEC,
        RET = 0x20,
        CALL,
        JMP,
        JC,
        JNC,
        JZ,
        JNZ,
        JL,
        JLE,
        JNL,
        JNLE,
        MOV = 0x30,
        NOP,
        HLT,
        PUSH,
        POP,
        PUSHA,
        POPA,
        INT,
        LIDT,
        IRET,
        SYSCALL,
        SYSRET,
        ENTERUSER,
        UNKNOWN = 0xFF
    };

    enum class Register {
        r0,
        r1,
        r2,
        r3,
        r4,
        r5,
        r6,
        r7,
        r8,
        r9,
        r10,
        r11,
        r12,
        r13,
        r14,
        r15,
        scp,
        sbp,
        stp,
        _invalid_0x13,
        _invalid_0x14,
        _invalid_0x15,
        _invalid_0x16,
        _invalid_0x17,
        _invalid_0x18,
        _invalid_0x19,
        _invalid_0x1A,
        _invalid_0x1B,
        _invalid_0x1C,
        _invalid_0x1D,
        _invalid_0x1E,
        _invalid_0x1F,
        cr0 = 0x20,
        cr1,
        cr2,
        cr3,
        cr4,
        cr5,
        cr6,
        cr7,
        sts,
        ip,
        unknown = 0xFF
    };

    struct Data {
        Data()
            : type(false), data(nullptr) {}
        ~Data() {}
        bool type; // true for instruction, false for raw data
        void* data;
    };

    struct Block {
        char* name;
        size_t nameSize;
        LinkedList::RearInsertLinkedList<Data> dataBlocks;
        LinkedList::RearInsertLinkedList<uint64_t> jumpsToHere;
    };

    struct Label {
        char* name;
        size_t nameSize;
        LinkedList::RearInsertLinkedList<Block> blocks;
    };

    struct ComplexItem {
        bool present;
        bool sign;
        enum class Type {
            REGISTER,
            IMMEDIATE,
            LABEL,
            SUBLABEL,
            UNKNOWN
        } type;
        union {
            Register* reg;
            struct ImmediateData {
                OperandSize size;
                void* data;
            } imm;
            Label* label;
            Block* sublabel;
            uint64_t raw;
        } data;
    };

    struct ComplexData {
        ComplexItem base;
        ComplexItem index;
        ComplexItem offset;
        enum class Stage {
            BASE,
            INDEX,
            OFFSET
        } stage;
    };

    struct RegisterID {
        uint8_t number : 4;
        uint8_t type   : 4;
    } __attribute__((packed));



    struct BasicOperandInfo {
        CompactOperandType type : 4;
        OperandSize size : 2;
        OperandSize imm0Size : 2; // size of immediate 0 if present
    } __attribute__((packed));

    struct ExtendedOperandInfo {
        CompactOperandType type : 4;
        OperandSize size : 2;
        OperandSize imm0Size : 2; // size of immediate 0 if present
        OperandSize imm1Size : 2; // size of immediate 1 if present
        uint8_t reserved : 6;
    } __attribute__((packed));

    struct DoubleExtendedOperandInfo { // this one needs to be done differently so that a byte isn't wasted on padding
        struct {
            CompactOperandType type : 4;
            OperandSize size : 2;
            OperandSize imm0Size : 2; // size of immediate 0 if present
            OperandSize imm1Size : 2; // size of immediate 1 if present
            uint8_t reserved : 2;
        } __attribute__((packed)) first;
        struct {
            CompactOperandType type : 4;
            OperandSize size : 2;
            OperandSize imm0Size : 2; // size of immediate 0 if present
            OperandSize imm1Size : 2; // size of immediate 1 if present
            uint8_t reserved : 2;
        } __attribute__((packed)) second;
    } __attribute__((packed));

    struct TripleExtendedOperandInfo { // this one needs to be done differently so that a byte isn't wasted on padding
        struct {
            CompactOperandType type : 4;
            OperandSize size : 2;
            OperandSize imm0Size : 2; // size of immediate 0 if present
            OperandSize imm1Size : 2; // size of immediate 1 if present
            uint8_t reserved : 1;
            uint8_t isTripleExtended : 1; // set to 1 to indicate this is a triple extended struct
        } __attribute__((packed)) first;
        struct {
            CompactOperandType type : 4;
            OperandSize size : 2;
            OperandSize imm0Size : 2; // size of immediate 0 if present
        } __attribute__((packed)) second;
        struct {
            CompactOperandType type : 4;
            OperandSize size : 2;
            OperandSize imm0Size : 2; // size of immediate 0 if present
            OperandSize imm1Size : 2; // size of immediate 1 if present
            uint8_t reserved : 2;
        } __attribute__((packed)) third;
    } __attribute__((packed));

    struct ComplexOperandInfo {
        uint8_t type           : 2;
        uint8_t size           : 2;
        uint8_t baseType      : 1;
        uint8_t baseSize      : 2;
        uint8_t basePresent   : 1;
        uint8_t indexType     : 1;
        uint8_t indexSize     : 2;
        uint8_t indexPresent  : 1;
        uint8_t offsetType    : 1;
        uint8_t offsetSize    : 2;
        uint8_t offsetPresent : 1;
    } __attribute__((packed));


    enum class RawDataType {
        RAW,
        LABEL,
        SUBLABEL,
        ASCII,
        ASCIIZ,
        ALIGNMENT
    };

    struct RawData {
        void* data;
        size_t dataSize;
        RawDataType type;
        std::string fileName;
        size_t line;
    };

    class Instruction {
       public:
        Instruction();
        Instruction(Opcode opcode, const std::string& fileName, size_t line);
        ~Instruction();

        void SetOpcode(Opcode opcode);

        Opcode GetOpcode() const;

        const std::string& GetFileName() const;
        size_t GetLine() const;

        LinkedList::RearInsertLinkedList<Operand> operands;

       private:
        Opcode m_opcode;
        std::string m_fileName;
        size_t m_line;
    };

    class SimpleInstruction { // alternative that doesn't use the heap
       public:
        SimpleInstruction();
        explicit SimpleInstruction(Opcode opcode);
        ~SimpleInstruction();

        void SetOpcode(Opcode opcode);

        Opcode GetOpcode() const;

        Operand operands[3];
        size_t operandCount;

       private:
        Opcode m_opcode;

    };

    const char* GetInstructionName(Opcode opcode);

    bool DecodeInstruction(StreamBuffer& buffer, uint64_t& currentOffset, SimpleInstruction* out, void (*errorHandler)(const char* message, void* data), void* errorData = nullptr);
    size_t EncodeInstruction(Instruction* instruction, uint8_t* data, size_t dataSize, uint64_t globalOffset);
} // namespace InsEncoding

#endif /* _LIBARCH_INSTRUCTION_HPP */