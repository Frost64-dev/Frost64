/*
Copyright (©) 2023-2025  Frosty515

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

#include <Util.hpp>

#include <cstring>
#include <DataStructures/Buffer.hpp>

Buffer::Buffer()
    : m_size(0), m_blockSize(DEFAULT_BUFFER_BLOCK_SIZE) {
}

Buffer::Buffer(size_t size, size_t blockSize)
    : m_size(0), m_blockSize(blockSize), m_blocks() {
    Buffer::AddBlock(ALIGN_UP(size, m_blockSize));
}

Buffer::~Buffer() {
    for (uint64_t i = 0; i < m_blocks.getCount(); i++) {
        Block* block = m_blocks.get(0);
        delete[] block->data;
        m_blocks.remove(UINT64_C(0));
        delete block;
    }
}

void Buffer::Write(uint64_t offset, const uint8_t* data, size_t size) {
    Block* starting_block = nullptr;
    uint64_t starting_block_offset = 0;
    uint64_t starting_block_index = 0;
    uint64_t offset_within_block = 0;
    uint8_t const* i_data = data;
    uint64_t i = 0;
    m_blocks.Enumerate([&](Block* block, uint64_t index) -> bool {
        i = index;
        if (offset >= starting_block_offset && offset < (starting_block_offset + block->size)) {
            starting_block = block;
            offset_within_block = offset - starting_block_offset;
            starting_block_index = i;
            return false;
        }
        starting_block_offset += block->size;
        return true;
    });
    if (starting_block == nullptr) {
        starting_block = AddBlock(ALIGN_UP(size, m_blockSize));
        starting_block_offset = m_size - size;
        starting_block_index = i + 1;
        offset_within_block = 0;
    }
    if ((starting_block->size - offset_within_block) >= size) {
        memcpy(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(starting_block->data) + offset_within_block), i_data, size);
        starting_block->empty = false;
        return;
    } else {
        memcpy(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(starting_block->data) + offset_within_block), i_data, starting_block->size - offset_within_block);
        size -= starting_block->size - offset_within_block;
        i_data = reinterpret_cast<uint8_t const*>(reinterpret_cast<uint64_t>(i_data) + (starting_block->size - offset_within_block));
        m_blocks.Enumerate([&](Block* block, uint64_t index) -> bool {
            i = index;
            if (size <= block->size) {
                memcpy(block->data, i_data, size);
                block->empty = false;
                return false;
            } else {
                memcpy(block->data, i_data, block->size);
                block->empty = false;
                size -= block->size;
                i_data = reinterpret_cast<uint8_t const*>(reinterpret_cast<uint64_t>(i_data) + block->size);
                return true;
            }
        }, starting_block_index + 1);
        Block* block = AddBlock(ALIGN_UP(size, m_blockSize));
        block->empty = false;
        memcpy(block->data, i_data, size);
        return;
    }
}

void Buffer::Read(uint64_t offset, uint8_t* data, size_t size) const {
    Block* starting_block = nullptr;
    uint64_t starting_block_offset = 0;
    uint64_t starting_block_index = 0;
    uint64_t offset_within_block = 0;
    uint8_t* i_data = data;
    uint64_t i = 0;
    m_blocks.Enumerate([&](Block* block, uint64_t index) -> bool {
        i = index;
        if (offset >= starting_block_offset && offset < (starting_block_offset + block->size)) {
            starting_block = block;
            offset_within_block = offset - starting_block_offset;
            starting_block_index = i;
            return false;
        }
        starting_block_offset += block->size;
        return true;
    });
    if (starting_block == nullptr)
        return;
    if ((starting_block->size - offset_within_block) >= size) {
        memcpy(i_data, reinterpret_cast<void*>(reinterpret_cast<uint64_t>(starting_block->data) + offset_within_block), size);
        return;
    } else {
        memcpy(i_data, reinterpret_cast<void*>(reinterpret_cast<uint64_t>(starting_block->data) + offset_within_block), starting_block->size - offset_within_block);
        size -= starting_block->size - offset_within_block;
        i_data = reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(i_data) + (starting_block->size - offset_within_block));
        m_blocks.Enumerate([&](Block* block, uint64_t index) -> bool {
            i = index;
            if (size <= block->size) {
                memcpy(i_data, block->data, size);
                return false;
            } else {
                memcpy(i_data, block->data, block->size);
                size -= block->size;
                i_data = reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(i_data) + block->size);
                return true;
            }
        }, starting_block_index + 1);
    }
}

void Buffer::Clear(uint64_t offset, size_t size) {
    Block* starting_block = nullptr;
    uint64_t starting_block_offset = 0;
    uint64_t starting_block_index = 0;
    uint64_t offset_within_block = 0;
    uint64_t i = 0;
    m_blocks.Enumerate([&](Block* block, uint64_t index) -> bool {
        i = index;
        if (offset >= starting_block_offset && offset < (starting_block_offset + block->size)) {
            starting_block = block;
            offset_within_block = offset - starting_block_offset;
            starting_block_index = i;
            return false;
        }
        starting_block_offset += block->size;
        return true;
    });
    if (starting_block == nullptr)
        return;
    if ((starting_block->size - offset_within_block) <= size) {
        memset(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(starting_block->data) + offset_within_block), 0, size);
        if (offset_within_block == 0) {
            starting_block->empty = true;
            AutoShrink();
        }
        return;
    } else {
        memset(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(starting_block->data) + offset_within_block), 0, starting_block->size - offset_within_block);
        size -= starting_block->size - offset_within_block;
        m_blocks.Enumerate([&](Block* block, uint64_t index) -> bool {
            i = index;
            if (size <= block->size) {
                memset(block->data, 0, size);
                if (block->size == size) {
                    block->empty = true;
                    AutoShrink();
                }
                return false;
            } else {
                memset(block->data, 0, block->size);
                block->empty = true;
                size -= block->size;
                return true;
            }
        }, starting_block_index + 1);
    }
}

void Buffer::Clear() {
    const uint64_t count = m_blocks.getCount();
    for (uint64_t i = 0; i < count; i++)
        DeleteBlock(0);
}

void Buffer::AutoShrink() {
    m_blocks.EnumerateReverse([&](Block* block, uint64_t index) -> bool {
        if (block->empty) {
            delete[] block->data;
            m_blocks.remove(index);
            m_size -= block->size;
            delete block;
            return true;
        }
        return false;
    });
}

uint64_t Buffer::ClearUntil(uint64_t offset) {
    uint64_t blocksDeleted = 0;
    for (uint64_t i = 0; i < m_blocks.getCount() && offset > 0; i++) {
        if (Block* block = m_blocks.get(0); offset >= block->size) {
            DeleteBlock(0);
            blocksDeleted++;
            offset -= block->size;
        } else {
            memset(block->data, 0, offset);
            block->empty = false;
            return blocksDeleted;
        }
    }
    return blocksDeleted;
}

size_t Buffer::GetSize() const {
    return m_size;
}

Buffer::Block* Buffer::AddBlock(size_t size) {
    Block* block = new Block;
    block->data = new uint8_t[size];
    block->size = size;
    block->empty = true;
    m_blocks.insert(block);
    m_size += size;
    return block;
}

void Buffer::DeleteBlock(uint64_t index) {
    Block* block = m_blocks.get(index);
    m_blocks.remove(index);
    delete[] block->data;
    m_size -= block->size;
    delete block;
}

void StreamBuffer::WriteStream8(uint8_t data) {
    WriteStream(&data, sizeof(data));
}

void StreamBuffer::ReadStream8(uint8_t& data) {
    ReadStream(&data, sizeof(data));
}

void StreamBuffer::WriteStream16(uint16_t data) {
    WriteStream(reinterpret_cast<uint8_t*>(&data), sizeof(data));
}

void StreamBuffer::ReadStream16(uint16_t& data) {
    ReadStream(reinterpret_cast<uint8_t*>(&data), sizeof(data));
}

void StreamBuffer::WriteStream32(uint32_t data) {
    WriteStream(reinterpret_cast<uint8_t*>(&data), sizeof(data));
}

void StreamBuffer::ReadStream32(uint32_t& data) {
    ReadStream(reinterpret_cast<uint8_t*>(&data), sizeof(data));
}

void StreamBuffer::WriteStream64(uint64_t data) {
    WriteStream(reinterpret_cast<uint8_t*>(&data), sizeof(data));
}

void StreamBuffer::ReadStream64(uint64_t& data) {
    ReadStream(reinterpret_cast<uint8_t*>(&data), sizeof(data));
}
