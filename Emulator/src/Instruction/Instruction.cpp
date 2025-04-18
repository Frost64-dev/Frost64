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

#include "Instruction.hpp"
#include "InstructionCache.hpp"

#include <atomic>
#include <utility>

#ifdef EMULATOR_DEBUG
#include <cstdio>
#endif

#include <Emulator.hpp>
#include <Exceptions.hpp>
#include <Interrupts.hpp>
#include <Stack.hpp>

#include <IO/IOBus.hpp>

#include <MMU/MMU.hpp>

#include <Common/Spinlock.hpp>

#include <LibArch/Instruction.hpp>
#include <LibArch/Operand.hpp>

std::atomic_uchar g_ExecutionAllowed = 1;
std::atomic_uchar g_ExecutionRunning = 0;
std::atomic_uchar g_TerminateExecution = 0;
std::atomic_uchar g_AllowOneInstruction = 0;

InsEncoding::SimpleInstruction g_current_instruction;

Operand g_currentOperands[2];
ComplexData g_complex[2];

std::unordered_map<uint64_t, std::function<void(uint64_t)>> g_breakpoints;
spinlock_new(g_breakpointsLock);
std::atomic_uchar g_breakpointsEnabled = 0;
std::pair<uint64_t, std::function<void(uint64_t)>> g_CurrentBreakpoint;
bool g_breakpointHit = false;

InstructionCache g_insCache;

struct InstructionExecutionRunState {
    bool Allowed;
    bool Running;
    bool Terminate;
    bool AllowOne;
};

void InitInsCache(uint64_t starting_IP, MMU *mmu) {
    g_insCache.Init(mmu, starting_IP);
}

void UpdateInsCacheMMU(MMU *mmu) {
    g_insCache.UpdateMMU(mmu);
}

void InsCache_MaybeSetBaseAddress(uint64_t IP) {
    g_insCache.MaybeSetBaseAddress(IP);
}

void StopExecution(void** state) {
    if (state != nullptr) {
        InstructionExecutionRunState* s = new InstructionExecutionRunState();
        s->Terminate = g_TerminateExecution.load() == 1;
        s->Running = g_ExecutionRunning.load() == 1;
        s->Allowed = g_ExecutionAllowed.load() == 1;
        s->AllowOne = g_AllowOneInstruction.load() == 1;
        *state = s;
    }

    g_TerminateExecution.store(1);
    while (g_ExecutionRunning.load() == 1) {
    }
}

void PauseExecution() {
    g_ExecutionAllowed.store(0);
    g_ExecutionRunning.wait(1);
}

void AllowExecution(void** old_state) {
    if (old_state != nullptr) {
        InstructionExecutionRunState* s = static_cast<InstructionExecutionRunState*>(*old_state);
        g_AllowOneInstruction.store(s->AllowOne);
        g_ExecutionAllowed.store(s->Allowed);
        g_TerminateExecution.store(s->Terminate);

        g_AllowOneInstruction.notify_all();
        g_ExecutionAllowed.notify_all();

        delete s;
    }
    else {
        g_TerminateExecution.store(0);
        g_ExecutionAllowed.store(1);
        g_ExecutionAllowed.notify_all();
    }
}

void AllowOneInstruction() {
    g_AllowOneInstruction.store(1);
    g_ExecutionAllowed.store(1);
    g_ExecutionAllowed.notify_all();
    g_AllowOneInstruction.wait(1);
    g_ExecutionRunning.wait(1);
}

void AddBreakpoint(uint64_t address, std::function<void(uint64_t)> callback) {
    spinlock_acquire(&g_breakpointsLock);
    g_breakpoints[address] = std::move(callback);
    spinlock_release(&g_breakpointsLock);
    if (g_breakpointsEnabled.load() == 0)
        g_breakpointsEnabled.store(1);
}

void RemoveBreakpoint(uint64_t address) {
    spinlock_acquire(&g_breakpointsLock);
    g_breakpoints.erase(address);
    spinlock_release(&g_breakpointsLock);
}

