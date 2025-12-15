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

#include <atomic>
#include <cstring>
#include <utility>

#include "InstructionCache.hpp"

#ifdef EMULATOR_DEBUG
#include <cstdio>
#endif

#include <csignal>

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

uint64_t* g_rawIPPointer = nullptr;
uint64_t* g_rawNextIPPointer = nullptr;

struct InsOpcodeArgCountPair {
    void* function;
    uint8_t argCount;
};

struct InstructionData {
    InsEncoding::SimpleInstruction instruction;
    Operand operands[3];
    ComplexData complex[3];
    uint64_t IP;
    bool used;
    InsOpcodeArgCountPair pair;
    uint64_t size;
};

InstructionData g_InstructionDataCache[128]; // 128 instructions is quite big, should hopefully be enough

InstructionData* g_currentInstruction;
int g_currentCacheOffset;
bool g_cacheJustMissed = true;

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

InsOpcodeArgCountPair g_InstructionFunctions[256];

void InitInstructionSubsystem(uint64_t startingIP, MMU* mmu) {
    memset(g_InstructionFunctions, 0, sizeof(g_InstructionFunctions));
#define SETINSFUNC(op, Func, args) g_InstructionFunctions[static_cast<int>(InsEncoding::Opcode::op)] = {reinterpret_cast<void*>(Func), args}
    SETINSFUNC(ADD, ins_add, 2);
    SETINSFUNC(SUB, ins_sub, 2);
    SETINSFUNC(MUL, ins_mul, 3);
    SETINSFUNC(DIV, ins_div, 3);
    SETINSFUNC(SMUL, ins_smul, 3);
    SETINSFUNC(SDIV, ins_sdiv, 3);
    SETINSFUNC(OR, ins_or, 2);
    SETINSFUNC(NOR, ins_nor, 2);
    SETINSFUNC(XOR, ins_xor, 2);
    SETINSFUNC(XNOR, ins_xnor, 2);
    SETINSFUNC(AND, ins_and, 2);
    SETINSFUNC(NAND, ins_nand, 2);
    SETINSFUNC(NOT, ins_not, 1);
    SETINSFUNC(SHL, ins_shl, 2);
    SETINSFUNC(SHR, ins_shr, 2);
    SETINSFUNC(CMP, ins_cmp, 2);
    SETINSFUNC(INC, ins_inc, 1);
    SETINSFUNC(DEC, ins_dec, 1);
    SETINSFUNC(RET, ins_ret, 0);
    SETINSFUNC(CALL, ins_call, 1);
    SETINSFUNC(JMP, ins_jmp, 1);
    SETINSFUNC(JC, ins_jc, 1);
    SETINSFUNC(JNC, ins_jnc, 1);
    SETINSFUNC(JZ, ins_jz, 1);
    SETINSFUNC(JNZ, ins_jnz, 1);
    SETINSFUNC(JL, ins_jl, 1);
    SETINSFUNC(JLE, ins_jle, 1);
    SETINSFUNC(JNL, ins_jnl, 1);
    SETINSFUNC(JNLE, ins_jnle, 1);
    SETINSFUNC(MOV, ins_mov, 2);
    SETINSFUNC(NOP, ins_nop, 0);
    SETINSFUNC(HLT, ins_hlt, 0);
    SETINSFUNC(PUSH, ins_push, 1);
    SETINSFUNC(POP, ins_pop, 1);
    SETINSFUNC(PUSHA, ins_pusha, 0);
    SETINSFUNC(POPA, ins_popa, 0);
    SETINSFUNC(INT, ins_int, 1);
    SETINSFUNC(LIDT, ins_lidt, 1);
    SETINSFUNC(IRET, ins_iret, 0);
    SETINSFUNC(SYSCALL, ins_syscall, 0);
    SETINSFUNC(SYSRET, ins_sysret, 0);
    SETINSFUNC(ENTERUSER, ins_enteruser, 1);
#undef SETINSFUNC

    g_rawIPPointer = Emulator::GetRawIPPointer();
    g_rawNextIPPointer = Emulator::GetRawNextIPPointer();

    InitInsCache(startingIP, mmu);
}

void InitInsCache(uint64_t startingIP, MMU *mmu) {
    g_insCache.Init(mmu, startingIP);
    g_currentCacheOffset = 0;
    g_currentInstruction = &g_InstructionDataCache[0];
    g_cacheJustMissed = true;
    for (int i = 0; i < 128; i++) {
        g_InstructionDataCache[i].used = false;
        g_InstructionDataCache[i].IP = 0;
        g_InstructionDataCache[i].pair.function = nullptr;
        g_InstructionDataCache[i].pair.argCount = 0;
    }
}

void UpdateInsCacheMMU(MMU *mmu) {
    g_insCache.UpdateMMU(mmu);
}

