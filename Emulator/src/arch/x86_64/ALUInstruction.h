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

#ifndef _X86_64_ALU_INSTRUCTION_HPP
#define _X86_64_ALU_INSTRUCTION_HPP

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: This enum should be somewhere else
enum FLAGS_OFFSETS {
    FO_CARRY = 0,
    FO_ZERO = 1,
    FO_SIGN = 2,
    FO_INTMODE = 3,
    FO_IO_WRITE = 4,
    FO_IO_READ = 5
};

#include <stdint.h>

uint64_t x86_64_add(uint64_t a, uint64_t b, uint64_t* flags);
uint64_t x86_64_mul(uint64_t a, uint64_t b, uint64_t* flags);
uint64_t x86_64_sub(uint64_t a, uint64_t b, uint64_t* flags);
uint64_t x86_64_div(uint64_t a, uint64_t b, uint64_t* flags);
uint64_t x86_64_or(uint64_t a, uint64_t b, uint64_t* flags);
uint64_t x86_64_xor(uint64_t a, uint64_t b, uint64_t* flags);
uint64_t x86_64_nor(uint64_t a, uint64_t b, uint64_t* flags);
uint64_t x86_64_and(uint64_t a, uint64_t b, uint64_t* flags);
uint64_t x86_64_nand(uint64_t a, uint64_t b, uint64_t* flags);
uint64_t x86_64_not(uint64_t src, uint64_t* flags);
void x86_64_cmp(uint64_t a, uint64_t b, uint64_t* flags);
uint64_t x86_64_inc(uint64_t src, uint64_t* flags);
uint64_t x86_64_dec(uint64_t src, uint64_t* flags);
uint64_t x86_64_shl(uint64_t a, uint64_t b, uint64_t* flags);
uint64_t x86_64_shr(uint64_t a, uint64_t b, uint64_t* flags);

#ifdef __cplusplus
}
#endif

#endif /* _X86_64_ALU_INSTRUCTION_HPP */