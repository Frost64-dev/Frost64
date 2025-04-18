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

#include "StorageFile.hpp"

StorageFile::StorageFile(const char* path) : m_path(path), m_handle(0), m_size(0), m_data(nullptr) {

}

StorageFile::~StorageFile() {
    if (m_data != nullptr)
        Destroy();
}

void StorageFile::Initialise() {
    m_handle = OpenFile(m_path);
    m_size = GetFileSize(m_handle);
    m_data = MapFile(m_handle, m_size, 0);
}

void StorageFile::Destroy() {
    UnmapFile(m_data, m_size);
    CloseFile(m_handle);
    m_data = nullptr;
    m_size = 0;
    m_handle = 0;
}
