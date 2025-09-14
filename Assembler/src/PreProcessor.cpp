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

#include "PreProcessor.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include <fstream>

#ifdef ENABLE_CXX20_FORMAT
#include <format>
#else
#include <sstream>
#endif

PreProcessor::PreProcessor()
    : m_currentOffset(0) {
}

PreProcessor::~PreProcessor() {
}

void PreProcessor::process(const char* source, size_t sourceSize, const std::string_view& fileName) {
    // 1. resolve includes in %include "file" format
    {
        char* source_i = new char[sourceSize + 1];
        strncpy(source_i, source, sourceSize);
        source_i[sourceSize] = '\0';
        source = source_i;
    }
    const char* originalSource = source;

    CreateReferencePoint(1, fileName.data(), 0);
    HandleIncludes(source, sourceSize, fileName);
    CreateReferencePoint(source, sourceSize, fileName.data(), m_currentOffset);

    delete[] originalSource;

    // export the buffer to a char const* and size
    size_t source2_size = m_currentOffset;
    char const* source2;
    {
        char* source2_i = new char[source2_size + 1];
        m_buffer.Read(0, reinterpret_cast<uint8_t*>(source2_i), source2_size);
        source2_i[source2_size] = '\0';
        source2 = source2_i;
    }

    // clear the buffer
    m_buffer.Clear();
    m_currentOffset = 0;

    const char* originalSource2 = source2;

    // 2. remove single line comments starting with ;
    while (true) {
        if (char const* commentStart = strchr(source2, ';'); commentStart == nullptr) {
            m_buffer.Write(m_currentOffset, reinterpret_cast<const uint8_t*>(source2), source2_size - (source2 - originalSource2));
            m_currentOffset += source2_size - (source2 - originalSource2);
            break;
        } else {
            m_buffer.Write(m_currentOffset, reinterpret_cast<const uint8_t*>(source2), commentStart - source2);
            m_currentOffset += commentStart - source2;
            source2 = commentStart;
            char const* commentEnd = strchr(source2, '\n');
            // need to update all the reference points
            m_referencePoints.Enumerate([&](ReferencePoint* ref) {
                if (ref->offset >= m_currentOffset)
                    ref->offset -= commentEnd - commentStart;
            });
            if (commentEnd == nullptr) {
                commentEnd = originalSource2 + source2_size;
                break;
            }
            source2 = commentEnd;
        }
    }

    delete[] originalSource2;

    // export the buffer to a char const* and size
    size_t source3_size = m_currentOffset;
    char const* source3;
    {
        char* source3_i = new char[source3_size + 1];
        m_buffer.Read(0, reinterpret_cast<uint8_t*>(source3_i), source3_size);
        source3_i[source3_size] = '\0';
        source3 = source3_i;
    }

    // clear the buffer
    m_buffer.Clear();
    m_currentOffset = 0;

    const char* originalSource3 = source3;

    // 3. remove multi line comments starting with /* and ending with */
    while (true) {
        if (char const* commentStart = strstr(source3, "/*"); commentStart == nullptr) {
            m_buffer.Write(m_currentOffset, reinterpret_cast<const uint8_t*>(source3), source3_size - (source3 - originalSource3));
            m_currentOffset += source3_size - (source3 - originalSource3);
            break;
        } else {
            m_buffer.Write(m_currentOffset, reinterpret_cast<const uint8_t*>(source3), commentStart - source3);
            m_currentOffset += commentStart - source3;
            source3 = commentStart;
            // need to add a reference point prior to the comment
            uint64_t index = 0;
            ReferencePoint* ref = nullptr;
            m_referencePoints.EnumerateReverse([&](ReferencePoint* currentRef, uint64_t i) {
                if (currentRef->offset < static_cast<size_t>(source3 - originalSource3)) {
                    ReferencePoint* newRef = new ReferencePoint{GetLineCount(originalSource3 + currentRef->offset, source3), currentRef->fileName, m_currentOffset - 1};
                    m_referencePoints.insertAt(i, newRef);
                    index = i;
                    ref = newRef;
                    return false;
                }
                return true;
            });
            if (ref == nullptr)
                internal_error("Unable to find previous reference point whilst resolving a multiline include.");
            char const* commentEnd = strstr(source3, "*/");
            if (commentEnd == nullptr)
                error("Unterminated multiline comment", ref->fileName, ref->line);
            source3 = commentEnd + 2;
            // insert a reference point after the comment
            ReferencePoint* newRef = new ReferencePoint{GetLineCount(originalSource3, source3 - 2), ref->fileName, m_currentOffset};
            m_referencePoints.insertAt(index + 1, newRef);
            // now need to update all the reference points
            m_referencePoints.Enumerate([&](ReferencePoint* currentRef, uint64_t) {
                if (currentRef->offset >= m_currentOffset + (commentEnd - commentStart) + 2)
                    currentRef->offset -= (commentEnd - commentStart) + 2;
                return true;
            });
        }
    }

    delete[] originalSource3;

    // export the buffer to a char const* and size
    size_t source4Size = m_currentOffset;
    char const* source4;
    {
        char* source4_i = new char[source4Size + 1];
        m_buffer.Read(0, reinterpret_cast<uint8_t*>(source4_i), source4Size);
        source4_i[source4Size] = '\0';
        source4 = source4_i;
    }

    // clear the buffer
    m_buffer.Clear();
    m_currentOffset = 0;

    const char* originalSource4 = source4;

    // 4. resolve macros in %define name value format

    struct Define {
        std::string name;
        std::string value;
        char const* start;
    };

    std::vector<Define> defines;

    // start by finding all the macros and making a list of them
    while (true) {
        char* defineStart = strstr(const_cast<char*>(source4), "%define ");
        if (defineStart == nullptr)
            break;
        defineStart += 8;
        char* defineEnd = strchr(defineStart, ' ');
        if (defineEnd == nullptr) {
            // need to find the previous reference point
            ReferencePoint* ref = nullptr;
            m_referencePoints.EnumerateReverse([&](ReferencePoint* currentRef) {
                if (currentRef->offset < static_cast<size_t>(defineStart - source4)) {
                    ref = currentRef;
                    return false;
                }
                return true;
            });
            if (ref == nullptr)
                internal_error("Unable to find previous reference point whilst handling a define directive error.");
            error("Invalid define directive", ref->fileName, ref->line + GetLineCount(originalSource4 + ref->offset, defineStart));
        }
        char* valueEnd = strchr(defineEnd + 1, '\n');
        if (valueEnd == nullptr) {
            // need to find the previous reference point
            ReferencePoint* ref = nullptr;
            m_referencePoints.EnumerateReverse([&](ReferencePoint* currentRef) {
                if (currentRef->offset < static_cast<size_t>(defineStart - source4)) {
                    ref = currentRef;
                    return false;
                }
                return true;
            });
            if (ref == nullptr)
                internal_error("Unable to find previous reference point whilst handling a define directive error.");
            error("Unterminated define directive", ref->fileName, ref->line + GetLineCount(originalSource4 + ref->offset, defineStart));
        }
        defines.push_back({std::string(defineStart, defineEnd), std::string(defineEnd + 1, valueEnd), defineStart - 8});
        source4 = valueEnd + 1;
    }

    source4 = originalSource4;

    // then remove the macros from the source
    for (Define& define : defines) {
        m_buffer.Write(m_currentOffset, reinterpret_cast<const uint8_t*>(source4), define.start - source4);
        m_currentOffset += define.start - source4;
        source4 = define.start + define.name.size() + define.value.size() + 9;
        // update all the reference points
        m_referencePoints.Enumerate([&](ReferencePoint* ref) {
            if (ref->offset >= m_currentOffset)
                ref->offset -= define.name.size() + define.value.size() + 9;
        });
    }
    // write the rest of the source
    m_buffer.Write(m_currentOffset, reinterpret_cast<const uint8_t*>(source4), source4Size - (source4 - originalSource4));
    m_currentOffset += source4Size - (source4 - originalSource4);

    // export into source 5
    delete[] originalSource4;

    size_t source5_size = m_currentOffset;
    char const* source5;
    {
        char* source5_i = new char[source5_size + 1];
        m_buffer.Read(0, reinterpret_cast<uint8_t*>(source5_i), source5_size);
        source5_i[source5_size] = '\0';
        source5 = source5_i;
    }

    // clear the buffer
    m_buffer.Clear();
    m_currentOffset = 0;

    char const* originalSource5 = source5;

    // make a list of all references to the macros
    struct DefineReference {
        std::string name;
        char const* start;
    };

    std::map<size_t /* offset */, Define&> defineReferences;

    for (Define& define : defines) {
        char* defineStart = const_cast<char*>(source5);
        while (defineStart != nullptr) {
            defineStart = strstr(defineStart, define.name.c_str());
            if (defineStart != nullptr) {
                defineReferences.insert({defineStart - source5, define});
                defineStart += define.name.size();
            }
        }
    }

    for (auto& [offset, define] : defineReferences) {
        if (offset < m_currentOffset) { // possible overlap
            // need to ensure it does overlap and isn't before. if it is before, then an error has occurred
            if (offset + define.name.size() <= m_currentOffset)
                internal_error("Illegal define reference.");

            // no need to copy before the offset
            m_buffer.Write(m_currentOffset - (m_currentOffset - offset), reinterpret_cast<const uint8_t*>(define.value.c_str()), define.value.size());
            m_currentOffset += define.value.size() - (m_currentOffset - offset);
            source5 += define.name.size() - (m_currentOffset - offset);
            // update all the reference points
            m_referencePoints.Enumerate([&](ReferencePoint* ref) {
                    if (ref->offset >= m_currentOffset)
                    ref->offset += define.value.size() - (m_currentOffset - offset);
            });
        } else {
            m_buffer.Write(m_currentOffset, reinterpret_cast<const uint8_t*>(source5), offset - (source5 - originalSource5));
            m_currentOffset += offset - (source5 - originalSource5);
            m_buffer.Write(m_currentOffset, reinterpret_cast<const uint8_t*>(define.value.c_str()), define.value.size());
            m_currentOffset += define.value.size();
            source5 += offset - (source5 - originalSource5) + define.name.size();
            // update all the reference points
            m_referencePoints.Enumerate([&](ReferencePoint* ref) {
                if (ref->offset >= m_currentOffset)
                    ref->offset += define.value.size() - define.name.size();
            });
        }
    }

    // copy the rest of the source
    m_buffer.Write(m_currentOffset, reinterpret_cast<const uint8_t*>(source5), source5_size - (source5 - originalSource5));
    m_currentOffset += source5_size - (source5 - originalSource5);

    delete[] originalSource5;
}

