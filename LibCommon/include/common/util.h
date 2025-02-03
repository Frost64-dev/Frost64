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

#ifndef _EMULATOR_UTIL_H
#define _EMULATOR_UTIL_H

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

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define PAGE_SIZE 4'096

#define IS_POWER_OF_TWO(x) ((x & (x - 1)) == 0 && x != 0)

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

#endif /* _EMULATOR_UTIL_H */
