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

constexpr const char* INSTRUCTIONS_STR = "add mul sub div or xor nor and nand not cmp inc dec shl shr ret call jmp jc jnc jz jnz jl jle jnl jnle jg jge jng jnge mov nop hlt push pop pusha popa int lidt iret syscall sysret enteruser";
constexpr size_t INSTRUCTIONS_STR_LEN = 190 - 1;

bool IsInstruction(const std::string& str) {
    const char* rawToken = str.c_str();
    if (const char* ins = strstr(INSTRUCTIONS_STR, rawToken); ins != nullptr) {
        if (ins != INSTRUCTIONS_STR && ins[-1] != ' ')
            return false;
        if (size_t strSize = str.size(); INSTRUCTIONS_STR + INSTRUCTIONS_STR_LEN <= ins + strSize && ins[strSize] != ' ')
            return false;
        return true;
    }
    return false;
}

Lexer::Lexer() {
}

Lexer::~Lexer() {
}

void Lexer::tokenize(const char* source, size_t sourceSize, const LinkedList::RearInsertLinkedList<PreProcessor::ReferencePoint>& referencePoints) {
    if (source == nullptr || sourceSize == 0)
        return;

    bool startOfToken = true;
    std::string token;
    uint64_t currentOffsetInToken = 0;
    PreProcessor::ReferencePoint* currentReferencePoint = referencePoints.get(0);
    PreProcessor::ReferencePoint* nextReferencePoint = referencePoints.get(1);
    size_t currentReferencePointIndex = 0;
#define APPEND_TOKEN(token) AddToken(token, currentReferencePoint->fileName, currentReferencePoint->line + GetLineDifference(source, currentReferencePoint->offset, i - currentOffsetInToken))
#define ERROR(msg) error(msg, currentReferencePoint->fileName, currentReferencePoint->line + GetLineDifference(source, currentReferencePoint->offset, i))
    for (uint64_t i = 0; i < sourceSize; i++) {
        if (startOfToken) {
            if (source[i] == ' ' || source[i] == '\n' || source[i] == '\t')
                continue;
            else if ((source[i] == '[' || source[i] == ']' || source[i] == ',' || source[i] == '+' || source[i] == '*') || (source[i] == '-' && ((i + 1) >= sourceSize || !(source[i + 1] >= '0' && source[i + 1] <= '9')))) {
                token += source[i];
                APPEND_TOKEN(token);
                token = "";
                currentOffsetInToken = 0;
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
                currentOffsetInToken = 0;
                i = end - source;
            } else if (source[i] == '\'') {
                if ((i + 2) >= sourceSize)
                    ERROR("Invalid character literal");
                i++;
                if (source[i] == '\'')
                    ERROR("Character literal cannot be empty");
                char c = 0;
                if (source[i] == '\\') {
                    i++;
                    if ((i + 1) >= sourceSize)
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
                        if ((i + 2) >= sourceSize)
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
                currentOffsetInToken = 0;
            } else {
                startOfToken = false;
                token += source[i];
                currentOffsetInToken++;
            }
        } else {
            if (source[i] == ' ' || source[i] == '\n' || source[i] == '\t') {
                startOfToken = true;
                APPEND_TOKEN(token);
                token = "";
                currentOffsetInToken = 0;
            } else if (source[i] == '[' || source[i] == ']' || source[i] == ',') {
                startOfToken = true;
                APPEND_TOKEN(token);
                token = "";
                currentOffsetInToken = 0;
                token += source[i];
                APPEND_TOKEN(token);
                token = "";
            } else if (source[i] == '+' || source[i] == '*' || source[i] == '-') {
                if (source[i] == '-' && (i + 1) < sourceSize) {
                    if (source[i + 1] >= '0' && source[i + 1] <= '9') { // do not read outside of bounds
                        startOfToken = true;
                        APPEND_TOKEN(token);
                        token = "";
                        token += source[i];
                        currentOffsetInToken = 1;
                        continue;
                    }
                }
                startOfToken = true;
                APPEND_TOKEN(token);
                token = "";
                currentOffsetInToken = 0;
                token += source[i];
                APPEND_TOKEN(token);
                token = "";
            } else {
                token += source[i];
                currentOffsetInToken++;
            }
        }
        if (nextReferencePoint != nullptr && i + 1 >= nextReferencePoint->offset) {
            currentReferencePointIndex++;
            currentReferencePoint = nextReferencePoint;
            nextReferencePoint = referencePoints.get(currentReferencePointIndex + 1);
        }
    }
#undef APPEND_TOKEN
#undef ERROR
    for (uint64_t i = 0; i < currentOffsetInToken; i++) {
        if (!(token[i] == ' ' || token[i] == '\n' || token[i] == '\t')) {
            AddToken(token, currentReferencePoint->fileName, currentReferencePoint->line + GetLineDifference(source, currentReferencePoint->offset, sourceSize - currentOffsetInToken));
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

void Lexer::AddToken(const std::string& strToken, const std::string& fileName, size_t line) {
    std::string lowerToken;
    for (char c : strToken) { // convert the token to lowercase
        if (c >= 'A' && c <= 'Z')
            c = c - 'A' + 'a';
        lowerToken += c;
    }
    Token* newToken = new Token;
    newToken->fileName = fileName;
    newToken->line = line;
    newToken->data = new char[lowerToken.size() + 1];
    memcpy(newToken->data, lowerToken.c_str(), lowerToken.size());
    static_cast<char*>(newToken->data)[lowerToken.size()] = 0;
    newToken->dataSize = lowerToken.size();

    /* now we identify the token type */
#define IS_REGISTER(token) ((token) == "r0" || (token) == "r1" || (token) == "r2" || (token) == "r3" || (token) == "r4" || (token) == "r5" || (token) == "r6" || (token) == "r7" || (token) == "r8" || (token) == "r9" || (token) == "r10" || (token) == "r11" || (token) == "r12" || (token) == "r13" || (token) == "r14" || (token) == "r15" || (token) == "scp" || (token) == "sbp" || (token) == "stp" || (token) == "cr0" || (token) == "cr1" || (token) == "cr2" || (token) == "cr3" || (token) == "cr4" || (token) == "cr5" || (token) == "cr6" || (token) == "cr7" || (token) == "sts" || (token) == "ip")
    if IS_REGISTER (lowerToken)
        newToken->type = TokenType::REGISTER;
    else if (IsInstruction(lowerToken))
        newToken->type = TokenType::INSTRUCTION;
    else if (lowerToken == "[")
        newToken->type = TokenType::LBRACKET;
    else if (lowerToken == "]")
        newToken->type = TokenType::RBRACKET;
    else if (lowerToken == ",")
        newToken->type = TokenType::COMMA;
    else if (lowerToken == "db" || lowerToken == "dw" || lowerToken == "dd" || lowerToken == "dq" || lowerToken == "org" || lowerToken == "ascii" || lowerToken == "asciiz" || lowerToken == "align")
        newToken->type = TokenType::DIRECTIVE;
    else if (lowerToken == "byte" || lowerToken == "word" || lowerToken == "dword" || lowerToken == "qword")
        newToken->type = TokenType::SIZE;
    else if (lowerToken == "+" || lowerToken == "-" || lowerToken == "*")
        newToken->type = TokenType::OPERATOR;
    else if (lowerToken[0] == '\"' && lowerToken[lowerToken.size() - 1] == '\"')
        newToken->type = TokenType::STRING;
    else {
        if (uint64_t size = lowerToken.size(); size == 0)
            newToken->type = TokenType::UNKNOWN;
        else {
            uint64_t offset = 0;
            if (lowerToken[0] == '.')
                offset++;
            if (lowerToken[size - 1] == ':')
                size--;

            bool isLabel = true;

            for (uint64_t i = 0; (i + offset) < size; i++) {
                if (!((lowerToken[i + offset] >= 'a' && lowerToken[i + offset] <= 'z') || (i > 0 && lowerToken[i + offset] >= '0' && lowerToken[i + offset] <= '9') || ((i + offset + 1) < size && lowerToken[i + offset] == '_'))) {
                    isLabel = false;
                    break;
                }
            }

            if (isLabel) {
                if (offset == 0 && size == lowerToken.size())
                    newToken->type = TokenType::LABEL;
                else if (offset == 1 && size == lowerToken.size())
                    newToken->type = TokenType::SUBLABEL;
                else if (offset == 0 && size < lowerToken.size())
                    newToken->type = TokenType::BLABEL;
                else if (offset == 1 && size < lowerToken.size())
                    newToken->type = TokenType::BSUBLABEL;
            } else {
                bool isNumber = true;
                uint8_t base = 10;
                uint64_t i = 0;
                if (lowerToken[0] == '+' || lowerToken[0] == '-') {
                    i++;
                    if (i >= lowerToken.size()) {
                        isNumber = false;
                    }
                } else if (lowerToken[0] == '0') {
                    i += 2;
                    switch (lowerToken[1]) {
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
                    if (i >= lowerToken.size())
                        isNumber = false;
                }

                for (; i < lowerToken.size(); i++) {
                    if (base == 16) {
                        if (!((lowerToken[i] >= '0' && lowerToken[i] <= '9') || (lowerToken[i] >= 'a' && lowerToken[i] <= 'f'))) {
                            isNumber = false;
                            break;
                        }
                    } else if (base == 2) {
                        if (lowerToken[i] != '0' && lowerToken[i] != '1') {
                            isNumber = false;
                            break;
                        }
                    } else if (base == 8) {
                        if (lowerToken[i] < '0' || lowerToken[i] > '7') {
                            isNumber = false;
                            break;
                        }
                    } else {
                        if (lowerToken[i] < '0' || lowerToken[i] > '9') {
                            isNumber = false;
                            break;
                        }
                    }
                }
                if (isNumber)
                    newToken->type = TokenType::NUMBER;
                else
                    newToken->type = TokenType::UNKNOWN;
            }
        }
    }
    if (newToken->type == TokenType::STRING || newToken->type == TokenType::LABEL || newToken->type == TokenType::SUBLABEL || newToken->type == TokenType::BLABEL || newToken->type == TokenType::BSUBLABEL) {
        // need to switch to the unmodified token
        delete[] static_cast<char*>(newToken->data);
        newToken->data = new char[strToken.size() + 1];
        memcpy(newToken->data, strToken.c_str(), strToken.size());
        static_cast<char*>(newToken->data)[strToken.size()] = 0;
        newToken->dataSize = strToken.size();
    }
    m_tokens.insert(newToken);
#ifdef ASSEMBLER_DEBUG
    printf("Token: \"%s\" at %s:%zu, type = %s\n", lowerToken.c_str(), newToken->fileName.c_str(), newToken->line, TokenTypeToString(newToken->type));
#endif
}

size_t Lexer::GetLineDifference(const char* src, size_t srcOffset, size_t dstOffset) {
    char const* lineStart = src + srcOffset;
    size_t line = 0;
    while (true) {
        lineStart = strchr(lineStart, '\n');
        if (lineStart > src + dstOffset || lineStart == nullptr)
            break;
        lineStart++;
        line++;
    }
    return line;
}

[[noreturn]] void Lexer::error(const char* message, Token* token) {
    error(message, token->fileName, token->line);
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