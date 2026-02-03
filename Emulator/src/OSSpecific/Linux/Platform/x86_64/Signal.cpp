/*
Copyright (©) 2025  Frosty515

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

#include "../Signal.hpp"

#include <csignal>

#include <Emulator.hpp>
#include <Exceptions.hpp>

#include <Platform/x86_64/ALUInstruction.h>

void Linux_InstructionSignalHandler(int, siginfo_t* info, void*) {
    if (info->si_signo == SIGFPE) {
        if ((info->si_addr >= _x86_64_div_beforediv && info->si_addr <= _x86_64_div_afterdiv )|| (info->si_addr >= _x86_64_sdiv_beforediv && info->si_addr <= _x86_64_sdiv_afterdiv)) {
            // need to raise an integer overflow exception, as division by zero is handled by the relevant functions for the instructions
            if (Emulator::g_currentCPUState == nullptr || Emulator::g_currentCPUState->exceptionHandler == nullptr)
                Emulator::Crash("Linux_InstructionSignalHandler: CPUState or ExceptionHandler is null");
            Exception e;
            if (info->si_code == FPE_INTDIV)
                e = Exception::DIV_BY_ZERO;
            else if (info->si_code == FPE_INTOVF)
                e = Exception::INTEGER_OVERFLOW;
            else
                e = Exception::INVALID_INSTRUCTION; // unknown FPE code
            Emulator::g_currentCPUState->exceptionHandler->RaiseException(e);
        }
    }
    // can't handle it, so just fall back to the caller to handle it
}