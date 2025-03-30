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

#ifndef _OS_SPECIFIC_FILE_HPP
#define _OS_SPECIFIC_FILE_HPP

#include <cstddef>

#ifdef __unix__
typedef int FileHandle_t;

#define FILE_HANDLE_TO_VOID_PTR(x) ((void*)(unsigned long)(x))
#define VOID_PTR_TO_FILE_HANDLE(x) ((FileHandle_t)(unsigned long)(x))
#endif /* __unix__ */

FileHandle_t GetFileHandleForStdIn();
FileHandle_t GetFileHandleForStdOut();
FileHandle_t GetFileHandleForStdErr();

FileHandle_t OpenFile(const char* path, bool create = false);
void CloseFile(FileHandle_t handle);
size_t GetFileSize(FileHandle_t handle);

size_t ReadFile(FileHandle_t handle, void* buffer, size_t size, size_t offset);
size_t WriteFile(FileHandle_t handle, const void* buffer, size_t size, size_t offset);

void* MapFile(FileHandle_t handle, size_t size, size_t offset);
void UnmapFile(void* address, size_t size);


#endif /* _OS_SPECIFIC_FILE_HPP */