size_t PreProcessor::GetProcessedBufferSize() const {
    return m_currentOffset;
}

void PreProcessor::ExportProcessedBuffer(uint8_t* data) const {
    m_buffer.Read(0, data, m_currentOffset);
}

const LinkedList::RearInsertLinkedList<PreProcessor::ReferencePoint>& PreProcessor::GetReferencePoints() const {
    return m_referencePoints;
}

char* PreProcessor::GetLine(char* source, size_t sourceSize, size_t& lineSize) {
    if (void* line = memchr(source, '\n', sourceSize); line != nullptr) {
        lineSize = static_cast<char*>(line) - source;
        return static_cast<char*>(line) + 1;
    }
    lineSize = sourceSize;
    return nullptr;
}

void PreProcessor::HandleIncludes(const char* source, size_t sourceSize, const std::string_view& fileName) {
    char* includeStart = const_cast<char*>(source);
    char* includeEnd = nullptr;
    char const* i_source = source;

    while (includeStart != nullptr) {
        includeStart = strstr(includeStart, "%include \"");
        if (includeStart != nullptr) {
            m_buffer.Write(m_currentOffset, reinterpret_cast<const uint8_t*>(i_source), includeStart - i_source);
            m_currentOffset += includeStart - i_source;
            ReferencePoint* startRef = CreateReferencePoint(source, includeStart - source, fileName.data(), m_currentOffset);
            includeStart += 10;
            includeEnd = strchr(includeStart, '"');
            if (includeEnd != nullptr) {
                std::string includeString = std::filesystem::path(fileName).replace_filename(std::string(includeStart, includeEnd));

                std::ifstream file(includeString, std::ios::binary);
                if (file) {
                    std::vector<char> fileData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    file.close();

                    PreProcessor preprocessor;
                    preprocessor.CreateReferencePoint(1, includeString, 0);
                    preprocessor.HandleIncludes(fileData.data(), fileData.size(), includeString);
                    preprocessor.CreateReferencePoint(fileData.data(), fileData.size(), includeString, preprocessor.m_currentOffset);
                    size_t processedSize = preprocessor.GetProcessedBufferSize();
                    
                    std::vector<uint8_t> processedData(processedSize);
                    preprocessor.ExportProcessedBuffer(processedData.data());

                    m_buffer.Write(m_currentOffset, processedData.data(), processedSize);
                    m_currentOffset += processedSize;

                    preprocessor.m_referencePoints.Enumerate([&](ReferencePoint* ref) {
                        ref->offset += m_currentOffset - processedSize;
                        m_referencePoints.insert(ref);
                    });
                    preprocessor.m_referencePoints.clear();
                } else {
#ifdef ENABLE_CXX20_FORMAT
                    error(std::format("Could not open included file \"{}\": {}", includeString, strerror(errno)).c_str(), startRef->fileName, startRef->line);
#else
                    std::stringstream ss;
                    ss << "Could not open included file \"" << includeString << "\"";
                    ss << ": " << strerror(errno);
                    error(ss.str().c_str(), startRef->fileName, startRef->line);                    
#endif
                }
                i_source = includeEnd + 1;
                CreateReferencePoint(source, i_source - source, fileName.data(), m_currentOffset);
            } else {
                // need to find the previous reference point
                ReferencePoint* ref = nullptr;
                m_referencePoints.EnumerateReverse([&](ReferencePoint* currentRef) {
                    if (currentRef->offset < static_cast<size_t>(includeStart - source)) {
                        ref = currentRef;
                        return false;
                    }
                    return true;
                });
                if (ref == nullptr)
                    internal_error("Unable to find previous reference point whilst handling an include directive error.");
                error("Unterminated include directive", startRef->fileName, startRef->line);
            }
        }
    }
    // read the rest of the source
    if (includeEnd == nullptr)
        includeEnd = const_cast<char*>(source);
    else
        includeEnd += 1;

    m_buffer.Write(m_currentOffset, reinterpret_cast<const uint8_t*>(includeEnd), sourceSize - (includeEnd - source));
    m_currentOffset += sourceSize - (includeEnd - source);
}

