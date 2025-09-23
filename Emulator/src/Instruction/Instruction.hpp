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

#ifndef _INSTRUCTION_HPP
#define _INSTRUCTION_HPP

#include <functional>

#include <MMU/MMU.hpp>

#include "Operand.hpp"


void InitInsCache(uint64_t startingIP, MMU* mmu);
void UpdateInsCacheMMU(MMU* mmu);
void InsCache_MaybeSetBaseAddress(uint64_t IP);

bool ExecuteInstruction(uint64_t IP);
void ExecutionLoop();
void StopExecution(void** state = nullptr); // If state is non-NULL, a new object of will be allocated with new, and deleted when parsed to the next AllowExecution call.
void AllowExecution(void** oldState = nullptr); // If oldState is non-NULL, it will be deleted after restoring the state.
void PauseExecution();
void AllowOneInstruction();
void AddBreakpoint(uint64_t address, std::function<void(uint64_t)> callback);
void RemoveBreakpoint(uint64_t address);

// return function pointer to instruction based on opcode, output argument count into argumentCount if non-null.
void* DecodeOpcode(uint8_t opcode, uint8_t* argumentCount);

void ins_add(Operand* dst, Operand* src);
void ins_sub(Operand* dst, Operand* src);
void ins_mul(Operand* dst2, Operand* dst1, Operand* src);
void ins_div(Operand* dst2, Operand* dst1, Operand* src);
void ins_smul(Operand* dst2, Operand* dst1, Operand* src);
void ins_sdiv(Operand* dst2, Operand* dst1, Operand* src);
void ins_or(Operand* dst, Operand* src);
void ins_nor(Operand* dst, Operand* src);
void ins_xor(Operand* dst, Operand* src);
void ins_xnor(Operand* dst, Operand* src);
void ins_and(Operand* dst, Operand* src);
void ins_nand(Operand* dst, Operand* src);
void ins_not(Operand* dst);
void ins_shl(Operand* dst, Operand* src);
void ins_shr(Operand* dst, Operand* src);
void ins_cmp(Operand* a, Operand* b);
void ins_inc(Operand* dst);
void ins_dec(Operand* dst);

void ins_ret();
void ins_call(Operand* dst);
void ins_jmp(Operand* dst);
void ins_jc(Operand* dst);
void ins_jnc(Operand* dst);
void ins_jz(Operand* dst);
void ins_jnz(Operand* dst);
void ins_jl(Operand* dst);
void ins_jle(Operand* dst);
void ins_jnl(Operand* dst);
void ins_jnle(Operand* dst);
void ins_jg(Operand* dst);
void ins_jge(Operand* dst);
void ins_jng(Operand* dst);
void ins_jnge(Operand* dst);

void ins_mov(Operand* dst, Operand* src);
void ins_nop();
void ins_hlt();
void ins_push(Operand* src);
void ins_pop(Operand* dst);
void ins_pusha();
void ins_popa();
void ins_int(Operand* number);
void ins_lidt(Operand* src);
void ins_iret();

void ins_syscall();
void ins_sysret();
void ins_enteruser(Operand* dst);

#endif /* _INSTRUCTION_HPP */