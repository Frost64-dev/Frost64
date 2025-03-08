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

#include <ExecFile.hpp>
#include <cstdlib>
#include "ExecFormat.hpp"
#include "common/spinlock.h"

namespace ExecFormat {

    ExecFile::ExecFile() : m_Path(nullptr), m_Lock(0) {

    }

    ExecFile::ExecFile(const char* path) : m_Path(path), m_Lock(0) {

    }

    ExecFile::~ExecFile() {
        
    }

    void ExecFile::Load() {
    
    }

    void ExecFile::Load(const char* path) {
        m_Path = path;
        Load();
    }

    void ExecFile::Save() {
        FILE* file = fopen(m_Path, "wb");
        if (file == nullptr)
            EncodeError("Failed to open file for writing");

        ExecHeader header = GetHeader();
        if (1 != fwrite(&header, sizeof(ExecHeader), 1, file))
            EncodeError("Failed to write header");

        uint64_t currentFSTE_DOffset = sizeof(ExecHeader) + m_FileSections.getCount() * sizeof(FileSectionEntry);

        for (uint64_t i = 0; i < m_FileSections.getCount(); i++) {
            FileSectionEntry* entry = m_FileSections.get(i);
            entry->Offset = currentFSTE_DOffset;
            currentFSTE_DOffset += entry->Size;

            uint64_t EntrySize = sizeof(FileSectionEntry);

            switch (entry->Type) {
                case (uint16_t)FileSectionType::LOAD_Info:
                    EntrySize += sizeof(LOADTableEntry);
                    break;
                case (uint16_t)FileSectionType::OutSegment_Info:
                    EntrySize += sizeof(OutSegmentTableEntry);
                    break;
                default:
                    EncodeError("Unknown file section type");
            }

            if (1 != fwrite(entry, EntrySize, 1, file))
                EncodeError("Failed to write file section entry");
        }

        for (uint64_t i = 0; i < m_FileSections.getCount(); i++) {
            FileSectionEntry* entry = m_FileSections.get(i);
            
            switch (entry->Type) {
                case (uint16_t)FileSectionType::LOAD_Info: {
                    for (uint64_t j = 0; j < m_LOADInfoFS.getCount(); j++) {
                        LOADTableEntry* loadEntry = m_LOADInfoFS.get(j);
                        if (1 != fwrite(loadEntry, sizeof(LOADTableEntry), 1, file))
                            EncodeError("Failed to write LOAD Info FS entry");
                    }
                    break;
                }
                case (uint16_t)FileSectionType::OutSegment_Info: {
                    OutSegmentTableEntry* outSegmentEntry = m_OutSegmentFS.get(i);
                    if (1 != fwrite(outSegmentEntry, sizeof(OutSegmentTableEntry), 1, file))
                        EncodeError("Failed to write Out Segment Info FS entry");
                    break;
                }
                default:
                    EncodeError("Unknown file section type");
            }
        }
    }

    void ExecFile::Save(const char* path) {
        m_Path = path;
        Save();
    }

    void ExecFile::SetPath(const char* path) {
        m_Path = path;
    }

    const char* ExecFile::GetPath() const {
        return m_Path;
    }

    ExecHeader ExecFile::GetHeader() {
        ExecHeader header = {
            .Magic = ExecFormatMagic,
            .Version = 1,
            .ABI = 0,
            .Arch = 0,
            .Type = 0,
            .Flags = 0,
            .Align0 = 0,
            .FSecTS = 0x28,
            .FSecNum = m_FileSections.getCount()
        };
        return header;
    }

    void ExecFile::AddFileSection(const FileSectionEntry& entry) {
        m_FileSections.insert(new FileSectionEntry(entry));
    }

    void ExecFile::RemoveFileSection(uint64_t index) {
        FileSectionEntry* entry = m_FileSections.get(index);
        m_FileSections.remove(index);
        delete entry;
    }

    FileSectionEntry& ExecFile::GetFileSection(uint64_t index) {
        return *m_FileSections.get(index);
    }

    uint64_t ExecFile::GetFileSectionCount() const {
        return m_FileSections.getCount();
    }

    void ExecFile::AddLOADInfoFS(const LOADTableEntry& entry) {
        m_LOADInfoFS.insert(new LOADTableEntry(entry));
    }

    void ExecFile::RemoveLOADInfoFS(uint64_t index) {
        LOADTableEntry* entry = m_LOADInfoFS.get(index);
        m_LOADInfoFS.remove(index);
        delete entry;
    }

    LOADTableEntry& ExecFile::GetLOADInfoFS(uint64_t index) {
        return *m_LOADInfoFS.get(index);
    }

    uint64_t ExecFile::GetLOADInfoFSCount() const {
        return m_LOADInfoFS.getCount();
    }

    void ExecFile::AddOutSegmentFS(const OutSegmentTableEntry& entry) {
        m_OutSegmentFS.insert(new OutSegmentTableEntry(entry));
    }

    void ExecFile::RemoveOutSegmentFS(uint64_t index) {
        OutSegmentTableEntry* entry = m_OutSegmentFS.get(index);
        m_OutSegmentFS.remove(index);
        delete entry;
    }

    OutSegmentTableEntry& ExecFile::GetOutSegmentFS(uint64_t index) {
        return *m_OutSegmentFS.get(index);
    }

    uint64_t ExecFile::GetOutSegmentFSCount() const {
        return m_OutSegmentFS.getCount();
    }

    void ExecFile::Lock() {
        spinlock_acquire(&m_Lock);
    }

    void ExecFile::Unlock() {
        spinlock_release(&m_Lock);
    }

    [[noreturn]] void ExecFile::EncodeError(const char* error) {
        printf("Exec file encoding error: %s\n", error);
        exit(1);
    }

}