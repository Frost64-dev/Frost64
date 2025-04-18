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

#include <cstring>
#include <DataStructures/Bitmap.hpp>

Bitmap::Bitmap() : m_data(nullptr), m_size(0) {
}

Bitmap::Bitmap(uint64_t size) : m_data(nullptr), m_size(size) {
    m_data = new uint8_t[size >> 3];
}

Bitmap::~Bitmap() {
    delete[] m_data;
}

void Bitmap::Set(uint64_t index) {
    m_data[index >> 3] |= (1 << (index & 0x7));
}

void Bitmap::Clear(uint64_t index) {
    m_data[index >> 3] &= ~(1 << (index & 0x7));
}

bool Bitmap::Test(uint64_t index) const {
    return m_data[index >> 3] & (1 << (index & 0x7));
}

bool Bitmap::operator[](uint64_t index) const {
    return Test(index);
}

void Bitmap::ClearAll() {
    memset(m_data, 0, m_size >> 3);
}

void Bitmap::SetAll() {
    memset(m_data, 0xFF, m_size >> 3);
}

uint8_t* Bitmap::GetData() const {
    return m_data;
}

uint64_t Bitmap::GetSize() const {
    return m_size;
}

void Bitmap::Resize(uint64_t size) {
    delete[] m_data;
    m_size = size;
    if (size > 0)
        m_data = new uint8_t[size >> 3];
    else
        m_data = nullptr;
}
