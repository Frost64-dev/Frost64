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

#ifndef _PARSER_HPP
#define _PARSER_HPP

#include <cstddef>

#include <string_view>

#include <Common/DataStructures/LinkedList.hpp>

#include <LibArch/Instruction.hpp>

#include "Lexer.hpp"

class Parser {
public:
    Parser();
    ~Parser();

    // builds a new list of tokens that is stored internally
    void SimplifyExpressions(const LinkedList::RearInsertLinkedList<Token>& tokens);
    void parse();

    void PrintSections(FILE* fd) const;

    void Clear();

    const LinkedList::RearInsertLinkedList<InsEncoding::Label>& GetLabels() const;
    uint64_t GetBaseAddress() const { return m_baseAddress; }

private:
    InsEncoding::Opcode GetOpcode(const char* name, size_t nameSize);
    InsEncoding::Register GetRegister(const char* name, size_t nameSize);

    Token* SimplifyExpression(const LinkedList::RearInsertLinkedList<Token>& tokens);

    // if the token is going to be printed, a colon followed by a space is insert after the message, then the token is printed inside double quotes
    [[noreturn]] static void error(const char* message, Token* token, bool printToken = false);

    static const char* GetInstructionName(InsEncoding::Opcode opcode);
    static const char* GetRegisterName(InsEncoding::Register reg);

private:
    LinkedList::RearInsertLinkedList<Token> m_tokens;
    LinkedList::RearInsertLinkedList<InsEncoding::Label> m_labels;
    uint64_t m_baseAddress;
    std::unordered_map<std::string_view, InsEncoding::Opcode> m_opcodes;
    bool m_opcodeTableInitialised;
    std::unordered_map<std::string_view, InsEncoding::Register> m_registers;
    bool m_registerTableInitialised;
};

#endif /* _PARSER_HPP */