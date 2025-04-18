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

#include "Lexer.hpp"

#include <cstdio>
#include <cstring>

#include <algorithm>

#include <Common/Util.hpp>

constexpr const char* instructions_str = "add mul sub div or xor nor and nand not cmp inc dec shl shr ret call jmp jc jnc jz jnz jl jle jnl jnle jg jge jng jnge mov nop hlt push pop pusha popa int lidt iret syscall sysret enteruser";
constexpr size_t instructions_str_len = sizeof(instructions_str) - 1;

bool IsInstruction(const std::string& str) {
    const char* raw_token = str.c_str();
    if (const char* ins = strstr(instructions_str, raw_token); ins != nullptr) {
        if (ins != instructions_str && ins[-1] != ' ')
            return false;
        if (size_t str_size = str.size(); instructions_str + instructions_str_len <= ins + str_size && ins[str_size] != ' ')
            return false;
        return true;
    }
    return false;
}

Lexer::Lexer() {
}

Lexer::~Lexer() {
}

void Lexer::tokenize(const char* source, size_t source_size, const LinkedList::RearInsertLinkedList<PreProcessor::ReferencePoint>& reference_points) {
    if (source == nullptr || source_size == 0)
        return;

    bool start_of_token = true;
    std::string token;
    uint64_t current_offset_in_token = 0;
    PreProcessor::ReferencePoint* current_reference_point = reference_points.get(0);
    PreProcessor::ReferencePoint* next_reference_point = reference_points.get(1);
    size_t current_reference_point_index = 0;
#define APPEND_TOKEN(token) AddToken(token, current_reference_point->file_name, current_reference_point->line + GetLineDifference(source, current_reference_point->offset, i - current_offset_in_token))
#define ERROR(msg) error(msg, current_reference_point->file_name, current_reference_point->line + GetLineDifference(source, current_reference_point->offset, i))
    for (uint64_t i = 0; i < source_size; i++) {
        if (start_of_token) {
            if (source[i] == ' ' || source[i] == '\n' || source[i] == '\t')
                continue;
            else if ((source[i] == '[' || source[i] == ']' || source[i] == ',' || source[i] == '+' || source[i] == '*') || (source[i] == '-' && ((i + 1) >= source_size || !(source[i + 1] >= '0' && source[i + 1] <= '9')))) {
                token += source[i];
                APPEND_TOKEN(token);
                token = "";
                current_offset_in_token = 0;
            } else if (source[i] == '\"') {
                char const* end;
                uint64_t offset = i;
                while (true) {
                    end = strchr(source + offset + 1, '\"');
                    if (end == nullptr)
                        ERROR("Unterminated string literal");
                    offset = end - source;
                    if (source[offset - 1] != '\\')
                        break;
                }
                token = std::string(&source[i], end + 1);
                APPEND_TOKEN(token);
                token = "";
                current_offset_in_token = 0;
                i = end - source;
            } else if (source[i] == '\'') {
                if ((i + 2) >= source_size)
                    ERROR("Invalid character literal");
                i++;
                if (source[i] == '\'')
                    ERROR("Character literal cannot be empty");
                char c = 0;
                if (source[i] == '\\') {
                    i++;
                    if ((i + 1) >= source_size)
                        ERROR("Invalid character literal");
                    switch (source[i]) {
                    case 'n':
                        c = '\n';
                        break;
                    case 't':
                        c = '\t';
                        break;
                    case 'r':
                        c = '\r';
                        break;
                    case '0':
                        c = '\0';
                        break;
                    case '\\':
                        c = '\\';
                        break;
                    case '\'':
                        c = '\'';
                        break;
                    case '\"':
                        c = '\"';
                        break;
                    case 'x': {
                        i++;
                        if ((i + 2) >= source_size)
                            ERROR("Invalid character literal");
                        uint8_t hex = 0;
                        for (uint8_t j = 0; j < 2; j++) {
                            hex *= 16;
                            if (char current = source[i + j]; current >= '0' && current <= '9')
                                hex += current - '0';
                            else if (current >= 'a' && current <= 'f')
                                hex += current - 'a' + 10;
                            else if (current >= 'A' && current <= 'F')
                                hex += current - 'A' + 10;
                            else
                                ERROR("Invalid escape sequence");
                        }
                        i++; // due to incrementing earlier, we must increment 1 instead of 2
                        c = static_cast<char>(hex);
                        break;
                    }
                    default:
                        ERROR("Invalid escape sequence");
                    }
                } else
                    c = source[i];
                i++;
                if (source[i] != '\'')
                    ERROR("Invalid character literal: missing end");
                i++;
                uint8_t raw = static_cast<uint8_t>(c);
                token = "";
                if (raw == 0)
                    token = "0";
                else {
                    while (raw != 0) {
                        token += raw % 10 + '0';
                        raw /= 10;
                    }
                    std::ranges::reverse(token);
                }
                APPEND_TOKEN(token);
                token = "";
                current_offset_in_token = 0;
            } else {
                start_of_token = false;
                token += source[i];
                current_offset_in_token++;
            }
        } else {
            if (source[i] == ' ' || source[i] == '\n' || source[i] == '\t') {
                start_of_token = true;
                APPEND_TOKEN(token);
                token = "";
                current_offset_in_token = 0;
            } else if (source[i] == '[' || source[i] == ']' || source[i] == ',') {
                start_of_token = true;
                APPEND_TOKEN(token);
                token = "";
                current_offset_in_token = 0;
                token += source[i];
                APPEND_TOKEN(token);
                token = "";
            } else if (source[i] == '+' || source[i] == '*' || source[i] == '-') {
                if (source[i] == '-' && (i + 1) < source_size) {
                    if (source[i + 1] >= '0' && source[i + 1] <= '9') { // do not read outside of bounds
                        start_of_token = true;
                        APPEND_TOKEN(token);
                        token = "";
                        token += source[i];
                        current_offset_in_token = 1;
                        continue;
                    }
                }
                start_of_token = true;
                APPEND_TOKEN(token);
                token = "";
                current_offset_in_token = 0;
                token += source[i];
                APPEND_TOKEN(token);
                token = "";
            } else {
                token += source[i];
                current_offset_in_token++;
            }
        }
        if (next_reference_point != nullptr && i + 1 >= next_reference_point->offset) {
            current_reference_point_index++;
            current_reference_point = next_reference_point;
            next_reference_point = reference_points.get(current_reference_point_index + 1);
        }
    }
#undef APPEND_TOKEN
#undef ERROR
    for (uint64_t i = 0; i < current_offset_in_token; i++) {
        if (!(token[i] == ' ' || token[i] == '\n' || token[i] == '\t')) {
            AddToken(token, current_reference_point->file_name, current_reference_point->line + GetLineDifference(source, current_reference_point->offset, source_size - current_offset_in_token));
            break;
        }
    }
}

