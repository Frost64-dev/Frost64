/*
Copyright (©) 2023-2026  Frosty515

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

namespace Emulator {
    struct CPUState;
}

struct CPUInsState;

bool InitInstructionSubsystem(Emulator::CPUState* cpu, uint64_t startingIP, MMU* mmu);

void InitInsCache(CPUInsState* state, uint64_t startingIP, MMU* mmu);
bool UpdateInsCacheMMU(Emulator::CPUState* cpu, MMU* mmu);
void InsCache_MaybeSetBaseAddress(Emulator::CPUState* cpu, uint64_t IP);


void AddBreakpoint(Emulator::CPUState* cpu, uint64_t address, std::function<void(uint64_t)> callback);
void RemoveBreakpoint(Emulator::CPUState* cpu, uint64_t address);

void AllowExecution(Emulator::CPUState* cpu, void** oldState = nullptr); // If oldState is non-NULL, it will be deleted after restoring the state.
void AllowOneInstruction(Emulator::CPUState* cpu);
void PauseExecution(Emulator::CPUState* cpu);
void StopExecution(Emulator::CPUState* cpu, void** state = nullptr); // If state is non-NULL, a new object of will be allocated with new, and deleted when parsed to the next AllowExecution call.
void SwitchExecution(Emulator::CPUState* cpu, uint64_t IP);
void InsRaiseInterrupt(Emulator::CPUState* cpu, uint8_t interrupt);

void StartExecution(Emulator::CPUState* cpu);
void ExecutionLoop(Emulator::CPUState* cpu);

// return function pointer to instruction based on opcode, output argument count into argumentCount if non-null.
void* DecodeOpcode(uint8_t opcode, uint8_t* argumentCount);

void ins_add(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_adc(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_sub(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_sbb(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_mul(Emulator::CPUState* cpu, Operand* dst2, Operand* dst1, Operand* src);
void ins_div(Emulator::CPUState* cpu, Operand* dst2, Operand* dst1, Operand* src);
void ins_smul(Emulator::CPUState* cpu, Operand* dst2, Operand* dst1, Operand* src);
void ins_sdiv(Emulator::CPUState* cpu, Operand* dst2, Operand* dst1, Operand* src);
void ins_or(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_nor(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_xor(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_xnor(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_and(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_nand(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_shl(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_shr(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_not(Emulator::CPUState* cpu, Operand* dst);
void ins_inc(Emulator::CPUState* cpu, Operand* dst);
void ins_dec(Emulator::CPUState* cpu, Operand* dst);
void ins_cmp(Emulator::CPUState* cpu, Operand* a, Operand* b);

void ins_ret(Emulator::CPUState* cpu);
void ins_call(Emulator::CPUState* cpu, Operand* dst);
void ins_jmp(Emulator::CPUState* cpu, Operand* dst);
void ins_jc(Emulator::CPUState* cpu, Operand* dst);
void ins_jnc(Emulator::CPUState* cpu, Operand* dst);
void ins_jz(Emulator::CPUState* cpu, Operand* dst);
void ins_jnz(Emulator::CPUState* cpu, Operand* dst);
void ins_jl(Emulator::CPUState* cpu, Operand* dst);
void ins_jle(Emulator::CPUState* cpu, Operand* dst);
void ins_jnl(Emulator::CPUState* cpu, Operand* dst);
void ins_jnle(Emulator::CPUState* cpu, Operand* dst);
void ins_jo(Emulator::CPUState* cpu, Operand* dst);
void ins_jno(Emulator::CPUState* cpu, Operand* dst);
void ins_js(Emulator::CPUState* cpu, Operand* dst);
void ins_jns(Emulator::CPUState* cpu, Operand* dst);

void ins_setc(Emulator::CPUState* cpu, Operand* dst);
void ins_setnc(Emulator::CPUState* cpu, Operand* dst);
void ins_setz(Emulator::CPUState* cpu, Operand* dst);
void ins_setnz(Emulator::CPUState* cpu, Operand* dst);
void ins_setl(Emulator::CPUState* cpu, Operand* dst);
void ins_setle(Emulator::CPUState* cpu, Operand* dst);
void ins_setnl(Emulator::CPUState* cpu, Operand* dst);
void ins_setnle(Emulator::CPUState* cpu, Operand* dst);
void ins_seto(Emulator::CPUState* cpu, Operand* dst);
void ins_setno(Emulator::CPUState* cpu, Operand* dst);
void ins_sets(Emulator::CPUState* cpu, Operand* dst);
void ins_setns(Emulator::CPUState* cpu, Operand* dst);

void ins_movc(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_movnc(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_movz(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_movnz(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_movl(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_movle(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_movnl(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_movnle(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_movo(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_movno(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_movs(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_movns(Emulator::CPUState* cpu, Operand* dst, Operand* src);

void ins_mov(Emulator::CPUState* cpu, Operand* dst, Operand* src);
void ins_nop(Emulator::CPUState* cpu);
void ins_hlt(Emulator::CPUState* cpu);
void ins_push(Emulator::CPUState* cpu, Operand* src);
void ins_pop(Emulator::CPUState* cpu, Operand* dst);
void ins_pusha(Emulator::CPUState* cpu);
void ins_popa(Emulator::CPUState* cpu);
void ins_int(Emulator::CPUState* cpu, Operand* number);
void ins_lidt(Emulator::CPUState* cpu, Operand* src);
void ins_iret(Emulator::CPUState* cpu);

void ins_syscall(Emulator::CPUState* cpu);
void ins_sysret(Emulator::CPUState* cpu);
void ins_enteruser(Emulator::CPUState* cpu, Operand* dst);

#endif /* _INSTRUCTION_HPP */