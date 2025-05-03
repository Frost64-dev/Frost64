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

#include "Emulator.hpp"

#include <Common/Util.hpp>
#include <cstdint>
#include <cstdio>
#include <DebugInterface.hpp>
#include <Exceptions.hpp>
#include <Instruction/Instruction.hpp>
#include <Interrupts.hpp>
#include <IO/Devices/ConsoleDevice.hpp>
#include <IO/Devices/Storage/StorageDevice.hpp>
#include <IO/Devices/Video/VideoDevice.hpp>
#include <IO/IOBus.hpp>
#include <IO/IOInterfaceManager.hpp>
#include <IO/IOMemoryRegion.hpp>
#include <MMU/BIOSMemoryRegion.hpp>
#include <MMU/MMU.hpp>
#include <MMU/StandardMemoryRegion.hpp>
#include <MMU/VirtualMMU.hpp>
#include <Register.hpp>
#include <Stack.hpp>
#include <thread>

namespace Emulator {

    void EmulatorMain();

    struct
    {
        SafeRegister* IP;
        Register* SCP;
        Register* SBP;
        Register* STP;
        Register* GPR[16];
        SafeRegister* STS;
        SafeSyncingRegister* Control[8];
    } g_registers;

    bool g_registersInitialised = false;

    ConsoleDevice* g_ConsoleDevice;
    VideoDevice* g_VideoDevice;
    StorageDevice* g_StorageDevice;

    uint64_t g_NextIP;

    MMU g_physicalMMU;
    VirtualMMU* g_virtualMMU = nullptr;
    MMU* g_CurrentMMU = &g_physicalMMU;

    uint64_t g_ramSize = 0;

    bool g_instructionInProgress = false;

    uint64_t g_currentInstruction = 0;

    uint64_t g_currentInstructionOffset = 0;

    bool g_emulatorRunning = false;

    bool g_isInProtectedMode = false;
    bool g_isInUserMode = false;
    bool g_isPagingEnabled = false;

    LinkedList::LockableLinkedList<Event> g_events;
    std::atomic_uchar g_eventWait = 0;

    std::thread* ExecutionThread;
    std::thread* EmulatorThread;

    IOMemoryRegion* g_IOMemoryRegion;
    BIOSMemoryRegion* g_BIOSMemoryRegion;

    DebugInterface* g_DebugInterface;