bool ExecuteInstruction(uint64_t IP) {
    if (g_TerminateExecution.load() == 1) {
        g_ExecutionRunning.store(0);
        g_ExecutionRunning.notify_all();
        return false; // completely stop execution
    }
    if (g_ExecutionAllowed.load() == 0) {
        if (g_ExecutionRunning.load() == 1) {
            g_ExecutionRunning.store(0);
            g_ExecutionRunning.notify_all();
        }
        g_ExecutionAllowed.wait(0);
        return true; // still looping through instructions, just not doing anything
    }
    else if (g_ExecutionRunning.load() == 0) {
        g_ExecutionRunning.store(1);
        g_ExecutionRunning.notify_all();
    }

    if (g_AllowOneInstruction.load() == 1) {
        g_ExecutionRunning.store(1);
        g_AllowOneInstruction.store(0);
        g_AllowOneInstruction.notify_all();
        g_ExecutionAllowed.store(0);
    }

    if (g_breakpointsEnabled.load() == 1 && g_AllowOneInstruction.load() == 0) { // don't check on single step
        // fprintf(stderr, "Breakpoint check at 0x%lx\n", IP);
        spinlock_acquire(&g_breakpointsLock);
        auto it = g_breakpoints.find(IP);
        if (it != g_breakpoints.end()) {
            g_ExecutionRunning.store(0);
            g_ExecutionRunning.notify_all();
            g_ExecutionAllowed.store(0);
            g_ExecutionAllowed.notify_all();

            g_CurrentBreakpoint.first = IP;
            g_CurrentBreakpoint.second = it->second;
            g_breakpointHit = true;
            g_breakpoints.erase(it);
            spinlock_release(&g_breakpointsLock);

            g_CurrentBreakpoint.second(IP);

            return true;
        }
        spinlock_release(&g_breakpointsLock);
    }

    if (g_breakpointHit && g_CurrentBreakpoint.first != IP) {
        spinlock_acquire(&g_breakpointsLock);
        g_breakpoints[g_CurrentBreakpoint.first] = g_CurrentBreakpoint.second;
        g_breakpointHit = false;
        spinlock_release(&g_breakpointsLock);
    }

    uint64_t current_offset = 0;
    if (!DecodeInstruction(g_insCache, current_offset, &g_current_instruction, [](const char* message, void*) {
#ifdef EMULATOR_DEBUG
        printf("Decoding error: %s\n", message);
#else
        (void)message;
#endif
        g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
    }))
        g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
    uint8_t Opcode = static_cast<uint8_t>(g_current_instruction.GetOpcode());
    for (uint64_t i = 0; i < g_current_instruction.operand_count; i++) {
        switch (InsEncoding::Operand* op = &g_current_instruction.operands[i]; op->type) {
        case InsEncoding::OperandType::REGISTER: {
            InsEncoding::Register* temp_reg = static_cast<InsEncoding::Register*>(op->data);
            Register* reg = Emulator::GetRegisterPointer(static_cast<uint8_t>(*temp_reg));
            g_currentOperands[i] = Operand(static_cast<OperandSize>(op->size), OperandType::Register, reg);
            break;
        }
        case InsEncoding::OperandType::IMMEDIATE: {
            uint64_t data;
            switch (op->size) {
            case InsEncoding::OperandSize::BYTE:
                data = *static_cast<uint8_t*>(op->data);
                break;
            case InsEncoding::OperandSize::WORD:
                data = *static_cast<uint16_t*>(op->data);
                break;
            case InsEncoding::OperandSize::DWORD:
                data = *static_cast<uint32_t*>(op->data);
                break;
            case InsEncoding::OperandSize::QWORD:
                data = *static_cast<uint64_t*>(op->data);
                break;
            default:
                g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
                break;
            }
            g_currentOperands[i] = Operand(static_cast<OperandSize>(op->size), OperandType::Immediate, data);
            break;
        }
        case InsEncoding::OperandType::MEMORY: {
            uint64_t* temp = static_cast<uint64_t*>(op->data);
            g_currentOperands[i] = Operand(static_cast<OperandSize>(op->size), OperandType::Memory, *temp, Emulator::HandleMemoryOperation);
            break;
        }
        case InsEncoding::OperandType::COMPLEX: {
            InsEncoding::ComplexData* temp = static_cast<InsEncoding::ComplexData*>(op->data);
            g_complex[i].base.present = temp->base.present;
            g_complex[i].index.present = temp->index.present;
            g_complex[i].offset.present = temp->offset.present;
            if (g_complex[i].base.present) {
                if (temp->base.type == InsEncoding::ComplexItem::Type::REGISTER) {
                    InsEncoding::Register* temp_reg = temp->base.data.reg;
                    Register* reg = Emulator::GetRegisterPointer(static_cast<uint8_t>(*temp_reg));
                    g_complex[i].base.data.reg = reg;
                    g_complex[i].base.type = ComplexItem::Type::REGISTER;
                } else {
                    g_complex[i].base.data.imm.size = static_cast<OperandSize>(temp->base.data.imm.size);
                    g_complex[i].base.data.imm.data = temp->base.data.imm.data;
                    g_complex[i].base.type = ComplexItem::Type::IMMEDIATE;
                }
            } else
                g_complex[i].base.present = false;
            if (g_complex[i].index.present) {
                if (temp->index.type == InsEncoding::ComplexItem::Type::REGISTER) {
                    InsEncoding::Register* temp_reg = temp->index.data.reg;
                    Register* reg = Emulator::GetRegisterPointer(static_cast<uint8_t>(*temp_reg));
                    g_complex[i].index.data.reg = reg;
                    g_complex[i].index.type = ComplexItem::Type::REGISTER;
                } else {
                    g_complex[i].index.data.imm.size = static_cast<OperandSize>(temp->index.data.imm.size);
                    g_complex[i].index.data.imm.data = temp->index.data.imm.data;
                    g_complex[i].index.type = ComplexItem::Type::IMMEDIATE;
                }
            } else
                g_complex[i].index.present = false;
            if (g_complex[i].offset.present) {
                if (temp->offset.type == InsEncoding::ComplexItem::Type::REGISTER) {
                    InsEncoding::Register* temp_reg = temp->offset.data.reg;
                    Register* reg = Emulator::GetRegisterPointer(static_cast<uint8_t>(*temp_reg));
                    g_complex[i].offset.data.reg = reg;
                    g_complex[i].offset.type = ComplexItem::Type::REGISTER;
                    g_complex[i].offset.sign = temp->offset.sign;
                } else {
                    g_complex[i].offset.data.imm.size = static_cast<OperandSize>(temp->offset.data.imm.size);
                    g_complex[i].offset.data.imm.data = temp->offset.data.imm.data;
                    g_complex[i].offset.type = ComplexItem::Type::IMMEDIATE;
                }
            } else
                g_complex[i].offset.present = false;
            g_currentOperands[i] = Operand(static_cast<OperandSize>(op->size), OperandType::Complex, &g_complex[i], Emulator::HandleMemoryOperation);
            break;
        }
        default:
            g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            break;
        }
    }

    // Get the instruction
    uint8_t argument_count = 0;
    void* ins = DecodeOpcode(Opcode, &argument_count);
    if (ins == nullptr)
        g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);

    // Increment instruction pointer
    Emulator::SetNextIP(IP + current_offset);

    // Execute the instruction
    if (argument_count == 0)
        reinterpret_cast<void (*)()>(ins)();
    else if (argument_count == 1)
        reinterpret_cast<void (*)(Operand*)>(ins)(&g_currentOperands[0]);
    else if (argument_count == 2)
        reinterpret_cast<void (*)(Operand*, Operand*)>(ins)(&g_currentOperands[0], &g_currentOperands[1]);

    Emulator::SyncRegisters();

    Emulator::SetCPU_IP(Emulator::GetNextIP());
    return true;
}

