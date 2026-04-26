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

#include "Emulator.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

#include <LibArch/Instruction.hpp>

#include <DebugInterface.hpp>
#include <Exceptions.hpp>
#include <Interrupts.hpp>
#include <Register.hpp>
#include <Stack.hpp>

#include <IO/IOBus.hpp>
#include <IO/IOInterfaceManager.hpp>
#include <IO/IOMemoryRegion.hpp>

#include <IO/Devices/ConsoleDevice.hpp>

#include <IO/Devices/Input/InputDeviceBus.hpp>

#include <IO/Devices/Storage/StorageDevice.hpp>

#include <IO/Devices/Video/VideoDevice.hpp>

#include <Instruction/Instruction.hpp>

#include <MMU/BIOSMemoryRegion.hpp>
#include <MMU/MMU.hpp>
#include <MMU/StandardMemoryRegion.hpp>
#include <MMU/SystemControlMemoryRegion.hpp>
#include <MMU/VirtualMMU.hpp>

#include <OSSpecific/Signal.hpp>

#include <Common/Util.hpp>

namespace Emulator {

    void EmulatorMain(uint64_t cpuCount);

    struct RegisterSyncData {
        CPUState* state;
        InsEncoding::Register reg;
    };

    ConsoleDevice* g_ConsoleDevice;
    VideoDevice* g_VideoDevice;
    StorageDevice* g_StorageDevice;
    InputDeviceBus* g_InputDeviceBus;

    MMU g_physicalMMU;

    uint64_t g_ramSize = 0;

    bool g_emulatorRunning = false;

    LinkedList::LockableLinkedList<Event> g_events;
    std::atomic_uchar g_eventWait = 0;

    std::thread* EmulatorThread;

    SystemControlMemoryRegion* g_SysControlMemoryRegion;
    BIOSMemoryRegion* g_BIOSMemoryRegion;

    DebugInterface* g_DebugInterface = nullptr;

    CPUState* g_cpuStates;
    uint64_t g_cpuCount;

    thread_local CPUState* g_currentCPUState = nullptr;

    void RaiseEvent(Event event) {
        g_events.lock();
        Event* new_event = new Event(event);
        g_events.insert(new_event);
        g_events.unlock();
        g_eventWait.store(1);
        g_eventWait.notify_all();
    }

    // what the EmulatorThread will run. just loops waiting for events.
    void WaitForOperation() {
        while (true) {
            g_eventWait.wait(0);
            if (g_eventWait.load() == 1)
                g_eventWait.store(0);

            g_events.lock();
            if (g_events.getCount() == 0) {
                g_events.unlock();
                continue;
            }
            for (uint64_t i = 0; i < g_events.getCount(); i++) {
                Event* event = g_events.getHead();
                switch (event->type) {
                case EventType::SwitchToIP: {
                    // assuming that the execution thread has joined this thread
                    CPUState* state = event->state;
                    if (state == nullptr)
                        Crash("No CPU state on SwitchToIP event");
                    if (state->executionThread == nullptr)
                        Crash("No executionThread for SwitchToIP event");
                    state->executionThread->detach();
                    delete state->executionThread;
                    state->registers.IP->SetValue(event->data);
                    InsCache_MaybeSetBaseAddress(state, event->data);
                    state->executionThread = new std::thread(StartExecution, state);
                    break;
                }
                case EventType::NewMMU: {
                    // assuming that the execution thread has joined this thread
                    CPUState* state = event->state;
                    if (state == nullptr)
                        Crash("No CPU state on SwitchToIP event");
                    state->interruptHandler->ChangeMMU(state->currentMMU);
                    assert(state->executionThread != nullptr);
                    state->executionThread->detach();
                    delete state->executionThread;
                    UpdateInsCacheMMU(state, state->currentMMU);
                    state->executionThread = new std::thread(StartExecution, state);
                    break;
                }
                case EventType::StorageTransfer: {
                    StorageDevice* device = reinterpret_cast<StorageDevice*>(event->data);
                    g_currentCPUState = event->state;
                    if (g_currentCPUState == nullptr)
                        Crash("No CPU state on StorageTransfer event");
                    device->StartTransfer();
                    g_currentCPUState = nullptr;
                    break;
                }
                default:
                    break;
                }
                g_events.remove(g_events.getHead());
                delete event;
            }
            g_events.unlock();
        }
    }

