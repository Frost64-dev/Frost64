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

#ifndef _INSTRUCTION_CACHE_HPP
#define _INSTRUCTION_CACHE_HPP

#include <cstdint>

#include <MMU/MMU.hpp>

#include <Common/DataStructures/Buffer.hpp>

#define INSTRUCTION_CACHE_SIZE 256 // Size of the instruction cache in bytes

class InstructionCache : public StreamBuffer {
public:
    InstructionCache();
    ~InstructionCache();

    void Init(MMU* mmu, uint64_t base_address);

    // Write size bytes from data to the cache at offset
    void Write(uint64_t offset, const uint8_t* data, size_t size);

    // Read size bytes from the cache at offset to data
    void Read(uint64_t offset, uint8_t* data, size_t size) const;

    void WriteStream(const uint8_t* data, size_t size) override;
    void ReadStream(uint8_t* data, size_t size) const override;

    void WriteStream8(uint8_t data) override;
    [[gnu::always_inline]] inline void ReadStream8(uint8_t& data) override __attribute__((always_inline)) {
        if (__builtin_expect(m_cacheOffset >= INSTRUCTION_CACHE_SIZE, 0))
            CacheMiss();

        data = m_cache[m_cacheOffset++];
    }
    void WriteStream16(uint16_t data) override;
    void ReadStream16(uint16_t& data) override;
    void WriteStream32(uint32_t data) override;
    void ReadStream32(uint32_t& data) override;
    void WriteStream64(uint64_t data) override;
    void ReadStream64(uint64_t& data) override;

    void SeekStream(uint64_t offset) override;

    void UpdateMMU(MMU* mmu);

    void SetBaseAddress(uint64_t base_address);
    [[nodiscard]] uint64_t GetBaseAddress() const;
    
    // Set the base address if outside, otherwise just update the offset
    void MaybeSetBaseAddress(uint64_t base_address);

    [[nodiscard]] uint64_t GetOffset() const override;

private:
    [[gnu::cold]] void CacheMiss(uint64_t offset = INSTRUCTION_CACHE_SIZE);

private:
    uint8_t m_cache[INSTRUCTION_CACHE_SIZE]; // Instruction cache
    uint64_t m_cacheOffset; // Current offset in the instruction cache
    MMU* m_mmu; // MMU instance
    uint64_t m_base_address; // Base address of the cache
};

#endif /* _INSTRUCTION_CACHE_HPP */