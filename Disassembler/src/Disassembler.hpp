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

#ifndef _DISASSEMBLER_HPP
#define _DISASSEMBLER_HPP

#include "FileBuffer.hpp"

#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

#include <LibArch/Instruction.hpp>
#include <LibArch/Operand.hpp>

class Disassembler {
public:
    explicit Disassembler(FileBuffer& buffer);
    ~Disassembler();

    void Disassemble(std::function<void()> errorCallback);

    [[nodiscard]] const std::vector<std::string>& GetInstructions() const;

private:
    [[noreturn]] void error(const char* message);

    void PrintCurrentInstruction() const;

    static const char* GetInstructionName(InsEncoding::Opcode opcode);
    static const char* GetRegisterName(InsEncoding::Register reg);

    static void StringifyOperand(const InsEncoding::Operand& operand, std::stringstream& ss);

private:
    FileBuffer& m_buffer;
    uint64_t m_current_offset;
    std::function<void()> m_errorCallback;

    InsEncoding::SimpleInstruction m_current_instruction;
    std::vector<std::string> m_instructions;
};

#endif /* _DISASSEMBLER_HPP */