void ExecutionLoop() {
    bool status = true;
    while (status)
        status = ExecuteInstruction(Emulator::GetCPU_IP());
}

void* DecodeOpcode(uint8_t opcode, uint8_t* argument_count) {
    uint8_t offset = opcode & 0x0f;
    switch ((opcode >> 4) & 0x07) {
    case 0: // ALU
        switch (offset) {
        case 0: // add
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_add);
        case 1: // mul
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_mul);
        case 2: // sub
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_sub);
        case 3: // div
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_div);
        case 4: // or
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_or);
        case 5: // xor
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_xor);
        case 6: // nor
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_nor);
        case 7: // and
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_and);
        case 8: // nand
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_nand);
        case 9: // not
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_not);
        case 0xa: // cmp
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_cmp);
        case 0xb: // inc
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_inc);
        case 0xc: // dec
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_dec);
        case 0xd: // shl
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_shl);
        case 0xe: // shr
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_shr);
        default:
            return nullptr;
        }
    case 1: // control flow
        switch (offset) {
        case 0: // ret
            if (argument_count != nullptr)
                *argument_count = 0;
            return reinterpret_cast<void*>(ins_ret);
        case 1: // call
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_call);
        case 2: // jmp
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_jmp);
        case 3: // jc
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_jc);
        case 4: // jnc
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_jnc);
        case 5: // jz
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_jz);
        case 6: // jnz
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_jnz);
        case 7: // jl
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_jl);
        case 8: // jle
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_jle);
        case 9: // jnl
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_jnl);
        case 0xa: // jnle
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_jnle);
        default:
            return nullptr;
        }
    case 2: // other
        switch (offset) {
        case 0: // mov
            if (argument_count != nullptr)
                *argument_count = 2;
            return reinterpret_cast<void*>(ins_mov);
        case 1: // nop
            if (argument_count != nullptr)
                *argument_count = 0;
            return reinterpret_cast<void*>(ins_nop);
        case 2: // hlt
            if (argument_count != nullptr)
                *argument_count = 0;
            return reinterpret_cast<void*>(ins_hlt);
        case 3: // push
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_push);
        case 4: // pop
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_pop);
        case 5: // pusha
            if (argument_count != nullptr)
                *argument_count = 0;
            return reinterpret_cast<void*>(ins_pusha);
        case 6: // popa
            if (argument_count != nullptr)
                *argument_count = 0;
            return reinterpret_cast<void*>(ins_popa);
        case 7: // int
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_int);
        case 8: // lidt
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_lidt);
        case 9: // iret
            if (argument_count != nullptr)
                *argument_count = 0;
            return reinterpret_cast<void*>(ins_iret);
        case 0xa: // syscall
            if (argument_count != nullptr)
                *argument_count = 0;
            return reinterpret_cast<void*>(ins_syscall);
        case 0xb: // sysret
            if (argument_count != nullptr)
                *argument_count = 0;
            return reinterpret_cast<void*>(ins_sysret);
        case 0xc: // enteruser
            if (argument_count != nullptr)
                *argument_count = 1;
            return reinterpret_cast<void*>(ins_enteruser);
        default:
            return nullptr;
        }
    default: // reserved
        return nullptr;
    }
    return nullptr;
}

