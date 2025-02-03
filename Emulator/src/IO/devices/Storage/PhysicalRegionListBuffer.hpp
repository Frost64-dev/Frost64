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

#ifndef _PHYSICAL_REGION_LIST_BUFFER_HPP
#define _PHYSICAL_REGION_LIST_BUFFER_HPP

#include <common/Data-structures/Buffer.hpp>
#include <common/Data-structures/LinkedList.hpp>

#include <MMU/MMU.hpp>

class StorageDevice;

class PhysicalRegionListBuffer : public Buffer {
   public:
    PhysicalRegionListBuffer(StorageDevice* storageDevice, MMU* PhysicalMMU);
    ~PhysicalRegionListBuffer() override;

    bool ParseList();
    void ResetList(uint64_t listStart, uint64_t listNodeCount, uint64_t size);
    void ClearList();

    // Write size bytes from data to the buffer at offset
    void Write(uint64_t offset, const uint8_t* data, size_t size) override;

    // Read size bytes from the buffer at offset to data
    void Read(uint64_t offset, uint8_t* data, size_t size) const override;

   protected:
    struct Item {
        uint64_t start;
        uint64_t size;
        uint64_t offset;
    };

    Block* AddBlock(size_t size) override;
    void DeleteBlock(uint64_t index) override;

   private:
    StorageDevice* m_storageDevice;
    MMU* m_PhysicalMMU;
    uint64_t m_listStart;
    uint64_t m_listNodeCount;
    LinkedList::RearInsertLinkedList<Item> m_items;
    bool m_listParsed;
    uint64_t m_size;
};

#endif /* _PHYSICAL_REGION_LIST_BUFFER_HPP */