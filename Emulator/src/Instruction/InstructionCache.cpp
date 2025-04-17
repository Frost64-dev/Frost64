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

#include "InstructionCache.hpp"

#include <cstring>

#include <MMU/MMU.hpp>

InstructionCache::InstructionCache() : m_cache{0}, m_cacheOffset(INSTRUCTION_CACHE_SIZE), m_mmu(nullptr), m_base_address(0) {

}

InstructionCache::~InstructionCache() {

}

void InstructionCache::Init(MMU* mmu, uint64_t base_address) {
    m_mmu = mmu;
    m_base_address = base_address;
    m_cacheOffset = 0;
    memset(m_cache, 0, INSTRUCTION_CACHE_SIZE);
    CacheMiss(0);
}

void InstructionCache::Write(uint64_t offset, const uint8_t* data, size_t size) {
    (void)offset;
    (void)data;
    (void)size;
}

void InstructionCache::Read(uint64_t offset, uint8_t* data, size_t size) const {
    (void)offset;
    (void)data;
    (void)size;
}

void InstructionCache::WriteStream(const uint8_t* data, size_t size) {
    (void)data;
    (void)size;
}

void InstructionCache::ReadStream(uint8_t* data, size_t size) const {
    (void)data;
    (void)size;
}

void InstructionCache::WriteStream8(uint8_t data) {
    if (m_cacheOffset >= INSTRUCTION_CACHE_SIZE)
        CacheMiss();

    m_cache[m_cacheOffset++] = data;
}

// ReadStream8 is in the header file as it is a very hot path

void InstructionCache::WriteStream16(uint16_t data) {
    if (m_cacheOffset + 2 > INSTRUCTION_CACHE_SIZE) {
        if (m_cacheOffset >= INSTRUCTION_CACHE_SIZE)
            CacheMiss();
        else
            CacheMiss(m_cacheOffset);
    }

    *reinterpret_cast<uint16_t*>(&m_cache[m_cacheOffset]) = data;
    m_cacheOffset += sizeof(data);
}

void InstructionCache::ReadStream16(uint16_t& data) {
    if (m_cacheOffset + 2 > INSTRUCTION_CACHE_SIZE) {
        if (m_cacheOffset >= INSTRUCTION_CACHE_SIZE)
            CacheMiss();
        else
            CacheMiss(m_cacheOffset);
    }

    data = *reinterpret_cast<uint16_t*>(&m_cache[m_cacheOffset]);
    m_cacheOffset += sizeof(data);
}

void InstructionCache::WriteStream32(uint32_t data) {
    if (m_cacheOffset + 4 > INSTRUCTION_CACHE_SIZE) {
        if (m_cacheOffset >= INSTRUCTION_CACHE_SIZE)
            CacheMiss();
        else
            CacheMiss(m_cacheOffset);
    }

    *reinterpret_cast<uint32_t*>(&m_cache[m_cacheOffset]) = data;
    m_cacheOffset += sizeof(data);
}

void InstructionCache::ReadStream32(uint32_t& data) {
    if (m_cacheOffset + 4 > INSTRUCTION_CACHE_SIZE) {
        if (m_cacheOffset >= INSTRUCTION_CACHE_SIZE)
            CacheMiss();
        else
            CacheMiss(m_cacheOffset);
    }

    data = *reinterpret_cast<uint32_t*>(&m_cache[m_cacheOffset]);
    m_cacheOffset += sizeof(data);
}

void InstructionCache::WriteStream64(uint64_t data) {
    if (m_cacheOffset + 8 > INSTRUCTION_CACHE_SIZE) {
        if (m_cacheOffset >= INSTRUCTION_CACHE_SIZE)
            CacheMiss();
        else
            CacheMiss(m_cacheOffset);
    }

    *reinterpret_cast<uint64_t*>(&m_cache[m_cacheOffset]) = data;
    m_cacheOffset += sizeof(data);
}

void InstructionCache::ReadStream64(uint64_t& data) {
    if (m_cacheOffset + 8 > INSTRUCTION_CACHE_SIZE) {
        if (m_cacheOffset >= INSTRUCTION_CACHE_SIZE)
            CacheMiss();
        else
            CacheMiss(m_cacheOffset);
    }

    data = *reinterpret_cast<uint64_t*>(&m_cache[m_cacheOffset]);
    m_cacheOffset += sizeof(data);
}

void InstructionCache::SeekStream(uint64_t offset) {
    if (offset >= INSTRUCTION_CACHE_SIZE)
        CacheMiss(offset - m_cacheOffset);
    else
        m_cacheOffset = static_cast<uint64_t>(offset);
}

void InstructionCache::UpdateMMU(MMU* mmu) {
    MMU* old_mmu = m_mmu;
    m_mmu = mmu;
    if (old_mmu != mmu)
        CacheMiss(0);
}

void InstructionCache::SetBaseAddress(uint64_t base_address) {
    m_base_address = base_address;
    CacheMiss(0);
}

void InstructionCache::MaybeSetBaseAddress(uint64_t base_address) {
    if (base_address < m_base_address || base_address > m_base_address + INSTRUCTION_CACHE_SIZE) {
        m_base_address = base_address;
        CacheMiss(0);
    }
    else {
        m_cacheOffset = base_address - m_base_address;
        if (m_cacheOffset >= INSTRUCTION_CACHE_SIZE)
            CacheMiss();
    }
}

uint64_t InstructionCache::GetBaseAddress() const {
    return m_base_address;
}

uint64_t InstructionCache::GetOffset() const {
    return m_base_address + m_cacheOffset;
}

void InstructionCache::CacheMiss(uint64_t offset) {
    m_cacheOffset = 0;
    m_base_address += offset;
    m_mmu->ReadBuffer(m_base_address, m_cache, INSTRUCTION_CACHE_SIZE);
}
