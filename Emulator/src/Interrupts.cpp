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

#include "Interrupts.hpp"

#include <Instruction/Instruction.hpp>

#include <Emulator.hpp>
#include <Exceptions.hpp>
#include <Stack.hpp>

InterruptHandler::InterruptHandler(Emulator::CPUState* cpu, MMU* mmu) : m_cpu(cpu), m_MMU(mmu), m_IDTR(0) {
    for (int i = 0; i < 256; i++) {
        m_IDT[i].loaded = true;
        m_IDT[i].flags = 0;
        m_IDT[i].handler = 0;
    }
}

InterruptHandler::~InterruptHandler() {
}

void InterruptHandler::SetIDTR(uint64_t base) {
    m_IDTR = base;
    for (int i = 0; i < 256; i++)
        m_IDT[i].loaded = false;
}

void InterruptHandler::RaiseInterrupt(uint8_t interrupt, uint64_t IP, bool external) {
    m_lock.lock();
    if (external)
        m_cpu->registers.STS->Lock();
    uint64_t STS = m_cpu->registers.STS->GetValueNoCheck(!external);
    RaiseInterruptCommon(interrupt, IP, STS);
    if (external) {
        STS = m_cpu->registers.STS->GetValueNoCheck(false);
        m_cpu->registers.STS->SetValueNoCheck(STS | 1 << Emulator::REG_STS_INT_BIT, false);
        m_cpu->registers.STS->Unlock();
    }
    m_lock.unlock();
    Emulator::JumpToIP(m_cpu, m_IDT[interrupt].handler);
}

void InterruptHandler::RaiseInterruptExternal(uint8_t interrupt) {
    if ((m_cpu->registers.STS->GetValueNoCheck() & 1 << Emulator::REG_STS_INT_BIT) > 0)
        return; // already in an interrupt, ignore this one
    InsRaiseInterrupt(m_cpu, interrupt);
}


void InterruptHandler::ReturnFromInterrupt() {
    m_lock.lock();
    StackViolationErrorCode code = {1, 0, 0, 0};
    if (m_cpu->stack->WillUnderflowOnPop())
        m_cpu->exceptionHandler->RaiseException(Exception::STACK_VIOLATION, code);
    m_cpu->registers.STS->SetValueNoCheck(m_cpu->stack->pop());
    if (m_cpu->stack->WillUnderflowOnPop())
        m_cpu->exceptionHandler->RaiseException(Exception::STACK_VIOLATION, code);
    m_lock.unlock();
    Emulator::JumpToIP(m_cpu, m_cpu->stack->pop());
}

void InterruptHandler::ChangeMMU(MMU* mmu) {
    m_MMU = mmu;
    for (int i = 0; i < 256; i++) // invalidate all descriptors
        m_IDT[i].loaded = false;
}

InterruptDescriptor InterruptHandler::ReadDescriptor(uint8_t interrupt) {
    if (!m_MMU->ValidateRead(m_IDTR + sizeof(RawInterruptDescriptor) * interrupt, sizeof(RawInterruptDescriptor)))
        HandleFailure(interrupt);
    RawInterruptDescriptor rawDescriptor;
    m_MMU->ReadBuffer(m_IDTR + sizeof(RawInterruptDescriptor) * interrupt, reinterpret_cast<uint8_t*>(&rawDescriptor), sizeof(RawInterruptDescriptor));
    InterruptDescriptor descriptor;
    descriptor.loaded = true;
    descriptor.flags = rawDescriptor.Present;
    descriptor.handler = rawDescriptor.addr;
    return descriptor;
}

void InterruptHandler::HandleFailure(uint8_t interrupt) {
    if (static_cast<Exception>(interrupt) == Exception::UNHANDLED_INTERRUPT)
        m_cpu->exceptionHandler->RaiseException(Exception::TWICE_UNHANDLED_INTERRUPT);
    m_cpu->exceptionHandler->RaiseException(Exception::UNHANDLED_INTERRUPT, interrupt);
}

void InterruptHandler::RaiseInterruptCommon(uint8_t interrupt, uint64_t IP, uint64_t STS) {
    if (!m_IDT[interrupt].loaded)
        m_IDT[interrupt] = ReadDescriptor(interrupt);
    if ((m_IDT[interrupt].flags & 1) == 0)
        HandleFailure(interrupt);
    if (Emulator::isInProtectedMode(m_cpu) && Emulator::isInUserMode(m_cpu))
        Emulator::ExitUserMode(m_cpu);
    if (m_cpu->stack->WillOverflowOnPush())
        HandleFailure(interrupt);
    m_cpu->stack->push(IP);
    if (m_cpu->stack->WillOverflowOnPush())
        HandleFailure(interrupt);
    m_cpu->stack->push(STS);
}
