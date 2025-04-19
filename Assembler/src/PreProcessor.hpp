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

#ifndef _PREPROCESSOR_HPP
#define _PREPROCESSOR_HPP

#include <cstddef>

#include <string>
#include <string_view>

#include <Common/DataStructures/Buffer.hpp>

class PreProcessor {
public:
    struct ReferencePoint {
        size_t line;
        std::string fileName;
        size_t offset; // offset in the preprocessed buffer
    };

    PreProcessor();
    ~PreProcessor();

    void process(const char* source, size_t sourceSize, const std::string_view& fileName);

    [[nodiscard]] size_t GetProcessedBufferSize() const;
    void ExportProcessedBuffer(uint8_t* data) const;

    [[nodiscard]] const LinkedList::RearInsertLinkedList<ReferencePoint>& GetReferencePoints() const;

private:
    static char* GetLine(char* source, size_t sourceSize, size_t& lineSize);

    void HandleIncludes(const char* source, size_t sourceSize, const std::string_view& fileName);

    static size_t GetLineCount(const char* src, const char* dst);

    ReferencePoint* CreateReferencePoint(const char* source, size_t sourceOffset, const std::string& fileName, size_t offset);
    ReferencePoint* CreateReferencePoint(size_t line, const std::string& fileName, size_t offset);

    [[noreturn]] static void error(const char* message, const std::string& file, size_t line);
    [[noreturn]] static void internal_error(const char* message);

private:
    Buffer m_buffer;
    uint64_t m_currentOffset;
    LinkedList::RearInsertLinkedList<ReferencePoint> m_referencePoints;
};

#endif /* _PREPROCESSOR_HPP */