#ifdef EMULATOR_DEBUG
#define PRINT_INS_INFO2(dst, src)                              \
    printf("%s: dst = \"", __extension__ __PRETTY_FUNCTION__); \
    dst->PrintInfo();                                          \
    printf("\", src = \"");                                    \
    src->PrintInfo();                                          \
    printf("\"\n")
#define PRINT_INS_INFO1(dst)                                   \
    printf("%s: dst = \"", __extension__ __PRETTY_FUNCTION__); \
    dst->PrintInfo();                                          \
    printf("\"\n")
#define PRINT_INS_INFO0() printf("%s\n", __extension__ __PRETTY_FUNCTION__)
#else
#define PRINT_INS_INFO2(dst, src)
#define PRINT_INS_INFO1(dst)
#define PRINT_INS_INFO0()
#endif

#ifdef __x86_64__

#include <Platform/x86_64/ALUInstruction.h>

#define ALU_INSTRUCTION2(name)                                                  \
    void ins_##name(Operand* dst, Operand* src) {                               \
        PRINT_INS_INFO2(dst, src);                                              \
        uint64_t flags = 0;                                                     \
        dst->SetValue(x86_64_##name(dst->GetValue(), src->GetValue(), &flags)); \
        Emulator::ClearCPUStatus(0xF);                                          \
        Emulator::SetCPUStatus(flags & 0xF);                                    \
    }