const LinkedList::RearInsertLinkedList<Token>& Lexer::GetTokens() const {
    return m_tokens;
}

const char* Lexer::TokenTypeToString(TokenType type) {
    switch (type) {
    case TokenType::INSTRUCTION:
        return "INSTRUCTION";
    case TokenType::REGISTER:
        return "REGISTER";
    case TokenType::NUMBER:
        return "NUMBER";
    case TokenType::SIZE:
        return "SIZE";
    case TokenType::LBRACKET:
        return "LBRACKET";
    case TokenType::RBRACKET:
        return "RBRACKET";
    case TokenType::DIRECTIVE:
        return "DIRECTIVE";
    case TokenType::BLABEL:
        return "BLABEL";
    case TokenType::BSUBLABEL:
        return "BSUBLABEL";
    case TokenType::LABEL:
        return "LABEL";
    case TokenType::SUBLABEL:
        return "SUBLABEL";
    case TokenType::COMMA:
        return "COMMA";
    case TokenType::OPERATOR:
        return "OPERATOR";
    case TokenType::STRING:
        return "STRING";
    case TokenType::UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

void Lexer::Clear() {
    m_tokens.EnumerateReverse([&](Token* token) -> bool {
        delete[] static_cast<char*>(token->data);
        delete token;
        return true;
    });
    m_tokens.clear();
}

void Lexer::AddToken(const std::string& str_token, const std::string& file_name, size_t line) {
    std::string lower_token;
    for (char c : str_token) { // convert the token to lowercase
        if (c >= 'A' && c <= 'Z')
            c = c - 'A' + 'a';
        lower_token += c;
    }
    Token* new_token = new Token;
    new_token->file_name = file_name;
    new_token->line = line;
    new_token->data = new char[lower_token.size() + 1];
    memcpy(new_token->data, lower_token.c_str(), lower_token.size());
    static_cast<char*>(new_token->data)[lower_token.size()] = 0;
    new_token->data_size = lower_token.size();

    /* now we identify the token type */
#define IS_REGISTER(token) ((token) == "r0" || (token) == "r1" || (token) == "r2" || (token) == "r3" || (token) == "r4" || (token) == "r5" || (token) == "r6" || (token) == "r7" || (token) == "r8" || (token) == "r9" || (token) == "r10" || (token) == "r11" || (token) == "r12" || (token) == "r13" || (token) == "r14" || (token) == "r15" || (token) == "scp" || (token) == "sbp" || (token) == "stp" || (token) == "cr0" || (token) == "cr1" || (token) == "cr2" || (token) == "cr3" || (token) == "cr4" || (token) == "cr5" || (token) == "cr6" || (token) == "cr7" || (token) == "sts" || (token) == "ip")
    if IS_REGISTER (lower_token)
        new_token->type = TokenType::REGISTER;
    else if (IsInstruction(lower_token))
        new_token->type = TokenType::INSTRUCTION;
    else if (lower_token == "[")
        new_token->type = TokenType::LBRACKET;
    else if (lower_token == "]")
        new_token->type = TokenType::RBRACKET;
    else if (lower_token == ",")
        new_token->type = TokenType::COMMA;
    else if (lower_token == "db" || lower_token == "dw" || lower_token == "dd" || lower_token == "dq" || lower_token == "org" || lower_token == "ascii" || lower_token == "asciiz" || lower_token == "align")
        new_token->type = TokenType::DIRECTIVE;
    else if (lower_token == "byte" || lower_token == "word" || lower_token == "dword" || lower_token == "qword")
        new_token->type = TokenType::SIZE;
    else if (lower_token == "+" || lower_token == "-" || lower_token == "*")
        new_token->type = TokenType::OPERATOR;
    else if (lower_token[0] == '\"' && lower_token[lower_token.size() - 1] == '\"')
        new_token->type = TokenType::STRING;
    else {
        if (uint64_t size = lower_token.size(); size == 0)
            new_token->type = TokenType::UNKNOWN;
        else {
            uint64_t offset = 0;
            if (lower_token[0] == '.')
                offset++;
            if (lower_token[size - 1] == ':')
                size--;

            bool is_label = true;

            for (uint64_t i = 0; (i + offset) < size; i++) {
                if (!((lower_token[i + offset] >= 'a' && lower_token[i + offset] <= 'z') || (i > 0 && lower_token[i + offset] >= '0' && lower_token[i + offset] <= '9') || ((i + offset + 1) < size && lower_token[i + offset] == '_'))) {
                    is_label = false;
                    break;
                }
            }

            if (is_label) {
                if (offset == 0 && size == lower_token.size())
                    new_token->type = TokenType::LABEL;
                else if (offset == 1 && size == lower_token.size())
                    new_token->type = TokenType::SUBLABEL;
                else if (offset == 0 && size < lower_token.size())
                    new_token->type = TokenType::BLABEL;
                else if (offset == 1 && size < lower_token.size())
                    new_token->type = TokenType::BSUBLABEL;
            } else {
                bool is_number = true;
                uint8_t base = 10;
                uint64_t i = 0;
                if (lower_token[0] == '+' || lower_token[0] == '-') {
                    i++;
                    if (i >= lower_token.size()) {
                        is_number = false;
                    }
                } else if (lower_token[0] == '0') {
                    i += 2;
                    switch (lower_token[1]) {
                    case 'x':
                        base = 16;
                        break;
                    case 'b':
                        base = 2;
                        break;
                    case 'o':
                        base = 8;
                        break;
                    default:
                        base = 10;
                        i -= 2;
                        break;
                    }
                    if (i >= lower_token.size())
                        is_number = false;
                }

                for (; i < lower_token.size(); i++) {
                    if (base == 16) {
                        if (!((lower_token[i] >= '0' && lower_token[i] <= '9') || (lower_token[i] >= 'a' && lower_token[i] <= 'f'))) {
                            is_number = false;
                            break;
                        }
                    } else if (base == 2) {
                        if (lower_token[i] != '0' && lower_token[i] != '1') {
                            is_number = false;
                            break;
                        }
                    } else if (base == 8) {
                        if (lower_token[i] < '0' || lower_token[i] > '7') {
                            is_number = false;
                            break;
                        }
                    } else {
                        if (lower_token[i] < '0' || lower_token[i] > '9') {
                            is_number = false;
                            break;
                        }
                    }
                }
                if (is_number)
                    new_token->type = TokenType::NUMBER;
                else
                    new_token->type = TokenType::UNKNOWN;
            }
        }
    }
    if (new_token->type == TokenType::STRING || new_token->type == TokenType::LABEL || new_token->type == TokenType::SUBLABEL || new_token->type == TokenType::BLABEL || new_token->type == TokenType::BSUBLABEL) {
        // need to switch to the unmodified token
        delete[] static_cast<char*>(new_token->data);
        new_token->data = new char[str_token.size() + 1];
        memcpy(new_token->data, str_token.c_str(), str_token.size());
        static_cast<char*>(new_token->data)[str_token.size()] = 0;
        new_token->data_size = str_token.size();
    }
    m_tokens.insert(new_token);
#ifdef ASSEMBLER_DEBUG
    printf("Token: \"%s\" at %s:%zu, type = %s\n", lower_token.c_str(), new_token->file_name.c_str(), new_token->line, TokenTypeToString(new_token->type));
#endif
}

size_t Lexer::GetLineDifference(const char* src, size_t src_offset, size_t dst_offset) {
    char const* line_start = src + src_offset;
    size_t line = 0;
    while (true) {
        line_start = strchr(line_start, '\n');
        if (line_start > src + dst_offset || line_start == nullptr)
            break;
        line_start++;
        line++;
    }
    return line;
}

[[noreturn]] void Lexer::error(const char* message, Token* token) {
    error(message, token->file_name, token->line);
}

[[noreturn]] void Lexer::error(const char* message, const std::string& file, size_t line) {
    printf("Lexer error at %s:%zu: %s\n", file.c_str(), line, message);
    Clear();
    exit(1);
}

[[noreturn]] void Lexer::internal_error(const char* message) {
    printf("Lexer internal error: %s\n", message);
    exit(2);
}