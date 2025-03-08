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

#ifndef _EXEC_FILE_HPP
#define _EXEC_FILE_HPP

#include <cstdint>
#include <cstdio>

#include <common/Data-structures/LinkedList.hpp>

#include <common/spinlock.h>

#include "ExecFormat.hpp"

namespace ExecFormat {

    class ExecFile {
    public:
        ExecFile();
        explicit ExecFile(const char* path);
        ~ExecFile();

        // File loading and saving API

        void Load();
        void Load(const char* path);

        void Save();
        void Save(const char* path);

        void SetPath(const char* path);
        [[nodiscard]] const char* GetPath() const;

        // Header API

        [[nodiscard]] ExecHeader GetHeader();
        
        // File section API

        void AddFileSection(const FileSectionEntry& entry);
        void RemoveFileSection(uint64_t index);
        [[nodiscard]] FileSectionEntry& GetFileSection(uint64_t index);

        [[nodiscard]] uint64_t GetFileSectionCount() const;

        // LOAD Info File Section API

        void AddLOADInfoFS(const LOADTableEntry& entry);
        void RemoveLOADInfoFS(uint64_t index);
        [[nodiscard]] LOADTableEntry& GetLOADInfoFS(uint64_t index);

        [[nodiscard]] uint64_t GetLOADInfoFSCount() const;

        // Out Segment Info File Section API

        void AddOutSegmentFS(const OutSegmentTableEntry& entry);
        void RemoveOutSegmentFS(uint64_t index);
        [[nodiscard]] OutSegmentTableEntry& GetOutSegmentFS(uint64_t index);

        [[nodiscard]] uint64_t GetOutSegmentFSCount() const;

        // Locking API

        void Lock();
        void Unlock();

        // Error handling API

        [[noreturn]] void EncodeError(const char* error);

    private:
        const char* m_Path;

        LinkedList::LockableLinkedList<FileSectionEntry> m_FileSections;
        LinkedList::LockableLinkedList<LOADTableEntry> m_LOADInfoFS;
        LinkedList::LockableLinkedList<OutSegmentTableEntry> m_OutSegmentFS;

        spinlock_t m_Lock;
    };
    
}

#endif /* _EXEC_FILE_HPP */