void InsCache_MaybeSetBaseAddress(uint64_t IP) {
    g_insCache.MaybeSetBaseAddress(IP);
}

int FindIPInInsCache(uint64_t IP) {
    if (g_currentInstruction->used && g_currentInstruction->IP == IP)
        return g_currentCacheOffset;
    for (int i = g_currentCacheOffset - 1; i > 0; i--) {
        if (g_InstructionDataCache[i].used && g_InstructionDataCache[i].IP == IP)
            return i;
    }
    for (int i = 127; i > g_currentCacheOffset; i--) {
        if (g_InstructionDataCache[i].used && g_InstructionDataCache[i].IP == IP)
            return i;
    }
    return -1;
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

void AllowExecution(void** oldState) {
    if (oldState != nullptr) {
        InstructionExecutionRunState* s = static_cast<InstructionExecutionRunState*>(*oldState);
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

void ExecutionLoop() {
    while (true) {
        uint64_t IP = *g_rawIPPointer;
        if (g_TerminateExecution.load() == 1) {
            g_ExecutionRunning.store(0);
            g_ExecutionRunning.notify_all();
            break; // completely stop execution
        }
        if (g_ExecutionAllowed.load() == 0) {
            if (g_ExecutionRunning.load() == 1) {
                g_ExecutionRunning.store(0);
                g_ExecutionRunning.notify_all();
            }
            g_ExecutionAllowed.wait(0);
            continue; // still looping through instructions, just not doing anything
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

                continue;
            }
            spinlock_release(&g_breakpointsLock);
        }

        if (g_breakpointHit && g_CurrentBreakpoint.first != IP) {
            spinlock_acquire(&g_breakpointsLock);
            g_breakpoints[g_CurrentBreakpoint.first] = g_CurrentBreakpoint.second;
            g_breakpointHit = false;
            spinlock_release(&g_breakpointsLock);
        }


        if (int offset = FindIPInInsCache(IP); offset != -1) {
            g_currentInstruction = &g_InstructionDataCache[offset];
            g_currentCacheOffset = offset;
            g_cacheJustMissed = false;
        }
        else {
            if (g_currentInstruction->used) {
                // need to find a different slot
                int i_offset = -1;
                for (int i = g_currentCacheOffset + 1; i < 128; i++) { // start with going from the current offset to the end
                    if (!g_InstructionDataCache[i].used) {
                        i_offset = i;
                        break;
                    }
                }
                if (i_offset == -1) {
                    // start from 0, until current offset
                    for (int i = 0; i < g_currentCacheOffset; i++) {
                        if (!g_InstructionDataCache[i].used) {
                            i_offset = i;
                            break;
                        }
                    }
                    if (i_offset == -1) {
                        // all used, start from current + 1
                        if (g_currentCacheOffset == 127)
                            i_offset = 0;
                        else
                            i_offset = g_currentCacheOffset + 1;
                    }
                }
                g_currentCacheOffset = i_offset;
                g_currentInstruction = &g_InstructionDataCache[i_offset];
            }
            g_currentInstruction->used = false;
            g_currentInstruction->IP = 0;
            g_currentInstruction->pair.function = nullptr;
            g_currentInstruction->pair.argCount = 0;
            uint64_t currentOffset = 0;
            if (!g_cacheJustMissed) {
                g_insCache.MaybeSetBaseAddress(IP);
                g_cacheJustMissed = true;
            }
            if (!DecodeInstruction(g_insCache, currentOffset, &g_currentInstruction->instruction, g_currentCacheOffset, [](const char* message, void*) {
        #ifdef EMULATOR_DEBUG
                printf("Decoding error: %s\n", message);
        #else
                (void)message;
        #endif
                g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            }))
                g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            InsEncoding::SimpleInstruction& currentIns = g_currentInstruction->instruction;
            ComplexData* complex = g_currentInstruction->complex;
            uint8_t Opcode = static_cast<uint8_t>(currentIns.GetOpcode());
            for (uint64_t i = 0; i < currentIns.operandCount; i++) {
                switch (InsEncoding::Operand* op = &currentIns.operands[i]; op->type) {
                case InsEncoding::OperandType::REGISTER: {
                    InsEncoding::Register* tempReg = static_cast<InsEncoding::Register*>(op->data);
                    Register* reg = Emulator::GetRegisterPointer(static_cast<uint8_t>(*tempReg));
                    g_currentInstruction->operands[i] = Operand(static_cast<OperandSize>(op->size), reg);
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
                    g_currentInstruction->operands[i] = Operand(static_cast<OperandSize>(op->size), data);
                    break;
                }
                case InsEncoding::OperandType::MEMORY: {
                    uint64_t* temp = static_cast<uint64_t*>(op->data);
                    g_currentInstruction->operands[i] = Operand(static_cast<OperandSize>(op->size), *temp, Emulator::HandleMemoryOperation);
                    break;
                }
                case InsEncoding::OperandType::COMPLEX: {
                    InsEncoding::ComplexData* temp = static_cast<InsEncoding::ComplexData*>(op->data);
                    complex[i].base.present = temp->base.present;
                    complex[i].index.present = temp->index.present;
                    complex[i].offset.present = temp->offset.present;
                    if (complex[i].base.present) {
                        if (temp->base.type == InsEncoding::ComplexItem::Type::REGISTER) {
                            InsEncoding::Register* tempReg = temp->base.data.reg;
                            Register* reg = Emulator::GetRegisterPointer(static_cast<uint8_t>(*tempReg));
                            complex[i].base.data.reg = reg;
                            complex[i].base.type = ComplexItem::Type::REGISTER;
                        } else {
                            complex[i].base.data.imm.size = static_cast<OperandSize>(temp->base.data.imm.size);
                            complex[i].base.data.imm.data = temp->base.data.imm.data;
                            complex[i].base.type = ComplexItem::Type::IMMEDIATE;
                        }
                    } else
                        complex[i].base.present = false;
                    if (complex[i].index.present) {
                        if (temp->index.type == InsEncoding::ComplexItem::Type::REGISTER) {
                            InsEncoding::Register* tempReg = temp->index.data.reg;
                            Register* reg = Emulator::GetRegisterPointer(static_cast<uint8_t>(*tempReg));
                            complex[i].index.data.reg = reg;
                            complex[i].index.type = ComplexItem::Type::REGISTER;
                        } else {
                            complex[i].index.data.imm.size = static_cast<OperandSize>(temp->index.data.imm.size);
                            complex[i].index.data.imm.data = temp->index.data.imm.data;
                            complex[i].index.type = ComplexItem::Type::IMMEDIATE;
                        }
                    } else
                        complex[i].index.present = false;
                    if (complex[i].offset.present) {
                        if (temp->offset.type == InsEncoding::ComplexItem::Type::REGISTER) {
                            InsEncoding::Register* tempReg = temp->offset.data.reg;
                            Register* reg = Emulator::GetRegisterPointer(static_cast<uint8_t>(*tempReg));
                            complex[i].offset.data.reg = reg;
                            complex[i].offset.type = ComplexItem::Type::REGISTER;
                            complex[i].offset.sign = temp->offset.sign;
                        } else {
                            complex[i].offset.data.imm.size = static_cast<OperandSize>(temp->offset.data.imm.size);
                            complex[i].offset.data.imm.data = temp->offset.data.imm.data;
                            complex[i].offset.type = ComplexItem::Type::IMMEDIATE;
                        }
                    } else
                        complex[i].offset.present = false;
                    g_currentInstruction->operands[i] = Operand(static_cast<OperandSize>(op->size), &complex[i], Emulator::HandleMemoryOperation);
                    break;
                }
                default:
                    g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
                    break;
                }
            }
            g_currentInstruction->IP = IP;

            // Get the instruction
            g_currentInstruction->pair = g_InstructionFunctions[Opcode];
            if (g_currentInstruction->pair.function == nullptr)
                g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            g_currentInstruction->size = currentOffset;
            g_currentInstruction->used = true;
        }

        // Increment instruction pointer
        *g_rawNextIPPointer = IP + g_currentInstruction->size;

        InsOpcodeArgCountPair pair = g_currentInstruction->pair;
        Operand* operands = g_currentInstruction->operands;

        // Update the cache
        g_currentCacheOffset++;
        if (g_currentCacheOffset >= 128) // wrap around
            g_currentCacheOffset = 0;
        g_currentInstruction = &g_InstructionDataCache[g_currentCacheOffset];

        // Execute the instruction
        if (pair.argCount == 0)
            reinterpret_cast<void (*)()>(pair.function)();
        else if (pair.argCount == 1)
            reinterpret_cast<void (*)(Operand*)>(pair.function)(&operands[0]);
        else if (pair.argCount == 2)
            reinterpret_cast<void (*)(Operand*, Operand*)>(pair.function)(&operands[0], &operands[1]);
        else if (pair.argCount == 3)
            reinterpret_cast<void (*)(Operand*, Operand*, Operand*)>(pair.function)(&operands[0], &operands[1], &operands[2]);
        else
            g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);

        Emulator::SyncRegisters();

        // Set the IP to the next instruction
        *g_rawIPPointer = *g_rawNextIPPointer;
    }
}

#ifdef EMULATOR_DEBUG
#define PRINT_INS_INFO3(dst2, dst1, src)                         \
    printf("%s: dst2 = \"", __extension__ __PRETTY_FUNCTION__);  \
    dst2->PrintInfo();                                           \
    printf("\", dst1 = \"");                                     \
    dst1->PrintInfo();                                           \
    printf("\", src = \"");                                      \
    src->PrintInfo();                                            \
    printf("\"\n")
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
#define PRINT_INS_INFO3(dst2, dst1, src)
#define PRINT_INS_INFO2(dst, src)
#define PRINT_INS_INFO1(dst)
#define PRINT_INS_INFO0()
#endif

#ifdef __x86_64__

#include <Platform/x86_64/ALUInstruction.h>




#define ALU_INSTRUCTION3(name)                                                            \
    void ins_##name(Operand* dst2, Operand* dst1, Operand* src) {                         \
        PRINT_INS_INFO3(dst2, dst1, src);                                                 \
        uint64_t flags = 0;                                                               \
        x86_64_128Data result = x86_64_##name(dst1->GetValue(), src->GetValue(), &flags); \
        dst1->SetValue(result.low);                                                       \
        dst2->SetValue(result.high);                                                      \
        Emulator::ClearCPUStatus(0xF);                                                    \
        Emulator::SetCPUStatus(flags & 0xF);                                              \
    }

