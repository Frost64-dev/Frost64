/*
Copyright (©) 2024  Frosty515

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

#ifndef _PARSER_HPP
#define _PARSER_HPP

#include <stddef.h>

#include <LinkedList.hpp>

#include "Lexer.hpp"

#include <libarch/Instruction.hpp>
#include <libarch/Operand.hpp>

class Parser {
public:
    Parser();
    ~Parser();

    void parse(const LinkedList::RearInsertLinkedList<Token>& tokens);

    void PrintSections(FILE* fd) const;

    void Clear();

    const LinkedList::RearInsertLinkedList<InsEncoding::Label>& GetLabels() const;
    uint64_t GetBaseAddress() const { return m_base_address; }

private:
    InsEncoding::Opcode GetOpcode(const char* name, size_t name_size);
    InsEncoding::Register GetRegister(const char* name, size_t name_size);

    // if the token is going to be printed, a colon followed by a space is insert after the message, then the token is printed inside double quotes
    static void error(const char* message, Token* token, bool print_token = false);

    static const char* GetInstructionName(InsEncoding::Opcode opcode);
    static const char* GetRegisterName(InsEncoding::Register reg);

private:
    LinkedList::RearInsertLinkedList<InsEncoding::Label> m_labels;
    uint64_t m_base_address;
    std::unordered_map<std::string_view, InsEncoding::Opcode> m_opcodes;
    bool m_opcodeTableInitialised;
    std::unordered_map<std::string_view, InsEncoding::Register> m_registers;
    bool m_registerTableInitialised;
};

#endif /* _PARSER_HPP */