    void HandleMemoryOperation(uint64_t address, void* data, uint64_t size, uint64_t count, bool write) {
        if (write) {
            for (uint64_t i = 0; i < count; i++) {
                switch (size) {
                case 1:
                    g_CurrentMMU->write8(address + i, static_cast<uint8_t*>(data)[i]);
                    break;
                case 2:
                    g_CurrentMMU->write16(address + i * 2, static_cast<uint16_t*>(data)[i]);
                    break;
                case 4:
                    g_CurrentMMU->write32(address + i * 4, static_cast<uint32_t*>(data)[i]);
                    break;
                case 8:
                    g_CurrentMMU->write64(address + i * 8, static_cast<uint64_t*>(data)[i]);
                    break;
                default:
                    printf("Invalid size: %lu\n", size);
                    abort();
                }
            }
        } else {
            for (uint64_t i = 0; i < count; i++) {
                switch (size) {
                case 1:
                    static_cast<uint8_t*>(data)[i] = g_CurrentMMU->read8(address + i);
                    break;
                case 2:
                    static_cast<uint16_t*>(data)[i] = g_CurrentMMU->read16(address + i * 2);
                    break;
                case 4:
                    static_cast<uint32_t*>(data)[i] = g_CurrentMMU->read32(address + i * 4);
                    break;
                case 8:
                    static_cast<uint64_t*>(data)[i] = g_CurrentMMU->read64(address + i * 8);
                    break;
                default:
                    printf("Invalid size: %lu\n", size);
                    abort();
                }
            }
        }
    }

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
                case EventType::SwitchToIP: // assuming that the execution thread has joined this thread
                    assert(ExecutionThread != nullptr);
                    ExecutionThread->detach();
                    delete ExecutionThread;
                    SetCPU_IP(event->data);
                    InsCache_MaybeSetBaseAddress(event->data);
                    ExecutionThread = new std::thread(ExecutionLoop);
                    break;
                case EventType::NewMMU: // assuming that the execution thread has joined this thread
                    g_InterruptHandler->ChangeMMU(g_CurrentMMU);
                    assert(ExecutionThread != nullptr);
                    ExecutionThread->detach();
                    delete ExecutionThread;
                    UpdateInsCacheMMU(g_CurrentMMU);
                    ExecutionThread = new std::thread(ExecutionLoop);
                    break;
                case EventType::StorageTransfer: {
                    StorageDevice* device = reinterpret_cast<StorageDevice*>(event->data);
                    device->StartTransfer();
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

    int Start(uint8_t* program, size_t size, const size_t ramSize, const std::string_view& consoleMode, const std::string_view& debugConsoleMode, bool has_display, VideoBackendType displayType, bool has_drive, const char* drivePath) {
        if (size > 0x1000'0000)
            return 1; // program too large

        g_ramSize = ramSize;

        // Configure the exception handler
        g_ExceptionHandler = new ExceptionHandler();

        // Configure the interrupt handler
        g_InterruptHandler = new InterruptHandler(&g_physicalMMU, g_ExceptionHandler);
        g_ExceptionHandler->SetINTHandler(g_InterruptHandler);

        // Configure the IO bus
        g_IOBus = new IOBus(&g_physicalMMU);

        // Add an IOMemoryRegion
        g_IOMemoryRegion = new IOMemoryRegion(0xFFFF'FF00, 0x1'0000'0000, g_IOBus);
        g_physicalMMU.AddMemoryRegion(g_IOMemoryRegion);

        // Add a BIOSMemoryRegion
        g_BIOSMemoryRegion = new BIOSMemoryRegion(0xF000'0000, 0xFFFF'FF00, size);
        g_physicalMMU.AddMemoryRegion(g_BIOSMemoryRegion);

        // Split the RAM into two regions
        g_physicalMMU.AddMemoryRegion(new StandardMemoryRegion(0, MIN(ramSize, 0xF000'0000)));
        if (ramSize > 0xF000'0000)
            g_physicalMMU.AddMemoryRegion(new StandardMemoryRegion(0x1'0000'0000, ramSize + 0x1000'0000));

        g_IOInterfaceManager = new IOInterfaceManager();

        // Configure the console device
        g_ConsoleDevice = new ConsoleDevice(16, consoleMode);
        g_IOBus->AddDevice(g_ConsoleDevice);
        g_IOInterfaceManager->AddInterfaceItem(g_ConsoleDevice);

        // Configure the debug interface
        if (debugConsoleMode != "disabled") {
            g_DebugInterface = new DebugInterface(IOInterfaceType::UNKNOWN, &g_physicalMMU, g_virtualMMU, debugConsoleMode);
            g_IOInterfaceManager->AddInterfaceItem(g_DebugInterface);
            g_DebugInterface->InterfaceInit();
        }

        // Configure the video device
        if (has_display) {
            g_VideoDevice = new VideoDevice(displayType, g_physicalMMU);
            assert(g_IOBus->AddDevice(g_VideoDevice));
        }

        // Configure the storage device
        if (has_drive) {
            g_StorageDevice = new StorageDevice(&g_physicalMMU, drivePath);
            g_StorageDevice->Initialise();
            assert(g_IOBus->AddDevice(g_StorageDevice));
        }

        // Configure the stack
        g_registers.SCP = new Register(RegisterType::Stack, 0, true);
        g_registers.SBP = new Register(RegisterType::Stack, 1, true);
        g_registers.STP = new Register(RegisterType::Stack, 2, true);

        g_stack = new Stack(&g_physicalMMU, *g_registers.SBP, *g_registers.STP, *g_registers.SCP);

        // Load program into RAM
        g_physicalMMU.WriteBuffer(0xF000'0000, program, size);

        g_registers.IP = new SafeRegister(RegisterType::Instruction, 0, false, 0xF000'0000); // explicitly initialise instruction pointer to start of BIOS region
        g_NextIP = 0;

        g_emulatorRunning = true;

        EmulatorMain();
        return 0;
    }

    void DumpRegisters(FILE* fp) {
        if (!g_registersInitialised)
            return;
        fprintf(fp, "Registers:\n");
        fprintf(fp, "R0 =%016lx R1 =%016lx R2 =%016lx R3 =%016lx\n", g_registers.GPR[0]->GetValue(), g_registers.GPR[1]->GetValue(), g_registers.GPR[2]->GetValue(), g_registers.GPR[3]->GetValue());
        fprintf(fp, "R4 =%016lx R5 =%016lx R6 =%016lx R7 =%016lx\n", g_registers.GPR[4]->GetValue(), g_registers.GPR[5]->GetValue(), g_registers.GPR[6]->GetValue(), g_registers.GPR[7]->GetValue());
        fprintf(fp, "R8 =%016lx R9 =%016lx R10=%016lx R11=%016lx\n", g_registers.GPR[8]->GetValue(), g_registers.GPR[9]->GetValue(), g_registers.GPR[10]->GetValue(), g_registers.GPR[11]->GetValue());
        fprintf(fp, "R12=%016lx R13=%016lx R14=%016lx R15=%016lx\n", g_registers.GPR[12]->GetValue(), g_registers.GPR[13]->GetValue(), g_registers.GPR[14]->GetValue(), g_registers.GPR[15]->GetValue());
        fprintf(fp, "SCP=%016lx SBP=%016lx STP=%016lx\n", g_registers.SCP->GetValue(), g_registers.SBP->GetValue(), g_registers.STP->GetValue());
        fprintf(fp, "IP =%016lx\n", g_registers.IP->GetValue());
        fprintf(fp, "CR0=%016lx CR1=%016lx CR2=%016lx CR3=%016lx\n", g_registers.Control[0]->GetValue(), g_registers.Control[1]->GetValue(), g_registers.Control[2]->GetValue(), g_registers.Control[3]->GetValue());
        fprintf(fp, "CR4=%016lx CR5=%016lx CR6=%016lx CR7=%016lx\n", g_registers.Control[4]->GetValue(), g_registers.Control[5]->GetValue(), g_registers.Control[6]->GetValue(), g_registers.Control[7]->GetValue());
        fprintf(fp, "STS = %016lx\n", g_registers.STS->GetValue());
    }

    void DumpRegisters(void (*write)(void*, const char*, ...), void* data) {
        if (!g_registersInitialised)
            return;
        if (write == nullptr)
            return DumpRegisters(stdout);

        write(data, "Registers:\n");
        write(data, "R0 =%016lx R1 =%016lx R2 =%016lx R3 =%016lx\n", g_registers.GPR[0]->GetValue(), g_registers.GPR[1]->GetValue(), g_registers.GPR[2]->GetValue(), g_registers.GPR[3]->GetValue());
        write(data, "R4 =%016lx R5 =%016lx R6 =%016lx R7 =%016lx\n", g_registers.GPR[4]->GetValue(), g_registers.GPR[5]->GetValue(), g_registers.GPR[6]->GetValue(), g_registers.GPR[7]->GetValue());
        write(data, "R8 =%016lx R9 =%016lx R10=%016lx R11=%016lx\n", g_registers.GPR[8]->GetValue(), g_registers.GPR[9]->GetValue(), g_registers.GPR[10]->GetValue(), g_registers.GPR[11]->GetValue());
        write(data, "R12=%016lx R13=%016lx R14=%016lx R15=%016lx\n", g_registers.GPR[12]->GetValue(), g_registers.GPR[13]->GetValue(), g_registers.GPR[14]->GetValue(), g_registers.GPR[15]->GetValue());
        write(data, "SCP=%016lx SBP=%016lx STP=%016lx\n", g_registers.SCP->GetValue(), g_registers.SBP->GetValue(), g_registers.STP->GetValue());
        write(data, "IP =%016lx\n", g_registers.IP->GetValue());
        write(data, "CR0=%016lx CR1=%016lx CR2=%016lx CR3=%016lx\n", g_registers.Control[0]->GetValue(), g_registers.Control[1]->GetValue(), g_registers.Control[2]->GetValue(), g_registers.Control[3]->GetValue());
        write(data, "CR4=%016lx CR5=%016lx CR6=%016lx CR7=%016lx\n", g_registers.Control[4]->GetValue(), g_registers.Control[5]->GetValue(), g_registers.Control[6]->GetValue(), g_registers.Control[7]->GetValue());
        write(data, "STS = %016lx\n", g_registers.STS->GetValue());
    }

    void DumpRAM(FILE* fp) {
        fprintf(fp, "RAM:\n");
        g_physicalMMU.DumpMemory(fp);
        fprintf(fp, "\n");
    }

    Register* GetRegisterPointer(uint8_t ID) {
        uint8_t type = (ID & 0xF0) >> 4;
        uint8_t index = ID & 0xF;
        Register* returnVal = nullptr;
        switch (type) {
        case 0: // GPR
            returnVal = g_registers.GPR[index];
            break;
        case 1: // stack
            switch (index) {
            case 0:
                returnVal = g_registers.SCP;
                break;
            case 1:
                returnVal = g_registers.SBP;
                break;
            case 2:
                returnVal = g_registers.STP;
                break;
            default:
                break;
            }
            break;
        case 2:
            if (index < 8)
                returnVal = g_registers.Control[index];
            else {
                index -= 8;
                switch (index) {
                case 0: // STS
                    returnVal = g_registers.STS;
                    break;
                case 1: // IP
                    returnVal = g_registers.IP;
                    break;
                default:
                    break;
                }
            }
            break;
        default:
            break;
        }
        return returnVal;
    }

    uint64_t ReadRegister(uint8_t ID) {
        Register* pointer = GetRegisterPointer(ID);
        return pointer->GetValue();
    }

    bool WriteRegister(uint8_t ID, uint64_t value) {
        Register* pointer = GetRegisterPointer(ID);
        if (ID == 0x28 || ID == 0x29 || ID == 0x2A)
            return false;
        pointer->SetValue(value);
        return true;
    }

    void EmulatorMain() {
        // Initialise all the registers
        for (int i = 0; i < 16; i++)
            g_registers.GPR[i] = new Register(RegisterType::GeneralPurpose, i, true);

        for (int i = 0; i < 8; i++)
            g_registers.Control[i] = new SafeSyncingRegister(RegisterType::Control, i, true);

        g_registers.STS = new SafeRegister(RegisterType::Status, 0, false);

        g_registersInitialised = true;

        SyncRegisters();

        InitInsCache(g_registers.IP->GetValue(), &g_physicalMMU);

        // setup instruction switch handling
        EmulatorThread = new std::thread(WaitForOperation);

        // setup instruction stuff
        g_instructionInProgress = false;

        // begin instruction loop.
        ExecutionThread = new std::thread(ExecutionLoop);

        // join with the emulator thread
        EmulatorThread->join();
    }

    void SetCPUStatus(uint64_t mask) {
        g_registers.STS->SetValue(g_registers.STS->GetValue() | mask, true);
    }

    void ClearCPUStatus(uint64_t mask) {
        g_registers.STS->SetValue(g_registers.STS->GetValue() & ~mask, true);
    }

    uint64_t GetCPUStatus() {
        return g_registers.STS->GetValue();
    }

    void SetNextIP(uint64_t value) {
        g_NextIP = value;
    }

    uint64_t GetNextIP() {
        return g_NextIP;
    }

    void SetCPU_IP(uint64_t value) {
        g_registers.IP->SetValue(value, true);
    }

    uint64_t GetCPU_IP() {
        return g_registers.IP->GetValue();
    }

    [[noreturn]] void JumpToIP(uint64_t value) {
        RaiseEvent({EventType::SwitchToIP, value});
        EmulatorThread->join();
        Crash("Emulator thread exited unexpectedly"); // should be unreachable
    }

    void JumpToIPExternal(uint64_t value) {
        SetCPU_IP(value);
        InsCache_MaybeSetBaseAddress(value);
        ExecutionThread = new std::thread(ExecutionLoop);
    }

    void SyncRegisters() {
        if (g_registers.Control[0]->IsDirty()) {
            uint64_t control = g_registers.Control[0]->GetValue();
            bool wasInProtectedMode = g_isInProtectedMode;
            g_isInProtectedMode = control & 1;
            if (((control & 2) > 0) != g_isPagingEnabled) {
                g_isPagingEnabled = (control & 2) > 0;
                if (g_isPagingEnabled) {
                    PageSize pageSize = static_cast<PageSize>((control & 0xC) >> 2);
                    PageTableLevelCount pageTableLevelCount = static_cast<PageTableLevelCount>((control & 0x30) >> 4);
                    if (pageSize == PS_64KiB && pageTableLevelCount == PTLC_5) {
                        // restore any changes
                        if (!wasInProtectedMode && g_isInProtectedMode)
                            g_isInProtectedMode = false;
                        g_isPagingEnabled = false;
                        g_registers.Control[0]->SetDirty(false);
                        g_ExceptionHandler->RaiseException(Exception::INVALID_INSTRUCTION);
                    }
                    uint64_t pageTableRoot = g_registers.Control[3]->GetValue();
                    g_registers.Control[3]->SetDirty(false);
                    g_virtualMMU = new VirtualMMU(&g_physicalMMU, pageTableRoot, pageSize, pageTableLevelCount);
                    g_CurrentMMU = g_virtualMMU;
                } else {
                    g_CurrentMMU = &g_physicalMMU;
                    delete g_virtualMMU;
                }
                g_registers.Control[0]->SetDirty(false);
                g_registers.IP->SetValue(g_NextIP);
                RaiseEvent({EventType::NewMMU, 0});
                EmulatorThread->join();
                Crash("Emulator thread exited unexpectedly"); // should be unreachable
            }
            g_registers.Control[0]->SetDirty(false);
        }
        if (g_registers.Control[3]->IsDirty() && g_isPagingEnabled) {
            uint64_t pageTableRoot = g_registers.Control[3]->GetValue();
            g_registers.Control[3]->SetDirty(false);
            g_virtualMMU->SetPageTableRoot(pageTableRoot);
        }
    }

    [[noreturn]] void Crash(const char* message) {
        g_emulatorRunning = false;
        printf("Crash: %s\n", message);
        DumpRegisters(stdout);
        // DumpRAM(stderr);
        exit(0);
    }

    void HandleHalt() {
        // DumpRAM(stdout);
        // DumpRegisters(stdout);
        g_emulatorRunning = false;
        exit(0);
    }

    bool isInProtectedMode() {
        return g_isInProtectedMode;
    }

    bool isInUserMode() {
        return g_isInUserMode;
    }

    void EnterUserMode() {
        uint64_t status = g_registers.STS->GetValue();
        g_registers.STS->SetValue(g_registers.Control[1]->GetValue(), true);
        g_registers.Control[1]->SetValue(status, true);
        g_NextIP = g_registers.GPR[14]->GetValue();
        g_registers.SCP->SetValue(g_registers.GPR[15]->GetValue());
        g_isInUserMode = true;
    }

    void EnterUserMode(uint64_t address) {
        g_registers.STS->SetValue(0, true);
        g_NextIP = address;
        g_isInUserMode = true;
    }

    void ExitUserMode() {
        g_isInUserMode = false;
        uint64_t status = g_registers.STS->GetValue();
        g_registers.STS->SetValue(g_registers.Control[1]->GetValue(), true);
        g_registers.Control[1]->SetValue(status, true);
        g_registers.GPR[14]->SetValue(GetNextIP(), true);
        g_NextIP = g_registers.Control[2]->GetValue();
        g_registers.GPR[15]->SetValue(g_registers.SCP->GetValue());
    }

    void KillCurrentInstruction() {
        if (std::this_thread::get_id() == ExecutionThread->get_id())
            Crash("Cannot kill current instruction from the instruction thread");

        void* state = nullptr;

        StopExecution(&state); // wait for current instruction to finish executing

        ExecutionThread->join(); // ensure the thread has finished executing
        delete ExecutionThread;

        AllowExecution(&state);
    }

    bool isPagingEnabled() {
        return g_isPagingEnabled;
    }

    DebugInterface* GetDebugInterface() {
        return g_DebugInterface;
    }

} // namespace Emulator
