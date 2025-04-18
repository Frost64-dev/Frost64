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

#ifndef _BITMAP_HPP
#define _BITMAP_HPP

#include <cstdint>

class Bitmap {
   public:
    Bitmap();
    explicit Bitmap(uint64_t size); // size is in bits
    ~Bitmap();

    void Set(uint64_t index);
    void Clear(uint64_t index);
    [[nodiscard]] bool Test(uint64_t index) const;

    bool operator[](uint64_t index) const;

    void ClearAll();
    void SetAll();

    [[nodiscard]] uint8_t* GetData() const;
    [[nodiscard]] uint64_t GetSize() const; // in bits

    void Resize(uint64_t size); // size is in bits

   private:
    uint8_t* m_data;
    uint64_t m_size;
};

#endif /* _BITMAP_HPP */