#define DIV_INSTRUCTION3(name)                                           \
    void ins_##name(Operand* dst2, Operand* dst1, Operand* src) {       \
        PRINT_INS_INFO3(dst2, dst1, src);                               \
        uint64_t srcVal = src->GetValue();                              \
        if (srcVal == 0)                                                 \
            g_ExceptionHandler->RaiseException(Exception::DIV_BY_ZERO);  \
        uint64_t flags = 0;                                              \
        x86_64_128Data dividend = {dst1->GetValue(), dst2->GetValue()};  \
        x86_64_128Data result = x86_64_##name(dividend, srcVal, &flags); \
        dst1->SetValue(result.low);                                      \
        dst2->SetValue(result.high);                                     \
        Emulator::ClearCPUStatus(0xF);                                   \
        Emulator::SetCPUStatus(flags & 0xF);                             \
    }

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
ALU_INSTRUCTION2(sub)
ALU_INSTRUCTION3(mul)
DIV_INSTRUCTION3(div)
ALU_INSTRUCTION3(smul)
DIV_INSTRUCTION3(sdiv)
ALU_INSTRUCTION2(or)
ALU_INSTRUCTION2(nor)
ALU_INSTRUCTION2(xor)
ALU_INSTRUCTION2(xnor)
ALU_INSTRUCTION2(and)
ALU_INSTRUCTION2(nand)
ALU_INSTRUCTION1(not)
ALU_INSTRUCTION2(shl)
ALU_INSTRUCTION2(shr)
ALU_INSTRUCTION2_NO_RET_VAL(cmp)
ALU_INSTRUCTION1(inc)
ALU_INSTRUCTION1(dec)

