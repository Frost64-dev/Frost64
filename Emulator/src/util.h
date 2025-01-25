/*
Copyright (©) 2023-2024  Frosty515

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

#ifndef _EMULATOR_UTIL_H
#define _EMULATOR_UTIL_H

#include <stdint.h>
#include <stddef.h>

#define KiB(x) ((uint64_t)x * 1024)
#define MiB(x) (KiB(x) * 1024)
#define GiB(x) (MiB(x) * 1024)

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

#define IN_BOUNDS(VALUE, MIN, MAX) ((VALUE >= MIN) && (VALUE <= MAX))

#define FLAG_SET(x, flag) (x |= flag)
#define FLAG_UNSET(x, flag) (x &= ~flag)

#define MAX(a, b) _util_max(a, b)
#define MIN(a, b) _util_min(a, b)

#ifdef __cplusplus
#define CMP16_B(a, byte, result) { \
    uint64_t _cmp16_b_wide = (uint64_t)byte * 0x0101010101010101; \
    uint64_t* _cmp16_b_list = reinterpret_cast<uint64_t*>(a); \
    result = _cmp16_b_list[0] == _cmp16_b_wide && _cmp16_b_list[1] == _cmp16_b_wide; \
}
#else
#define CMP16_B(a, byte, result) { \
    uint64_t _cmp16_b_wide = (uint64_t)byte * 0x0101010101010101; \
    uint64_t* _cmp16_b_list = (uint64_t*)a; \
    result = _cmp16_b_list[0] == _cmp16_b_wide && _cmp16_b_list[1] == _cmp16_b_wide; \
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Implemented in x86_64 assembly */

#ifdef __APPLE__

void* fast_memset(void* ptr, uint8_t c, size_t n);
void* fast_memcpy(void* dst, void* src, size_t n);

#else /* __APPLE__ */

#define fast_memset(ptr, c, n) _fast_memset(ptr, c, n)
#define fast_memcpy(dst, src, n) _fast_memcpy(dst, src, n)

void* _fast_memset(void* ptr, uint8_t c, size_t n);
void* _fast_memcpy(void* dst, void* src, size_t n);

#endif /* __APPLE__ */

/* Implemented in C */

uint64_t _util_max(uint64_t a, uint64_t b);
uint64_t _util_min(uint64_t a, uint64_t b);

#ifdef __cplusplus
}
#endif

#endif /* _EMULATOR_UTIL_H */
