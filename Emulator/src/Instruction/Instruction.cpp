/*
Copyright (©) 2024-2026  Frosty515

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

struct InstructionExecutionRunState {
    bool Allowed;
    bool Running;
    bool Terminate;
    bool AllowOne;
};

struct CPUInsState {
    explicit CPUInsState(Emulator::CPUState* cpu) : rawIPPointer(nullptr), rawNextIPPointer(nullptr), instructionDataCache(), currentInstruction(nullptr),
                                                    currentCacheOffset(0), cacheJustMissed(true), insCache(cpu) {}

    uint64_t* rawIPPointer;
    uint64_t* rawNextIPPointer;

    InstructionData instructionDataCache[128]; // 128 instructions is quite big, should hopefully be enough

    InstructionData* currentInstruction;
    int currentCacheOffset;
    bool cacheJustMissed;

    std::unordered_map<uint64_t, std::function<void(uint64_t)>> breakpoints;
    spinlock_new(breakpointsLock);
    std::atomic_uchar breakpointsEnabled = 0;
    std::pair<uint64_t, std::function<void(uint64_t)>> currentBreakpoint;
    bool breakpointHit = false;

    InstructionCache insCache;

    std::atomic_uchar executionAllowed = 0;
    std::atomic_uchar executionRunning = 0;
    std::atomic_uchar terminateExecution = 0;
    std::atomic_uchar allowOneInstruction = 0;

    spinlock_t lock = SPINLOCK_DEFAULT_VALUE;
};

InsOpcodeArgCountPair g_instructionFunctions[256];
bool g_insFunctionsInitialised = false;

bool InitInstructionSubsystem(Emulator::CPUState* cpu, uint64_t startingIP, MMU* mmu) {
    if (!g_insFunctionsInitialised) {
        memset(g_instructionFunctions, 0, sizeof(g_instructionFunctions));
#define SETINSFUNC(op, Func, args) g_instructionFunctions[static_cast<int>(InsEncoding::Opcode::op)] = {reinterpret_cast<void*>(Func), args}
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
        g_insFunctionsInitialised = true;
    }

    if (cpu == nullptr || mmu == nullptr)
        return false;

    CPUInsState* state = new CPUInsState(cpu);
    cpu->insState = state;

    state->rawIPPointer = cpu->registers.IP->GetRawValuePointer();
    state->rawNextIPPointer = &cpu->nextIP;

    InitInsCache(state, startingIP, mmu);

    return true;
}

void InitInsCache(CPUInsState* state, uint64_t startingIP, MMU *mmu) {
    state->insCache.Init(mmu, startingIP);
    state->currentCacheOffset = 0;
    state->currentInstruction = &state->instructionDataCache[0];
    state->cacheJustMissed = true;
    for (int i = 0; i < 128; i++) {
        state->instructionDataCache[i].used = false;
        state->instructionDataCache[i].IP = 0;
        state->instructionDataCache[i].pair.function = nullptr;
        state->instructionDataCache[i].pair.argCount = 0;
    }
}

bool UpdateInsCacheMMU(Emulator::CPUState* cpu, MMU *mmu) {
    if (cpu == nullptr || cpu->insState == nullptr)
        return false;
    CPUInsState* state = cpu->insState;
    state->insCache.UpdateMMU(mmu);
    return true;
}

void InsCache_MaybeSetBaseAddress(CPUInsState* state, uint64_t IP) {
    if (state == nullptr)
        return;
    state->insCache.MaybeSetBaseAddress(IP);
}

void InsCache_MaybeSetBaseAddress(Emulator::CPUState* cpu, uint64_t IP) {
    if (cpu == nullptr || cpu->insState == nullptr)
        return;
    CPUInsState* state = cpu->insState;
    state->insCache.MaybeSetBaseAddress(IP);
}

int FindIPInInsCache(CPUInsState* state, uint64_t IP) {
    if (state->currentInstruction->used && state->currentInstruction->IP == IP)
        return state->currentCacheOffset;
    for (int i = state->currentCacheOffset - 1; i > 0; i--) {
        if (state->instructionDataCache[i].used && state->instructionDataCache[i].IP == IP)
            return i;
    }
    for (int i = 127; i > state->currentCacheOffset; i--) {
        if (state->instructionDataCache[i].used && state->instructionDataCache[i].IP == IP)
            return i;
    }
    return -1;
}

void AddBreakpoint(Emulator::CPUState* cpu, uint64_t address, std::function<void(uint64_t)> callback) {
    if (cpu == nullptr || cpu->insState == nullptr)
        return;
    CPUInsState* state = cpu->insState;
    spinlock_acquire(&state->breakpointsLock);
    state->breakpoints[address] = std::move(callback);
    spinlock_release(&state->breakpointsLock);
    if (state->breakpointsEnabled.load() == 0)
        state->breakpointsEnabled.store(1);
}

void RemoveBreakpoint(Emulator::CPUState* cpu, uint64_t address) {
    if (cpu == nullptr || cpu->insState == nullptr)
        return;
    CPUInsState* state = cpu->insState;
    spinlock_acquire(&state->breakpointsLock);
    state->breakpoints.erase(address);
    spinlock_release(&state->breakpointsLock);
    if (state->breakpoints.empty())
        state->breakpointsEnabled.store(0);
}

void AllowExecution(Emulator::CPUState* cpu, void** oldState) {
    if (cpu == nullptr || cpu->insState == nullptr)
        return;
    CPUInsState* state = cpu->insState;
    if (oldState != nullptr) {
        InstructionExecutionRunState* s = static_cast<InstructionExecutionRunState*>(*oldState);
        state->allowOneInstruction.store(s->AllowOne);
        state->executionAllowed.store(s->Allowed);
        state->terminateExecution.store(s->Terminate);

        state->allowOneInstruction.notify_all();
        state->executionAllowed.notify_all();

        delete s;
    } else {
        state->terminateExecution.store(0);
        state->executionAllowed.store(1);
        state->executionAllowed.notify_all();
    }
}

void AllowOneInstruction(Emulator::CPUState* cpu) {
    if (cpu == nullptr || cpu->insState == nullptr)
        return;
    CPUInsState* state = cpu->insState;
    state->allowOneInstruction.store(1);
    state->executionAllowed.store(1);
    state->executionAllowed.notify_all();
    state->allowOneInstruction.wait(1);
    state->executionRunning.wait(1);
}

void PauseExecution(Emulator::CPUState* cpu) {
    if (cpu == nullptr || cpu->insState == nullptr)
        return;
    CPUInsState* state = cpu->insState;
    state->executionAllowed.store(0);
    state->executionRunning.wait(1);
}

void StopExecution(Emulator::CPUState* cpu, void** state) {
    if (cpu == nullptr || cpu->insState == nullptr)
        return;
    CPUInsState* insState = cpu->insState;
    if (state != nullptr) {
        InstructionExecutionRunState* s = new InstructionExecutionRunState();
        s->Terminate = insState->terminateExecution.load() == 1;
        s->Running = insState->executionRunning.load() == 1;
        s->Allowed  = insState->executionAllowed.load() == 1;
        s->AllowOne = insState->allowOneInstruction.load() == 1;
        *state = s;
    }

    insState->terminateExecution.store(1);
    insState->executionRunning.wait(1);
}

void StartExecution(Emulator::CPUState* cpu) {
    if (cpu == nullptr || cpu->insState == nullptr)
        return;
    Emulator::g_currentCPUState = cpu; // need to set the thread_local variable
    CPUInsState* state = cpu->insState;
    state->executionAllowed.wait(0); // need to wait for the main thread to be ready

#ifdef EMULATOR_DEBUG
    fprintf(stderr, "Emulator: CPU %lu is online!\n", cpu->ID);
#endif

    return ExecutionLoop(cpu);
}

void ExecutionLoop(Emulator::CPUState* cpu) {
    if (cpu == nullptr || cpu->insState == nullptr)
        return;
    CPUInsState* state = cpu->insState;
    while (true) {
        uint64_t IP = *state->rawIPPointer;
        if (state->terminateExecution.load() == 1) {
            state->executionRunning.store(0);
            state->executionRunning.notify_all();
            break; // completely stop execution
        }
        if (state->executionAllowed.load() == 0) {
            if (state->executionRunning.load() == 1) {
                state->executionRunning.store(0);
                state->executionRunning.notify_all();
            }
            state->executionAllowed.wait(0);
            continue; // still looping through instructions, just not doing anything
        }
        else if (state->executionRunning.load() == 0) {
            state->executionRunning.store(1);
            state->executionRunning.notify_all();
        }

        if (state->allowOneInstruction.load() == 1) {
            state->executionRunning.store(1);
            state->allowOneInstruction.store(0);
            state->allowOneInstruction.notify_all();
            state->executionAllowed.store(0);
        }

        if (state->breakpointsEnabled.load() == 1 && state->allowOneInstruction.load() == 0) { // don't check on single step
            // fprintf(stderr, "Breakpoint check at 0x%lx\n", IP);
            spinlock_acquire(&state->breakpointsLock);
            auto it = state->breakpoints.find(IP);
            if (it != state->breakpoints.end()) {
                state->executionRunning.store(0);
                state->executionRunning.notify_all();
                state->executionAllowed.store(0);
                state->executionAllowed.notify_all();

                state->currentBreakpoint.first = IP;
                state->currentBreakpoint.second = it->second;
                state->breakpointHit = true;
                state->breakpoints.erase(it);
                spinlock_release(&state->breakpointsLock);

                state->currentBreakpoint.second(IP);

                continue;
            }
            spinlock_release(&state->breakpointsLock);
        }

        if (state->breakpointHit && state->currentBreakpoint.first != IP) {
            spinlock_acquire(&state->breakpointsLock);
            state->breakpoints[state->currentBreakpoint.first] = state->currentBreakpoint.second;
            state->breakpointHit = false;
            spinlock_release(&state->breakpointsLock);
        }

        if (int offset = FindIPInInsCache(state, IP); offset != -1) {
            state->currentInstruction = &state->instructionDataCache[offset];
            state->currentCacheOffset = offset;
            state->cacheJustMissed = false;
        }
        else {
            if (state->currentInstruction->used) {
                // need to find a different slot
                int i_offset = -1;
                for (int i = state->currentCacheOffset + 1; i < 128; i++) { // start with going from the current offset to the end
                    if (!state->instructionDataCache[i].used) {
                        i_offset = i;
                        break;
                    }
                }
                if (i_offset == -1) {
                    // start from 0, until current offset
                    for (int i = 0; i < state->currentCacheOffset; i++) {
                        if (!state->instructionDataCache[i].used) {
                            i_offset = i;
                            break;
                        }
                    }
                    if (i_offset == -1) {
                        // all used, start from current + 1
                        if (state->currentCacheOffset == 127)
                            i_offset = 0;
                        else
                            i_offset = state->currentCacheOffset + 1;
                    }
                }
                state->currentCacheOffset = i_offset;
                state->currentInstruction = &state->instructionDataCache[i_offset];
            }
            state->currentInstruction->used = false;
            state->currentInstruction->IP = 0;
            state->currentInstruction->pair.function = nullptr;
            state->currentInstruction->pair.argCount = 0;
            uint64_t currentOffset = 0;
            if (!state->cacheJustMissed) {
                state->insCache.MaybeSetBaseAddress(IP);
                state->cacheJustMissed = true;
            }
            if (!DecodeInstruction(state->insCache, currentOffset, &state->currentInstruction->instruction, state->currentCacheOffset, [](const char* message, void* data) {
        #ifdef EMULATOR_DEBUG
                printf("Decoding error: %s\n", message);
        #else
                (void)message;
        #endif
                Emulator::CPUState* cpu_state = static_cast<Emulator::CPUState*>(data);
                cpu_state->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            }, cpu))
                cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            InsEncoding::SimpleInstruction& currentIns = state->currentInstruction->instruction;
            ComplexData* complex = state->currentInstruction->complex;
            uint8_t Opcode = static_cast<uint8_t>(currentIns.GetOpcode());
            for (uint64_t i = 0; i < currentIns.operandCount; i++) {
                switch (InsEncoding::Operand* op = &currentIns.operands[i]; op->type) {
                case InsEncoding::OperandType::REGISTER: {
                    InsEncoding::Register* tempReg = static_cast<InsEncoding::Register*>(op->data);
                    Register* reg = cpu->registerLookup[static_cast<uint8_t>(*tempReg)];
                    state->currentInstruction->operands[i] = Operand(cpu, static_cast<OperandSize>(op->size), reg);
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
                        cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
                        break;
                    }
                    state->currentInstruction->operands[i] = Operand(cpu, static_cast<OperandSize>(op->size), data);
                    break;
                }
                case InsEncoding::OperandType::MEMORY: {
                    uint64_t* temp = static_cast<uint64_t*>(op->data);
                    state->currentInstruction->operands[i] = Operand(cpu, static_cast<OperandSize>(op->size), *temp, cpu->currentMMU);
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
                            Register* reg = cpu->registerLookup[static_cast<uint8_t>(*tempReg)];
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
                            Register* reg = cpu->registerLookup[static_cast<uint8_t>(*tempReg)];
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
                            Register* reg = cpu->registerLookup[static_cast<uint8_t>(*tempReg)];
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
                    state->currentInstruction->operands[i] = Operand(cpu, static_cast<OperandSize>(op->size), &complex[i], cpu->currentMMU);
                    break;
                }
                default:
                    cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
                    break;
                }
            }
            state->currentInstruction->IP = IP;

            // Get the instruction
            state->currentInstruction->pair = g_instructionFunctions[Opcode];
            if (state->currentInstruction->pair.function == nullptr)
                cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
            state->currentInstruction->size = currentOffset;
            state->currentInstruction->used = true;
        }

        // Increment instruction pointer
        *state->rawNextIPPointer = IP + state->currentInstruction->size;

        InsOpcodeArgCountPair pair = state->currentInstruction->pair;
        Operand* operands = state->currentInstruction->operands;

        // Update the cache
        state->currentCacheOffset++;
        if (state->currentCacheOffset >= 128) // wrap around
            state->currentCacheOffset = 0;
        state->currentInstruction = &state->instructionDataCache[state->currentCacheOffset];

        // Execute the instruction
        if (pair.argCount == 0)
            reinterpret_cast<void (*)(Emulator::CPUState* cpu)>(pair.function)(cpu);
        else if (pair.argCount == 1)
            reinterpret_cast<void (*)(Emulator::CPUState* cpu, Operand*)>(pair.function)(cpu, &operands[0]);
        else if (pair.argCount == 2)
            reinterpret_cast<void (*)(Emulator::CPUState* cpu, Operand*, Operand*)>(pair.function)(cpu, &operands[0], &operands[1]);
        else if (pair.argCount == 3)
            reinterpret_cast<void (*)(Emulator::CPUState* cpu, Operand*, Operand*, Operand*)>(pair.function)(cpu, &operands[0], &operands[1], &operands[2]);
        else
            cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);

        // Set the IP to the next instruction
        *state->rawIPPointer = *state->rawNextIPPointer;


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
    void ins_##name(Emulator::CPUState* cpu, Operand* dst2, Operand* dst1, Operand* src) {                         \
        PRINT_INS_INFO3(dst2, dst1, src);                                                 \
        x86_64_128Data result = x86_64_##name(dst1->GetValue(), src->GetValue(), cpu->registers.STS->GetRawValuePointer()); \
        dst1->SetValue(result.low);                                                       \
        dst2->SetValue(result.high);                                                      \
    }

#define DIV_INSTRUCTION3(name)                                           \
    void ins_##name(Emulator::CPUState* cpu, Operand* dst2, Operand* dst1, Operand* src) {       \
        PRINT_INS_INFO3(dst2, dst1, src);                               \
        x86_64_128Data dividend = {dst1->GetValue(), dst2->GetValue()};  \
        x86_64_128Data result = x86_64_##name(dividend, src->GetValue(), cpu->registers.STS->GetRawValuePointer()); \
        dst1->SetValue(result.low);                                      \
        dst2->SetValue(result.high);                                     \
    }

#define ALU_INSTRUCTION2(name)                                                  \
    void ins_##name(Emulator::CPUState* cpu, Operand* dst, Operand* src) {                               \
        PRINT_INS_INFO2(dst, src);                                              \
        dst->SetValue(x86_64_##name(dst->GetValue(), src->GetValue(), cpu->registers.STS->GetRawValuePointer())); \
    }

#define ALU_INSTRUCTION2_NO_RET_VAL(name)                        \
    void ins_##name(Emulator::CPUState* cpu, Operand* dst, Operand* src) {                \
        PRINT_INS_INFO2(dst, src);                               \
        x86_64_##name(dst->GetValue(), src->GetValue(), cpu->registers.STS->GetRawValuePointer()); \
    }

#define ALU_INSTRUCTION1(name)                                 \
    void ins_##name(Emulator::CPUState* cpu, Operand* dst) {                            \
        PRINT_INS_INFO1(dst);                                  \
        dst->SetValue(x86_64_##name(dst->GetValue(), cpu->registers.STS->GetRawValuePointer())); \
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

void ins_ret(Emulator::CPUState* cpu) {
    PRINT_INS_INFO0();
    uint64_t IP = cpu->stack->pop();
    *cpu->insState->rawNextIPPointer = IP;
    cpu->insState->insCache.MaybeSetBaseAddress(IP);
}

void ins_call(Emulator::CPUState* cpu, Operand* dst) {
    PRINT_INS_INFO1(dst);
    cpu->stack->push(*cpu->insState->rawNextIPPointer);
    uint64_t IP = dst->GetValue();
    *cpu->insState->rawNextIPPointer = IP;
    cpu->insState->insCache.MaybeSetBaseAddress(IP);
}

void ins_jmp(Emulator::CPUState* cpu, Operand* dst) {
    PRINT_INS_INFO1(dst);
    uint64_t IP = dst->GetValue();
    *cpu->insState->rawNextIPPointer = IP;
    cpu->insState->insCache.MaybeSetBaseAddress(IP);
}

void ins_jc(Emulator::CPUState* cpu, Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = cpu->registers.STS->GetValue(); flags & 1) {
        uint64_t IP = dst->GetValue();
        *cpu->insState->rawNextIPPointer = IP;
        cpu->insState->insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jnc(Emulator::CPUState* cpu, Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = cpu->registers.STS->GetValue(); !(flags & 1)) {
        uint64_t IP = dst->GetValue();
        *cpu->insState->rawNextIPPointer = IP;
        cpu->insState->insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jz(Emulator::CPUState* cpu, Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = cpu->registers.STS->GetValue(); flags & 2) {
        uint64_t IP = dst->GetValue();
        *cpu->insState->rawNextIPPointer = IP;
        cpu->insState->insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jnz(Emulator::CPUState* cpu, Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = cpu->registers.STS->GetValue(); !(flags & 2)) {
        uint64_t IP = dst->GetValue();
        *cpu->insState->rawNextIPPointer = IP;
        cpu->insState->insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jl(Emulator::CPUState* cpu, Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = cpu->registers.STS->GetValue(); (flags & 4) != (flags & 8)) {
        uint64_t IP = dst->GetValue();
        *cpu->insState->rawNextIPPointer = IP;
        cpu->insState->insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jle(Emulator::CPUState* cpu, Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = cpu->registers.STS->GetValue(); (flags & 4) != (flags & 8) || (flags & 2)) {
        uint64_t IP = dst->GetValue();
        *cpu->insState->rawNextIPPointer = IP;
        cpu->insState->insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jnl(Emulator::CPUState* cpu, Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = cpu->registers.STS->GetValue(); (flags & 4) == (flags & 8)) {
        uint64_t IP = dst->GetValue();
        *cpu->insState->rawNextIPPointer = IP;
        cpu->insState->insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_jnle(Emulator::CPUState* cpu, Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (uint64_t flags = cpu->registers.STS->GetValue(); (flags & 4) == (flags & 8) && !(flags & 2)) {
        uint64_t IP = dst->GetValue();
        *cpu->insState->rawNextIPPointer = IP;
        cpu->insState->insCache.MaybeSetBaseAddress(IP);
    }
}

void ins_mov(Emulator::CPUState*, Operand* dst, Operand* src) {
    PRINT_INS_INFO2(dst, src);
    dst->SetValue(src->GetValue());
}

void ins_nop(Emulator::CPUState*) {
    PRINT_INS_INFO0();
}

void ins_hlt(Emulator::CPUState*) {
    PRINT_INS_INFO0();
    Emulator::HandleHalt();
}

void ins_push(Emulator::CPUState* cpu, Operand* src) {
    PRINT_INS_INFO1(src);
    cpu->stack->push(src->GetValue());
}

void ins_pop(Emulator::CPUState* cpu, Operand* dst) {
    PRINT_INS_INFO1(dst);
    dst->SetValue(cpu->stack->pop());
}

void ins_pusha(Emulator::CPUState* cpu) {
    PRINT_INS_INFO0();
    uint64_t buffer[16];
    for (int i = 0; i < 16; i++)
        buffer[i] = cpu->registers.GPR[i]->GetValue();
    cpu->stack->BulkPush(buffer, 16);
}

void ins_popa(Emulator::CPUState* cpu) {
    PRINT_INS_INFO0();
    uint64_t buffer[16];
    cpu->stack->BulkPop(buffer, 16);
    for (int i = 0; i < 16; i++)
        cpu->registers.GPR[i]->SetValue(buffer[i]);
}

void ins_int(Emulator::CPUState* cpu, Operand* number) {
    PRINT_INS_INFO1(number);
    if (Emulator::isInProtectedMode(cpu) && Emulator::isInUserMode(cpu))
        cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    uint64_t interrupt = number->GetValue();
    cpu->interruptHandler->RaiseInterrupt(interrupt, *cpu->insState->rawNextIPPointer);
}

void ins_lidt(Emulator::CPUState* cpu, Operand* src) {
    PRINT_INS_INFO1(src);
    if (Emulator::isInProtectedMode(cpu) && Emulator::isInUserMode(cpu))
        cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    cpu->interruptHandler->SetIDTR(src->GetValue());
}

void ins_iret(Emulator::CPUState* cpu) {
    PRINT_INS_INFO0();
    if (Emulator::isInProtectedMode(cpu) && Emulator::isInUserMode(cpu))
        cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    cpu->interruptHandler->ReturnFromInterrupt();
}

void ins_syscall(Emulator::CPUState* cpu) {
    PRINT_INS_INFO0();
    if (Emulator::isInProtectedMode(cpu) && !Emulator::isInUserMode(cpu))
        cpu->exceptionHandler->RaiseException(Exception::SUPERVISOR_MODE_VIOLATION);
    Emulator::ExitUserMode(cpu);
}

void ins_sysret(Emulator::CPUState* cpu) {
    PRINT_INS_INFO0();
    if (Emulator::isInProtectedMode(cpu) && Emulator::isInUserMode(cpu))
        cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    Emulator::EnterUserMode(cpu);
}

void ins_enteruser(Emulator::CPUState* cpu, Operand* dst) {
    PRINT_INS_INFO1(dst);
    if (Emulator::isInProtectedMode(cpu) && Emulator::isInUserMode(cpu))
        cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    Emulator::EnterUserMode(cpu, dst->GetValue());
}