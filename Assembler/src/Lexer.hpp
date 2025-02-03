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

#ifndef _LEXER_HPP
#define _LEXER_HPP

#include <string>

#include <common/Data-structures/LinkedList.hpp>

#include "PreProcessor.hpp"

enum class TokenType {
    INSTRUCTION,
    REGISTER,
    NUMBER,
    SIZE,
    LBRACKET,
    RBRACKET,
    DIRECTIVE,
    BLABEL,
    BSUBLABEL,
    LABEL,
    SUBLABEL,
    COMMA,
    OPERATOR,
    STRING,
    UNKNOWN
};

struct Token {
    TokenType type;
    void* data;
    size_t data_size;
    std::string file_name;
    size_t line;
};

class Lexer {
   public:
    Lexer();
    ~Lexer();

    void tokenize(const char* source, size_t source_size, const LinkedList::RearInsertLinkedList<PreProcessor::ReferencePoint>& reference_points);

    [[nodiscard]] const LinkedList::RearInsertLinkedList<Token>& GetTokens() const;

    static const char* TokenTypeToString(TokenType type);

    void Clear();

   private:
    void AddToken(const std::string& str_token, const std::string& file_name, size_t line);

    static size_t GetLineDifference(const char* src, size_t src_offset, size_t dst_offset);

    [[noreturn]] void error(const char* message, Token* token);
    [[noreturn]] void error(const char* message, const std::string& file, size_t line);
    [[noreturn]] static void internal_error(const char* message);

   private:
    LinkedList::RearInsertLinkedList<Token> m_tokens;
};

#endif /* _LEXER_HPP */