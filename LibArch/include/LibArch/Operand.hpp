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

#ifndef _LIBARCH_OPERAND_HPP
#define _LIBARCH_OPERAND_HPP

namespace InsEncoding {

#define CONVERT_COMPACT_TO_OPERAND(type) \
    ((type) == CompactOperandType::REG ? OperandType::REGISTER : \
     (type) == CompactOperandType::IMM ? OperandType::IMMEDIATE : \
     (type) == CompactOperandType::MEM_BASE_IMM ? OperandType::MEMORY : \
     (type) == CompactOperandType::MEM_BASE_REG || ((type) >= CompactOperandType::MEM_BASE_OFF_REG && (type) <= CompactOperandType::MEM_BASE_IDX_OFF_REG_IMM2) ? OperandType::COMPLEX : \
     OperandType::UNKNOWN)

    enum class OperandType {
        REGISTER = 0,
        IMMEDIATE,
        MEMORY,
        COMPLEX,
        POTENTIAL_MEMORY, // not sure if it is memory or complex
        LABEL,
        SUBLABEL,
        UNKNOWN
    };

    enum class CompactOperandType {
        REG = 0,
        IMM,
        MEM_BASE_REG,
        MEM_BASE_IMM,
        MEM_BASE_OFF_REG,
        MEM_BASE_OFF_REG_IMM,
        MEM_BASE_OFF_IMM_REG,
        MEM_BASE_OFF_IMM2,
        MEM_BASE_IDX_REG,
        MEM_BASE_IDX_REG_IMM,
        MEM_BASE_IDX_OFF_REG,
        MEM_BASE_IDX_OFF_REG2_IMM,
        MEM_BASE_IDX_OFF_REG_IMM_REG,
        MEM_BASE_IDX_OFF_REG_IMM2,
        RESERVED,
        MASK
    };



    enum class OperandSize : unsigned char {
        BYTE = 0,
        WORD = 1,
        DWORD = 2,
        QWORD = 3
    };

    class Operand {
    public:
        Operand();
        Operand(OperandType type, OperandSize size, void* data);
        ~Operand();

        OperandType type;
        CompactOperandType fullType;
        OperandSize size;
        void* data;
        bool complete;
    };

}

#endif /* _LIBARCH_OPERAND_HPP */