#define ALU_INSTRUCTION2_NO_RET_VAL(name)                        \
    void ins_##name(Operand* dst, Operand* src) {                \
        PRINT_INS_INFO2(dst, src);                               \
        uint64_t flags = 0;                                      \
        x86_64_##name(dst->GetValue(), src->GetValue(), &flags); \
        Emulator::ClearCPUStatus(0xF);                           \
        Emulator::SetCPUStatus(flags & 0xF);                     \
    }

#define ALU_INSTRUCTION1(name)                                 \
    void ins_##name(Operand* dst) {                            \
        PRINT_INS_INFO1(dst);                                  \
        uint64_t flags = 0;                                    \
        dst->SetValue(x86_64_##name(dst->GetValue(), &flags)); \
        Emulator::ClearCPUStatus(0xF);                         \
        Emulator::SetCPUStatus(flags & 0xF);                   \
    }

ALU_INSTRUCTION2(add)
ALU_INSTRUCTION2(mul)
ALU_INSTRUCTION2(sub)

void ins_div(Operand* src1, Operand* src2) {
    PRINT_INS_INFO2(src1, src2);
    uint64_t src_val = src2->GetValue();
    if (src_val == 0)
        g_ExceptionHandler->RaiseException(Exception::DIV_BY_ZERO);
    uint64_t flags = 0;
    x86_64_DivResult result = x86_64_div(src1->GetValue(), src_val, &flags);
    Emulator::GetRegisterPointer(RegisterID_R0)->SetValue(result.quotient);
    Emulator::GetRegisterPointer(RegisterID_R1)->SetValue(result.remainder);
    Emulator::ClearCPUStatus(0xF);
    Emulator::SetCPUStatus(flags & 0xF);
}

ALU_INSTRUCTION2(or)
ALU_INSTRUCTION2(xor)
ALU_INSTRUCTION2(nor)
ALU_INSTRUCTION2(and)
ALU_INSTRUCTION2(nand)
ALU_INSTRUCTION1(not)
ALU_INSTRUCTION2(shl)
ALU_INSTRUCTION2(shr)
ALU_INSTRUCTION2_NO_RET_VAL(cmp)
ALU_INSTRUCTION1(inc)
ALU_INSTRUCTION1(dec)

#else /* __x86_64__ */

void ins_add(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(dst->GetValue() + src->GetValue());
}

void ins_mul(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(dst->GetValue() * src->GetValue());
}

void ins_sub(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(dst->GetValue() - src->GetValue());
}

void ins_div(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(dst->GetValue() / src->GetValue());
}

void ins_or(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(dst->GetValue() | src->GetValue());
}

void ins_xor(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(dst->GetValue() ^ src->GetValue());
}

void ins_nor(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(~(dst->GetValue() | src->GetValue()));
}

void ins_and(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(dst->GetValue() & src->GetValue());
}

void ins_nand(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(~(dst->GetValue() & src->GetValue()));
}

void ins_not(Operand* dst) {
    PRINT_INS_INFO1(dst);
    dst->SetValue(~dst->GetValue());
}

void ins_cmp(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
}

void ins_inc(Operand* dst) {
    PRINT_INS_INFO1(dst);
    dst->SetValue(dst->GetValue() + 1);
}

void ins_dec(Operand* dst) {
    PRINT_INS_INFO1(dst);
    dst->SetValue(dst->GetValue() - 1);
}

void ins_shl(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(dst->GetValue() << src->GetValue());
}

void ins_shr(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(dst->GetValue() >> src->GetValue());
}

#endif /* __x86_64__ */