    int Start(const EmulatorArgs& args) {
        if (args.firmwareSize > 0x1000'0000)
            return 1; // program too large

        g_ramSize = args.ramSize;

        // Configure the IO bus
        g_IOBus = new IOBus(&g_physicalMMU);

        // Add a SystemControlMemoryRegion
        g_SysControlMemoryRegion = new SystemControlMemoryRegion(0xFFFF'FF00, 0x1'0000'0000, g_IOBus, args.ramSize, &g_physicalMMU);
        g_physicalMMU.AddMemoryRegion(g_SysControlMemoryRegion);

        // Add a BIOSMemoryRegion
        g_BIOSMemoryRegion = new BIOSMemoryRegion(0xF000'0000, 0xFFFF'FF00, args.firmwareSize);
        g_physicalMMU.AddMemoryRegion(g_BIOSMemoryRegion);

        g_IOInterfaceManager = new IOInterfaceManager();

        // Configure the console device
        g_ConsoleDevice = new ConsoleDevice(16, args.consoleMode);
        g_IOBus->AddDevice(g_ConsoleDevice);
        g_IOInterfaceManager->AddInterfaceItem(g_ConsoleDevice);

        // Configure the debug interface
        if (args.debugConsoleMode != "disabled") {
            g_DebugInterface = new DebugInterface(IOInterfaceType::UNKNOWN, &g_physicalMMU, args.debugConsoleMode);
            g_IOInterfaceManager->AddInterfaceItem(g_DebugInterface);
            g_DebugInterface->InterfaceInit();
        }

        // Configure the video device
        if (args.has_display) {
            g_VideoDevice = new VideoDevice(args.displayType, g_physicalMMU);
            assert(g_IOBus->AddDevice(g_VideoDevice));
            g_InputDeviceBus = new InputDeviceBus(VideoBackendToInputBackend(args.displayType), g_VideoDevice);
            assert(g_IOBus->AddDevice(g_InputDeviceBus));
        }

        // Configure the storage device
        if (args.has_drive) {
            g_StorageDevice = new StorageDevice(&g_physicalMMU, args.drivePath);
            g_StorageDevice->Initialise();
            assert(g_IOBus->AddDevice(g_StorageDevice));
        }

        // Load program into RAM
        g_physicalMMU.WriteBuffer(0xF000'0000, args.firmware, args.firmwareSize);

        ConfigureEmulatorSignalHandlers(nullptr, nullptr);

        g_emulatorRunning = true;

        EmulatorMain(args.cpuCount);
        return 0;
    }

    void StartCPU(CPUState* state, uint64_t startingIP) {
        memset(&state->registers, 0, sizeof(CPUState) - sizeof(uint64_t));
        state->stateLock = SPINLOCK_LOCKED_VALUE; // initialise as locked

        state->registers.SCP = new Register(RegisterType::Stack, 0, true);
        state->registers.SBP = new Register(RegisterType::Stack, 1, true);
        state->registers.STP = new Register(RegisterType::Stack, 2, true);

        state->stack = new Stack(&g_physicalMMU, *state->registers.SBP, *state->registers.STP, *state->registers.SCP);

        // explicitly initialise instruction pointer to start of BIOS region, for now
        state->registers.IP = new SafeRegister(state, RegisterType::Instruction, 0, false, startingIP);
        state->nextIP = 0;

        // Init all the other registers
        for (int i = 0; i < 16; i++)
            state->registers.GPR[i] = new Register(RegisterType::GeneralPurpose, i, true);
        for (int i = 0; i < 8; i++)
            state->registers.Control[i] = new SafeSyncingRegister(state, RegisterType::Control, i, true);
        state->registers.STS = new SafeRegister(state, RegisterType::Status, 0, false, 0);

        state->registersInitialised = true;

        // Build the register lookup table
        for (int i = 0; i < 16; i++)
            state->registerLookup[static_cast<int>(InsEncoding::Register::r0) + i] = state->registers.GPR[i];
        state->registerLookup[static_cast<int>(InsEncoding::Register::scp)] = state->registers.SCP;
        state->registerLookup[static_cast<int>(InsEncoding::Register::sbp)] = state->registers.SBP;
        state->registerLookup[static_cast<int>(InsEncoding::Register::stp)] = state->registers.STP;
        for (int i = 0; i < 8; i++)
            state->registerLookup[static_cast<int>(InsEncoding::Register::cr0) + i] = state->registers.Control[i];
        state->registerLookup[static_cast<int>(InsEncoding::Register::sts)] = state->registers.STS;
        state->registerLookup[static_cast<int>(InsEncoding::Register::ip)] = state->registers.IP;

        // Setup callbacks for CR0 and CR3
        RegisterSyncData* syncData = new RegisterSyncData();
        syncData->reg = InsEncoding::Register::cr0;
        syncData->state = state;
        SafeSyncingRegister* reg = state->registers.Control[0];
        reg->SetCallback({SyncRegisters, syncData});
        syncData = new RegisterSyncData();
        syncData->reg = InsEncoding::Register::cr3;
        syncData->state = state;
        reg = state->registers.Control[3];
        reg->SetCallback({SyncRegisters, syncData});

        // MMU - just physical for now
        state->currentMMU = &g_physicalMMU;

        // Interrupts & Exceptions
        state->interruptHandler = new InterruptHandler(state, &g_physicalMMU);
        state->exceptionHandler = new ExceptionHandler(state, state->interruptHandler);

        if (!InitInstructionSubsystem(state, startingIP, state->currentMMU)) {
            spinlock_release(&state->stateLock);
            Crash("Failed to initialise instruction subsystem");
        }

        state->executionThread = new std::thread(StartExecution, state);

        spinlock_release(&state->stateLock);
    }

    void DumpRegisters(CPUState* state, FILE* fp) {
        if (state == nullptr)
            return;
        if (!state->registersInitialised)
            return;
        fprintf(fp, "Registers:\n");
        fprintf(fp, "R0 =%016lx R1 =%016lx R2 =%016lx R3 =%016lx\n", state->registers.GPR[0]->GetValue(), state->registers.GPR[1]->GetValue(), state->registers.GPR[2]->GetValue(), state->registers.GPR[3]->GetValue());
        fprintf(fp, "R4 =%016lx R5 =%016lx R6 =%016lx R7 =%016lx\n", state->registers.GPR[4]->GetValue(), state->registers.GPR[5]->GetValue(), state->registers.GPR[6]->GetValue(), state->registers.GPR[7]->GetValue());
        fprintf(fp, "R8 =%016lx R9 =%016lx R10=%016lx R11=%016lx\n", state->registers.GPR[8]->GetValue(), state->registers.GPR[9]->GetValue(), state->registers.GPR[10]->GetValue(), state->registers.GPR[11]->GetValue());
        fprintf(fp, "R12=%016lx R13=%016lx R14=%016lx R15=%016lx\n", state->registers.GPR[12]->GetValue(), state->registers.GPR[13]->GetValue(), state->registers.GPR[14]->GetValue(), state->registers.GPR[15]->GetValue());
        fprintf(fp, "SCP=%016lx SBP=%016lx STP=%016lx\n", state->registers.SCP->GetValue(), state->registers.SBP->GetValue(), state->registers.STP->GetValue());
        fprintf(fp, "IP =%016lx\n", state->registers.IP->GetValue());
        fprintf(fp, "CR0=%016lx CR1=%016lx CR2=%016lx CR3=%016lx\n", state->registers.Control[0]->GetValue(), state->registers.Control[1]->GetValue(), state->registers.Control[2]->GetValue(), state->registers.Control[3]->GetValue());
        fprintf(fp, "CR4=%016lx CR5=%016lx CR6=%016lx CR7=%016lx\n", state->registers.Control[4]->GetValue(), state->registers.Control[5]->GetValue(), state->registers.Control[6]->GetValue(), state->registers.Control[7]->GetValue());
        fprintf(fp, "STS = %016lx\n", state->registers.STS->GetValue());
    }

    void DumpRegisters(CPUState* state, void (*write)(void*, const char*, ...), void* data) {
        if (state == nullptr)
            return;
        if (!state->registersInitialised)
            return;
        if (write == nullptr)
            return DumpRegisters(state, stdout);

        write(data, "Registers:\n");
        write(data, "R0 =%016lx R1 =%016lx R2 =%016lx R3 =%016lx\n", state->registers.GPR[0]->GetValue(), state->registers.GPR[1]->GetValue(), state->registers.GPR[2]->GetValue(), state->registers.GPR[3]->GetValue());
        write(data, "R4 =%016lx R5 =%016lx R6 =%016lx R7 =%016lx\n", state->registers.GPR[4]->GetValue(), state->registers.GPR[5]->GetValue(), state->registers.GPR[6]->GetValue(), state->registers.GPR[7]->GetValue());
        write(data, "R8 =%016lx R9 =%016lx R10=%016lx R11=%016lx\n", state->registers.GPR[8]->GetValue(), state->registers.GPR[9]->GetValue(), state->registers.GPR[10]->GetValue(), state->registers.GPR[11]->GetValue());
        write(data, "R12=%016lx R13=%016lx R14=%016lx R15=%016lx\n", state->registers.GPR[12]->GetValue(), state->registers.GPR[13]->GetValue(), state->registers.GPR[14]->GetValue(), state->registers.GPR[15]->GetValue());
        write(data, "SCP=%016lx SBP=%016lx STP=%016lx\n", state->registers.SCP->GetValue(), state->registers.SBP->GetValue(), state->registers.STP->GetValue());
        write(data, "IP =%016lx\n", state->registers.IP->GetValue());
        write(data, "CR0=%016lx CR1=%016lx CR2=%016lx CR3=%016lx\n", state->registers.Control[0]->GetValue(), state->registers.Control[1]->GetValue(), state->registers.Control[2]->GetValue(), state->registers.Control[3]->GetValue());
        write(data, "CR4=%016lx CR5=%016lx CR6=%016lx CR7=%016lx\n", state->registers.Control[4]->GetValue(), state->registers.Control[5]->GetValue(), state->registers.Control[6]->GetValue(), state->registers.Control[7]->GetValue());
        write(data, "STS = %016lx\n", state->registers.STS->GetValue());
    }

    void DumpRAM(FILE* fp) {
        fprintf(fp, "RAM:\n");
        g_physicalMMU.DumpMemory(fp);
        fprintf(fp, "\n");
    }

    void EmulatorMain(uint64_t cpuCount) {
        g_cpuStates = new CPUState[cpuCount];
        g_cpuCount = cpuCount;

        for (uint64_t i = 0; i < cpuCount; i++) {
            CPUState* cpu = &g_cpuStates[i];
            cpu->ID = i;
            StartCPU(cpu, i == 0 ? 0xF000'0000 : 0);
        }

        // setup instruction switch handling
        EmulatorThread = new std::thread(WaitForOperation);

        AllowExecution(&g_cpuStates[0]);

        // join with the emulator thread
        EmulatorThread->join();
    }

    [[noreturn]] void JumpToIP(CPUState* state, uint64_t value) {
        RaiseEvent({EventType::SwitchToIP, state, value});
        EmulatorThread->join();
        Crash("Emulator thread exited unexpectedly"); // should be unreachable
    }

    void JumpToIPExternal(CPUState* state, uint64_t value) {
        state->registers.IP->SetValue(value);
        InsCache_MaybeSetBaseAddress(state, value);
        state->executionThread = new std::thread(StartExecution, state); // already allowed to execute from when it was killed.
    }

    void SyncRegisters(void* data, uint64_t value) {
        RegisterSyncData* syncData = static_cast<RegisterSyncData*>(data);
        if (syncData == nullptr || syncData->state == nullptr)
            return;
        CPUState* cpu = syncData->state;
        if (syncData->reg == InsEncoding::Register::cr0) {
            bool wasInProtectedMode = cpu->privilegeMode == PrivilegeMode::PROTECTED_MODE;
            cpu->privilegeMode = value & 1 ? PrivilegeMode::PROTECTED_MODE : PrivilegeMode::REAL_MODE;
            if (((value & 2) > 0) != cpu->isPagingEnabled) {
                cpu->isPagingEnabled = (value & 2) > 0;
                if (cpu->isPagingEnabled) {
                    PageSize pageSize = static_cast<PageSize>((value & 0xC) >> 2);
                    PageTableLevelCount pageTableLevelCount = static_cast<PageTableLevelCount>((value & 0x30) >> 4);
                    if (pageSize == PS_64KiB && pageTableLevelCount == PTLC_5) {
                        // restore any changes
                        if (!wasInProtectedMode && cpu->privilegeMode == PrivilegeMode::PROTECTED_MODE)
                            cpu->privilegeMode = PrivilegeMode::REAL_MODE;
                        cpu->isPagingEnabled = false;
                        cpu->exceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
                    }
                    uint64_t pageTableRoot = cpu->registers.Control[3]->GetValue();
                    cpu->virtualMMU = new VirtualMMU(cpu, &g_physicalMMU, pageTableRoot, pageSize, pageTableLevelCount);
                    cpu->currentMMU = cpu->virtualMMU;
                } else {
                    cpu->currentMMU = &g_physicalMMU;
                    delete cpu->virtualMMU;
                }
                cpu->registers.IP->SetValue(cpu->nextIP);
                RaiseEvent({EventType::NewMMU, cpu, 0});
                EmulatorThread->join();
                Crash("Emulator thread exited unexpectedly"); // should be unreachable
            }
        }
        if (syncData->reg == InsEncoding::Register::cr3 && cpu->isPagingEnabled)
            cpu->virtualMMU->SetPageTableRoot(value);
    }

    [[noreturn]] void Crash(const char* message) {
        g_emulatorRunning = false;
        printf("Crash: %s\n", message);
        for (uint64_t i = 0; i < g_cpuCount; i++) {
            CPUState* state = &g_cpuStates[i];
            printf("CPU %lu:\n", state->ID);
            DumpRegisters(state, stdout);
            putc('\n', stdout);
        }
        exit(0);
    }

    void HandleHalt() {
        // DumpRAM(stdout);
        // DumpRegisters(stdout);
        g_emulatorRunning = false;
        exit(0);
    }

    bool isInProtectedMode(CPUState* state) {
        return state->privilegeMode == PrivilegeMode::PROTECTED_MODE;
    }

    bool isInUserMode(CPUState* state) {
        return state->isInUserMode;
    }

    void EnterUserMode(CPUState* state) {
        uint64_t status = state->registers.STS->GetValue();
        state->registers.STS->SetValue(state->registers.Control[1]->GetValue(), true);
        state->registers.Control[1]->SetValue(status, true);
        state->nextIP = state->registers.GPR[14]->GetValue();
        state->registers.SCP->SetValue(state->registers.GPR[15]->GetValue());
        state->isInUserMode = true;
    }

    void EnterUserMode(CPUState* state, uint64_t address) {
        state->registers.STS->SetValue(0, true);
        state->nextIP = address;
        state->isInUserMode = true;
    }

    void ExitUserMode(CPUState* state) {
        state->isInUserMode = false;
        uint64_t status = state->registers.STS->GetValue();
        state->registers.STS->SetValue(state->registers.Control[1]->GetValue(), true);
        state->registers.Control[1]->SetValue(status, true);
        state->registers.GPR[14]->SetValue(state->nextIP, true);
        state->nextIP = state->registers.Control[2]->GetValue();
        state->registers.GPR[15]->SetValue(state->registers.SCP->GetValue());
    }

    void KillCurrentInstruction(CPUState* cpu) {
        if (std::this_thread::get_id() == cpu->executionThread->get_id())
            Crash("Cannot kill current instruction from the instruction thread");

        void* state = nullptr;

        StopExecution(cpu, &state); // wait for current instruction to finish executing

        cpu->executionThread->join(); // ensure the thread has finished executing
        delete cpu->executionThread;

        AllowExecution(cpu, &state);
    }

    bool isPagingEnabled(CPUState* state) {
        return state->isPagingEnabled;
    }

    DebugInterface* GetDebugInterface() {
        return g_DebugInterface;
    }

    uint64_t GetCPUCount() {
        return g_cpuCount;
    }

} // namespace Emulator
