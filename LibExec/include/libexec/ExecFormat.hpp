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

#ifndef _EXEC_FORMAT_HPP
#define _EXEC_FORMAT_HPP

#include <cstdint>

namespace ExecFormat {

    constexpr char ExecFormatMagicSTR[8] = { 'F', 'R', 'O', 'S', 'T', 'E', 'X', 'E' };
    constexpr uint64_t ExecFormatMagic = 0x45584554534f5246; // 'FROSTEXE' in hexadecimal

    struct [[gnu::packed]] ExecHeader {
        uint64_t Magic;
        uint64_t Version;
        uint8_t ABI;
        uint8_t Arch;
        uint8_t Type;
        uint8_t Flags;
        uint32_t Align0;
        uint64_t FSecTS; // File Section Table Start
        uint64_t FSecNum; // The number of file sections
    };

    struct [[gnu::packed]] FileSectionEntry {
        uint64_t Offset;
        uint64_t Size;
        uint16_t Type;
        uint8_t Flags;
        uint8_t Align0[5];
    };

    enum class FileSectionType : uint8_t {
        LOAD_Info = 0,
        DynLink_Info = 1,
        SymbolTable = 2,
        OutSegment_Info = 3,
        Debug_Info = 4,
        FileStoreTable = 5,
        General_Info = 6
    };

    struct [[gnu::packed]] LOAD_Info_FSHeader {
        FileSectionEntry CommonHeader;
        uint64_t Count;
    };

    struct [[gnu::packed]] OutSegment_FSHeader {
        FileSectionEntry CommonHeader;
        uint64_t Count;
    };

    struct [[gnu::packed]] GeneralInfo_FSHeader {
        FileSectionEntry CommonHeader;
        uint64_t Entry;
    };

    struct [[gnu::packed]] LOADTableEntry {
        uint64_t FileOffset;
        uint64_t FileSize;
        uint64_t MemOffset;
        uint64_t MemSize;
        uint8_t Flags;
        uint8_t Align0[7];
    };

    enum class LOADTableFlags : uint8_t {
        Read = 1,
        Write = 2,
        Execute = 4
    };

    struct [[gnu::packed]] OutSegmentTableEntry {
        uint64_t Offset;
        uint64_t Size;
        char Name[32];
    };
}

#endif /* _EXEC_FORMAT_HPP */