void ins_ret() {
    PRINT_INS_INFO0();
    uint64_t IP = g_stack->pop();
    Emulator::SetNextIP(IP);
    g_insCache.MaybeSetBaseAddress(IP);
}

void ins_call(Operand* dst) {
    PRINT_INS_INFO1(dst);
    g_stack->push(Emulator::GetNextIP());
    uint64_t IP = dst->GetValue();
    Emulator::SetNextIP(IP);
    g_insCache.MaybeSetBaseAddress(IP);
}

void ins_jmp(Operand* dst) {
    PRINT_INS_INFO1(dst);
    uint64_t IP = dst->GetValue();
    Emulator::SetNextIP(IP);
    g_insCache.MaybeSetBaseAddress(IP);
}

void ins_jc(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); flags & 1) {
        uint64_t IP = dst->GetValue();
        Emulator::SetNextIP(IP);
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jnc(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); !(flags & 1)) {
        uint64_t IP = dst->GetValue();
        Emulator::SetNextIP(IP);
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jz(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); flags & 2) {
        uint64_t IP = dst->GetValue();
        Emulator::SetNextIP(IP);
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jnz(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); !(flags & 2)) {
        uint64_t IP = dst->GetValue();
        Emulator::SetNextIP(IP);
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jl(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); (flags & 4) != (flags & 8)) {
        uint64_t IP = dst->GetValue();
        Emulator::SetNextIP(IP);
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jle(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); (flags & 4) != (flags & 8) || (flags & 2)) {
        uint64_t IP = dst->GetValue();
        Emulator::SetNextIP(IP);
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jnl(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); (flags & 4) == (flags & 8)) {
        uint64_t IP = dst->GetValue();
        Emulator::SetNextIP(IP);
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jnle(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); (flags & 4) == (flags & 8) && !(flags & 2)) {
        uint64_t IP = dst->GetValue();
        Emulator::SetNextIP(IP);
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_mov(Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(src->GetValue());
}

void ins_nop() {
    PRINT_INS_INFO0();
}

void ins_hlt() {
    PRINT_INS_INFO0();
    Emulator::HandleHalt();
}

void ins_push(Operand* src) {
    PRINT_INS_INFO1(src);
    g_stack->push(src->GetValue());
}

void ins_pop(Operand* dst) {
    PRINT_INS_INFO1(dst);
    dst->SetValue(g_stack->pop());
}

void ins_pusha() {
    PRINT_INS_INFO0();
    Register* r0 = Emulator::GetRegisterPointer(RegisterID_R0);
    Register* r1 = Emulator::GetRegisterPointer(RegisterID_R1);
    Register* r2 = Emulator::GetRegisterPointer(RegisterID_R2);
    Register* r3 = Emulator::GetRegisterPointer(RegisterID_R3);
    Register* r4 = Emulator::GetRegisterPointer(RegisterID_R4);
    Register* r5 = Emulator::GetRegisterPointer(RegisterID_R5);
    Register* r6 = Emulator::GetRegisterPointer(RegisterID_R6);
    Register* r7 = Emulator::GetRegisterPointer(RegisterID_R7);
    Register* r8 = Emulator::GetRegisterPointer(RegisterID_R8);
    Register* r9 = Emulator::GetRegisterPointer(RegisterID_R9);
    Register* r10 = Emulator::GetRegisterPointer(RegisterID_R10);
    Register* r11 = Emulator::GetRegisterPointer(RegisterID_R11);
    Register* r12 = Emulator::GetRegisterPointer(RegisterID_R12);
    Register* r13 = Emulator::GetRegisterPointer(RegisterID_R13);
    Register* r14 = Emulator::GetRegisterPointer(RegisterID_R14);
    Register* r15 = Emulator::GetRegisterPointer(RegisterID_R15);
    g_stack->push(r0->GetValue());
    g_stack->push(r1->GetValue());
    g_stack->push(r2->GetValue());
    g_stack->push(r3->GetValue());
    g_stack->push(r4->GetValue());
    g_stack->push(r5->GetValue());
    g_stack->push(r6->GetValue());
    g_stack->push(r7->GetValue());
    g_stack->push(r8->GetValue());
    g_stack->push(r9->GetValue());
    g_stack->push(r10->GetValue());
    g_stack->push(r11->GetValue());
    g_stack->push(r12->GetValue());
    g_stack->push(r13->GetValue());
    g_stack->push(r14->GetValue());
    g_stack->push(r15->GetValue());
}

void ins_popa() {
    PRINT_INS_INFO0();
    Register* r0 = Emulator::GetRegisterPointer(RegisterID_R0);
    Register* r1 = Emulator::GetRegisterPointer(RegisterID_R1);
    Register* r2 = Emulator::GetRegisterPointer(RegisterID_R2);
    Register* r3 = Emulator::GetRegisterPointer(RegisterID_R3);
    Register* r4 = Emulator::GetRegisterPointer(RegisterID_R4);
    Register* r5 = Emulator::GetRegisterPointer(RegisterID_R5);
    Register* r6 = Emulator::GetRegisterPointer(RegisterID_R6);
    Register* r7 = Emulator::GetRegisterPointer(RegisterID_R7);
    Register* r8 = Emulator::GetRegisterPointer(RegisterID_R8);
    Register* r9 = Emulator::GetRegisterPointer(RegisterID_R9);
    Register* r10 = Emulator::GetRegisterPointer(RegisterID_R10);
    Register* r11 = Emulator::GetRegisterPointer(RegisterID_R11);
    Register* r12 = Emulator::GetRegisterPointer(RegisterID_R12);
    Register* r13 = Emulator::GetRegisterPointer(RegisterID_R13);
    Register* r14 = Emulator::GetRegisterPointer(RegisterID_R14);
    Register* r15 = Emulator::GetRegisterPointer(RegisterID_R15);
    r15->SetValue(g_stack->pop());
    r14->SetValue(g_stack->pop());
    r13->SetValue(g_stack->pop());
    r12->SetValue(g_stack->pop());
    r11->SetValue(g_stack->pop());
    r10->SetValue(g_stack->pop());
    r9->SetValue(g_stack->pop());
    r8->SetValue(g_stack->pop());
    r7->SetValue(g_stack->pop());
    r6->SetValue(g_stack->pop());
    r5->SetValue(g_stack->pop());
    r4->SetValue(g_stack->pop());
    r3->SetValue(g_stack->pop());
    r2->SetValue(g_stack->pop());
    r1->SetValue(g_stack->pop());
    r0->SetValue(g_stack->pop());
}

void ins_int(Operand* number) {
    PRINT_INS_INFO1(number);
    if (Emulator::isInProtectedMode() && Emulator::isInUserMode())
        g_ExceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    uint64_t interrupt = number->GetValue();
    g_InterruptHandler->RaiseInterrupt(interrupt, Emulator::GetNextIP());
}

void ins_lidt(Operand* src) {
    PRINT_INS_INFO1(src);
    if (Emulator::isInProtectedMode() && Emulator::isInUserMode())
        g_ExceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    g_InterruptHandler->SetIDTR(src->GetValue());
}

void ins_iret() {
    PRINT_INS_INFO0();
    if (Emulator::isInProtectedMode() && Emulator::isInUserMode())
        g_ExceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    g_InterruptHandler->ReturnFromInterrupt();
}

void ins_syscall() {
    PRINT_INS_INFO0();
    if (Emulator::isInProtectedMode() && !Emulator::isInUserMode())
        g_ExceptionHandler->RaiseException(Exception::SUPERVISOR_MODE_VIOLATION);
    Emulator::ExitUserMode();
}

void ins_sysret() {
    PRINT_INS_INFO0();
    if (Emulator::isInProtectedMode() && Emulator::isInUserMode())
        g_ExceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    Emulator::EnterUserMode();
}

void ins_enteruser(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (Emulator::isInProtectedMode() && Emulator::isInUserMode())
        g_ExceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    Emulator::EnterUserMode(dst->GetValue());
}