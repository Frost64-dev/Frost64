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

#include "PhysicalRegionListBuffer.hpp"

PhysicalRegionListBuffer::PhysicalRegionListBuffer(StorageDevice* storageDevice, MMU* PhysicalMMU)
    : m_storageDevice(storageDevice), m_PhysicalMMU(PhysicalMMU), m_listStart(0), m_listNodeCount(0), m_listParsed(false), m_size(0) {
}

PhysicalRegionListBuffer::~PhysicalRegionListBuffer() {
}

bool PhysicalRegionListBuffer::ParseList() {
    if (m_listParsed)
        return true;

    uint64_t nodeStart = m_listStart;

    if (m_items.getCount() > 0) {
        const uint64_t itemCount = m_items.getCount();
        for (uint64_t i = 0; i < itemCount; i++) {
            Item* item = m_items.get(0);
            m_items.remove(item); // item deletion is faster than indexed deletion
            delete item;
        }
    }

    uint64_t currentSize = 0;

    for (uint64_t i = 0; i < m_listNodeCount; i++) {
        if (!m_PhysicalMMU->ValidateRead(nodeStart, 8))
            return false;
        uint64_t itemCount = m_PhysicalMMU->read64(nodeStart);
        nodeStart += 8;
        for (uint64_t j = 0; j < itemCount; j++) {
            if (!m_PhysicalMMU->ValidateRead(nodeStart, 16))
                return false;
            Item* item = new Item();
            item->start = m_PhysicalMMU->read64(nodeStart);
            item->size = m_PhysicalMMU->read64(nodeStart + 8);
            item->offset = currentSize;
            m_items.insert(item);
            currentSize += item->size << 9;
            nodeStart += 16;
        }
        if (!m_PhysicalMMU->ValidateRead(nodeStart, 8))
            return false;
        nodeStart = m_PhysicalMMU->read64(nodeStart);
    }

    if (currentSize != m_size)
        return false;

    m_listParsed = true;

    return true;
}

void PhysicalRegionListBuffer::ResetList(uint64_t listStart, uint64_t listNodeCount, uint64_t size) {
    if (m_items.getCount() > 0) {
        const uint64_t itemCount = m_items.getCount();
        for (uint64_t i = 0; i < itemCount; i++) {
            Item* item = m_items.get(0);
            m_items.remove(item); // item deletion is faster than indexed deletion
            delete item;
        }
    }

    m_listStart = listStart;
    m_listNodeCount = listNodeCount;
    m_listParsed = false;
    m_size = size;
}

void PhysicalRegionListBuffer::ClearList() {
    if (m_items.getCount() > 0) {
        const uint64_t itemCount = m_items.getCount();
        for (uint64_t i = 0; i < itemCount; i++) {
            Item* item = m_items.get(0);
            m_items.remove(item); // item deletion is faster than indexed deletion
            delete item;
        }
    }
    m_listStart = 0;
    m_listNodeCount = 0;
    m_listParsed = false;
    m_size = 0;
}

void PhysicalRegionListBuffer::Write(uint64_t offset, const uint8_t* data, size_t size) {
    if (offset + size > m_size)
        return;

    if (!m_listParsed)
        return;

    for (uint64_t i = 0; i < m_items.getCount(); i++) {
        if (Item* item = m_items.get(i); offset >= item->offset && offset < item->offset + item->size * 512) {
            uint64_t writeSize = size;
            if (offset + size > item->offset + item->size * 512)
                writeSize = item->offset + item->size * 512 - offset;
            m_PhysicalMMU->WriteBuffer(item->start + offset - item->offset, data, writeSize);
            data += writeSize;
            size -= writeSize;
            offset += writeSize;
        }
    }
}

void PhysicalRegionListBuffer::Read(uint64_t offset, uint8_t* data, size_t size) const {
    if (offset + size > m_size)
        return;

    if (!m_listParsed)
        return;

    for (uint64_t i = 0; i < m_items.getCount(); i++) {
        if (Item* item = m_items.get(i); offset >= item->offset && offset < item->offset + item->size * 512) {
            uint64_t readSize = size;
            if (offset + size > item->offset + item->size * 512)
                readSize = item->offset + item->size * 512 - offset;
            m_PhysicalMMU->ReadBuffer(item->start + offset - item->offset, data, readSize);
            data += readSize;
            size -= readSize;
            offset += readSize;
        }
    }
}

Buffer::Block* PhysicalRegionListBuffer::AddBlock(size_t) {
    return nullptr;
}

void PhysicalRegionListBuffer::DeleteBlock(uint64_t) {
}
