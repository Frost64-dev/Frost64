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

#ifndef _STORAGE_DEVICE_FILE_HPP
#define _STORAGE_DEVICE_FILE_HPP

#include <OSSpecific/File.hpp>

class StorageFile {
public:
    explicit StorageFile(const char* path);
    ~StorageFile();

    void Initialise();
    void Destroy();

    void* GetData() const { return m_data; }
    size_t GetSize() const { return m_size; }

   private:
    const char* m_path;
    FileHandle_t m_handle;
    size_t m_size;
    void* m_data;
};

#endif /* _STORAGE_DEVICE_FILE_HPP */