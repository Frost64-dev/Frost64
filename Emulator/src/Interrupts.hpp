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

#ifndef _INTERRUPTS_HPP
#define _INTERRUPTS_HPP

#include <stdint.h>

#include <MMU/MMU.hpp>

#define INTERRUPT_COUNT 256
#define RESERVED_INTERRUPTS 16
#define USABLE_INTERRUPTS (INTERRUPT_COUNT - RESERVED_INTERRUPTS)

struct InterruptDescriptor {
    bool loaded;
    uint8_t flags;
    uint64_t handler;
};

struct RawInterruptDescriptor {
    uint8_t Present : 1;
    uint8_t reserved : 7;
    uint64_t addr;
} __attribute__((packed));

struct IDTR {
    uint16_t limit;
    uint64_t base;
};

namespace Emulator {
    struct CPUState;
}

class ExceptionHandler;

class InterruptHandler {
public:
    InterruptHandler(Emulator::CPUState* cpu, MMU* mmu);
    ~InterruptHandler();

    void SetIDTR(uint64_t base);

    [[noreturn]] void RaiseInterrupt(uint8_t interrupt, uint64_t IP);
    void RaiseInterruptExternal(uint8_t interrupt);
    void ReturnFromInterrupt();

    void ChangeMMU(MMU* mmu);
    MMU* GetMMU() const { return m_MMU; }

private:
    InterruptDescriptor ReadDescriptor(uint8_t interrupt);
    void HandleFailure(uint8_t interrupt);

    void RaiseInterruptCommon(uint8_t interrupt, uint64_t IP);

private:
    Emulator::CPUState* m_cpu;
    MMU* m_MMU;
    InterruptDescriptor m_IDT[256];
    uint64_t m_IDTR;
};

#endif /* _INTERRUPTS_HPP */