size_t PreProcessor::GetLineCount(const char* src, const char* dst) {
    char const* lineStart = src;
    size_t line = 1;
    while (lineStart < dst) {
        lineStart = strchr(lineStart, '\n');
        if (lineStart == nullptr)
            break;
        lineStart += 1;
        line += 1;
    }
    return line;
}

PreProcessor::ReferencePoint* PreProcessor::CreateReferencePoint(const char* source, size_t sourceOffset, const std::string& fileName, size_t offset) {
    // need to get the line number using strchr
    char const* lineStart = source;
    size_t line = 1;
    while (lineStart < source + sourceOffset) {
        lineStart = strchr(lineStart, '\n');
        if (lineStart == nullptr)
            break;
        lineStart += 1;
        line += 1;
    }
    return CreateReferencePoint(line, fileName, offset);
}

PreProcessor::ReferencePoint* PreProcessor::CreateReferencePoint(size_t line, const std::string& fileName, size_t offset) {
    ReferencePoint* ref = new ReferencePoint{line, fileName, offset};
    m_referencePoints.insert(ref);
    return ref;
}

[[noreturn]] void PreProcessor::error(const char* message, const std::string& file, size_t line) {
    fprintf(stderr, "PreProcessor error at %s:%zu: %s\n", file.c_str(), line, message);
    exit(1);
}

[[noreturn]] void PreProcessor::internal_error(const char* message) {
    fprintf(stderr, "PreProcessor internal error: %s\n", message);
    exit(2);
}