#else /* __x86_64__ */
#error "ALU Instructions: Unsupported architecture"
#endif /* __x86_64__ */

void ins_ret() {
    PRINT_INS_INFO0();
    uint64_t IP = g_stack->pop();
    *g_rawNextIPPointer = IP;
    g_insCache.MaybeSetBaseAddress(IP);
}

void ins_call(Operand* dst) {
    PRINT_INS_INFO1(dst);
    g_stack->push(Emulator::GetNextIP());
    uint64_t IP = dst->GetValue();
    *g_rawNextIPPointer = IP;
    g_insCache.MaybeSetBaseAddress(IP);
}

void ins_jmp(Operand* dst) {
    PRINT_INS_INFO1(dst);
    uint64_t IP = dst->GetValue();
    *g_rawNextIPPointer = IP;
    g_insCache.MaybeSetBaseAddress(IP);
}

void ins_jc(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); flags & 1) {
        uint64_t IP = dst->GetValue();
        *g_rawNextIPPointer = IP;
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jnc(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); !(flags & 1)) {
        uint64_t IP = dst->GetValue();
        *g_rawNextIPPointer = IP;
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jz(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); flags & 2) {
        uint64_t IP = dst->GetValue();
        *g_rawNextIPPointer = IP;
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jnz(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); !(flags & 2)) {
        uint64_t IP = dst->GetValue();
        *g_rawNextIPPointer = IP;
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jl(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); (flags & 4) != (flags & 8)) {
        uint64_t IP = dst->GetValue();
        *g_rawNextIPPointer = IP;
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jle(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); (flags & 4) != (flags & 8) || (flags & 2)) {
        uint64_t IP = dst->GetValue();
        *g_rawNextIPPointer = IP;
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jnl(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); (flags & 4) == (flags & 8)) {
        uint64_t IP = dst->GetValue();
        *g_rawNextIPPointer = IP;
        g_insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jnle(Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = Emulator::GetCPUStatus(); (flags & 4) == (flags & 8) && !(flags & 2)) {
        uint64_t IP = dst->GetValue();
        *g_rawNextIPPointer = IP;
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