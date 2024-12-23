/*
Copyright (©) 2022-2024  Frosty515

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

#ifndef _UTIL_H
#define _UTIL_H

#include <stddef.h>
#include <stdint.h>

#define KiB(x) ((uint64_t)x * (uint64_t)1'024)
#define MiB(x) (KiB(x) * (uint64_t)1'024)
#define GiB(x) (MiB(x) * (uint64_t)1'024)

#ifdef __cplusplus
extern "C" {
#endif

#define BCD_TO_BINARY(x) (((x & 0xF0) >> 1) + ((x & 0xF0) >> 3) + (x & 0xF))

#define DIV_ROUNDUP(VALUE, DIV) (((VALUE) + ((DIV) - 1)) / (DIV))
#define DIV_ROUNDUP_ADDRESS(ADDR, DIV) ((void*)DIV_ROUNDUP(((unsigned long)(ADDR)), DIV))

#define ALIGN_UP(VALUE, ALIGN) (DIV_ROUNDUP(VALUE, ALIGN) * (ALIGN))
#define ALIGN_UP_BASE2(VALUE, ALIGN) (((VALUE) + ((ALIGN) - 1)) & ~((ALIGN) - 1))

#define ALIGN_UP_ADDRESS(ADDR, ALIGN) ((void*)ALIGN_UP(((unsigned long)(ADDR)), ALIGN))
#define ALIGN_UP_ADDRESS_BASE2(ADDR, ALIGN) ((void*)ALIGN_UP_BASE2(((unsigned long)(ADDR)), ALIGN))

#define ALIGN_DOWN(VALUE, ALIGN) (((VALUE) / (ALIGN)) * (ALIGN))
#define ALIGN_DOWN_BASE2(VALUE, ALIGN) ((VALUE) & ~((ALIGN) - 1))

#define ALIGN_DOWN_ADDRESS(ADDR, ALIGN) ((void*)ALIGN_DOWN(((unsigned long)ADDR), ALIGN))
#define ALIGN_DOWN_ADDRESS_BASE2(ADDR, ALIGN) ((void*)ALIGN_DOWN_BASE2(((unsigned long)ADDR), ALIGN))

#define PAGE_SIZE 4'096

#define IS_POWER_OF_TWO(x) ((x & (x - 1)) == 0 && x != 0)

#ifdef __APPLE__

void* fast_memset(void* dst, const uint8_t value, const size_t n);
void* fast_memcpy(void* dst, const void* src, const size_t n);

#else /* __APPLE__ */

#define fast_memset(dst, value, n) _fast_memset(dst, value, n)
#define fast_memcpy(dst, src, n) _fast_memcpy(dst, src, n)

void* _fast_memset(void* dst, const uint8_t value, const size_t n);
void* _fast_memcpy(void* dst, const void* src, const size_t n);

#endif /* __APPLE__ */

#define FLAG_SET(x, flag) x |= (flag)
#define FLAG_UNSET(x, flag) x &= ~(flag)

#ifdef __cplusplus
}
#endif

#endif /* _UTIL_H */