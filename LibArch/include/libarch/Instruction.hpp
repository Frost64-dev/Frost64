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

#include <string>

#include <common/Data-structures/Buffer.hpp>
#include <common/Data-structures/LinkedList.hpp>

#include "Operand.hpp"

namespace InsEncoding {
    enum class Opcode {
        ADD = 0x0,
        MUL,
        SUB,
        DIV,
        OR,
        XOR,
        NOR,
        AND,
        NAND,
        NOT,
        CMP,
        INC,
        DEC,
        SHL,
        SHR,
        RET = 0x10,
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
        MOV = 0x20,
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
        size_t name_size;
        LinkedList::RearInsertLinkedList<Data> data_blocks;
        LinkedList::RearInsertLinkedList<uint64_t> jumps_to_here;
    };

    struct Label {
        char* name;
        size_t name_size;
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

    struct ComplexOperandInfo {
        uint8_t type           : 2;
        uint8_t size           : 2;
        uint8_t base_type      : 1;
        uint8_t base_size      : 2;
        uint8_t base_present   : 1;
        uint8_t index_type     : 1;
        uint8_t index_size     : 2;
        uint8_t index_present  : 1;
        uint8_t offset_type    : 1;
        uint8_t offset_size    : 2;
        uint8_t offset_present : 1;
    } __attribute__((packed));

    struct StandardOperandInfo {
        uint8_t type     : 2;
        uint8_t size     : 2;
        uint8_t _padding : 4;
    } __attribute__((packed));

    struct ComplexStandardOperandInfo {
        ComplexOperandInfo complex;
        StandardOperandInfo standard;
    } __attribute__((packed));

    struct StandardComplexOperandInfo {
        StandardOperandInfo standard;
        ComplexOperandInfo complex;
    } __attribute__((packed));

    struct StandardStandardOperandInfo {
        uint8_t first_type  : 2;
        uint8_t first_size  : 2;
        uint8_t second_type : 2;
        uint8_t second_size : 2;
    } __attribute__((packed));

    struct ComplexComplexOperandInfo {
        ComplexOperandInfo first;
        ComplexOperandInfo second;
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
        size_t data_size;
        RawDataType type;
        std::string file_name;
        size_t line;
    };

    class Instruction {
       public:
        Instruction();
        Instruction(Opcode opcode, const std::string& file_name, size_t line);
        ~Instruction();

        void SetOpcode(Opcode opcode);

        Opcode GetOpcode() const;

        const std::string& GetFileName() const;
        size_t GetLine() const;

        LinkedList::RearInsertLinkedList<Operand> operands;

       private:
        Opcode m_opcode;
        std::string m_file_name;
        size_t m_line;
    };

    class SimpleInstruction { // alternative that doesn't use the heap
       public:
        SimpleInstruction();
        explicit SimpleInstruction(Opcode opcode);
        ~SimpleInstruction();

        void SetOpcode(Opcode opcode);

        Opcode GetOpcode() const;

        Operand operands[2];
        size_t operand_count;

       private:
        Opcode m_opcode;

    };

    bool DecodeInstruction(const uint8_t* data, size_t data_size, SimpleInstruction* out, void (*error_handler)(const char* message, void* data), void* error_data = nullptr);
    bool DecodeInstruction(Buffer& buffer, uint64_t& current_offset, SimpleInstruction* out, void (*error_handler)(const char* message, void* data), void* error_data = nullptr);
    size_t EncodeInstruction(Instruction* instruction, uint8_t* data, size_t data_size, uint64_t global_offset);
} // namespace InsEncoding

#endif /* _LIBARCH_INSTRUCTION_HPP */