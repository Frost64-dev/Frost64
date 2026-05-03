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

#ifndef _EMULATOR_HPP
#define _EMULATOR_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <thread>

#include <Register.hpp>

#include <MMU/MMU.hpp>
#include <MMU/VirtualMMU.hpp>

#include <IO/Devices/Video/VideoBackend.hpp>

#include "Interrupts.hpp"
#include "Stack.hpp"

class DebugInterface;

struct CPUInsState;

namespace Emulator {

    constexpr uint64_t REG_STS_INT_BIT = 4;

    enum StartErrors {
        SE_SUCCESS = 0,
        SE_MALLOC_FAIL = 1,
        SE_TOO_LITTLE_RAM = 2
    };

    enum class PrivilegeMode {
        REAL_MODE,
        PROTECTED_MODE
    };

    extern MMU g_physicalMMU;

    struct CPUState {
        uint64_t ID;

        // registers
        struct CPURegisters {
            SafeRegister* IP;
            Register* SCP;
            Register* SBP;
            Register* STP;
            Register* GPR[16];
            LockableSafeRegister* STS;
            SafeSyncingRegister* Control[8];
        } registers;
        bool registersInitialised = false;
        Register* registerLookup[256];
        Stack* stack;

        uint64_t nextIP;

        // MMU
        VirtualMMU* virtualMMU;
        MMU* currentMMU;

        std::thread* executionThread;

        CPUInsState* insState;

        PrivilegeMode privilegeMode = PrivilegeMode::REAL_MODE;
        bool isInUserMode = false;
        bool isPagingEnabled = false;

        InterruptHandler* interruptHandler;
        ExceptionHandler* exceptionHandler;

        spinlock_t stateLock;
    };

    extern thread_local CPUState* g_currentCPUState;

    extern CPUState* g_cpuStates;

    enum class EventType {
        NewMMU,
        StorageTransfer
    };

    struct Event {
        EventType type;
        CPUState* state;
        uint64_t data;
    };

    struct EmulatorArgs {
        uint8_t* firmware;
        size_t firmwareSize;
        size_t ramSize;
        uint64_t cpuCount;
        const std::string_view& consoleMode;
        const std::string_view& debugConsoleMode;
        bool has_display = false;
        VideoBackendType displayType = VideoBackendType::NONE;
        bool has_drive = false;
        const char* drivePath = nullptr;
    };

    void RaiseEvent(Event event);

    void WaitForOperation();

    int Start(const EmulatorArgs& args);
    void StartCPU(CPUState* state, uint64_t startingIP);

    void DumpRegisters(CPUState* state, FILE* fp);
    void DumpRegisters(CPUState* state, void (*write)(void*, const char*, ...), void* data = nullptr);
    void DumpRAM(FILE* fp);

    void EmulatorMain(uint64_t cpuCount);

    void JumpToIP(CPUState* state, uint64_t value);

    void SyncRegisters(void* data, uint64_t value);

    [[noreturn]] void Crash(const char* message);
    void HandleHalt();

    bool isInProtectedMode(CPUState* state);
    bool isInUserMode(CPUState* state);

    void EnterUserMode(CPUState* state);
    void EnterUserMode(CPUState* state, uint64_t address);
    void ExitUserMode(CPUState* state);

    bool isPagingEnabled(CPUState* state);

    DebugInterface* GetDebugInterface();
    uint64_t GetCPUCount();
} // namespace Emulator

#endif /* _EMULATOR_HPP */