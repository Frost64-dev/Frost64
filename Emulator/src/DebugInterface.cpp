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

#include "DebugInterface.hpp"

#include <cctype>
#include <cstdarg>
#include <cstring>
#include <functional>
#include <iterator>
#include <unordered_map>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <Emulator.hpp>

#include <Instruction/Instruction.hpp>

#include <IO/IOInterfaceManager.hpp>

#include <MMU/MMU.hpp>
#include <MMU/VirtualMMU.hpp>

#include <OSSpecific/Signal.hpp>

#include <Common/Util.hpp>

DebugInterface::DebugInterface(IOInterfaceType type, MMU* physicalMMU, VirtualMMU* virtualMMU, const std::string_view& data) : IOInterfaceItem(type, data), m_physicalMMU(physicalMMU), m_virtualMMU(virtualMMU), m_thread(nullptr), m_waitLock(0), m_eventPending(0), m_handlingEvents(0) {
}

DebugInterface::~DebugInterface() {
}

void DebugInterface::InterfaceInit() {
    spinlock_init(&m_waitLock);

    // build the command map
    m_commands["help"] = [this](auto&& PH1) {
        return Command_Help(std::forward<decltype(PH1)>(PH1));
    };
    m_commands["quit"] = [this](auto&& PH1) {
        return Command_Quit(std::forward<decltype(PH1)>(PH1));
    };
    m_commands["pause"] = [this](auto&& PH1) {
        return Command_Pause(std::forward<decltype(PH1)>(PH1));
    };
    m_commands["continue"] = [this](auto&& PH1) {
        return Command_Continue(std::forward<decltype(PH1)>(PH1));
    };
    m_commands["step"] = [this](auto&& PH1) {
        return Command_Step(std::forward<decltype(PH1)>(PH1));
    };
    m_commands["breakpoint"] = [this](auto&& PH1) {
        return Command_Breakpoint(std::forward<decltype(PH1)>(PH1));
    };
    m_commands["delete"] = [this](auto&& PH1) {
        return Command_Delete(std::forward<decltype(PH1)>(PH1));
    };
    m_commands["info"] = [this](auto&& PH1) {
        return Command_Info(std::forward<decltype(PH1)>(PH1));
    };
    m_commands["dump"] = [this](auto&& PH1) {
        return Command_Dump(std::forward<decltype(PH1)>(PH1));
    };

    // build the command alias map
    m_commandAliases["help"] = "help";
    m_commandAliases["h"] = "help";
    m_commandAliases["?"] = "help";

    m_commandAliases["quit"] = "quit";
    m_commandAliases["q"] = "quit";
    m_commandAliases["exit"] = "quit";

    m_commandAliases["pause"] = "pause";
    m_commandAliases["p"] = "pause";

    m_commandAliases["continue"] = "continue";
    m_commandAliases["c"] = "continue";

    m_commandAliases["step"] = "step";
    m_commandAliases["s"] = "step";

    m_commandAliases["breakpoint"] = "breakpoint";
    m_commandAliases["b"] = "breakpoint";

    m_commandAliases["delete"] = "delete";
    m_commandAliases["d"] = "delete";

    m_commandAliases["info"] = "info";
    m_commandAliases["i"] = "info";

    m_commandAliases["dump"] = "dump";
    m_commandAliases["dmp"] = "dump";

    // build the command help map
    m_commandHelp["help"] = "display this help message";
    m_commandHelp["quit"] = "quit the emulator";
    m_commandHelp["pause"] = "pause the emulator";
    m_commandHelp["continue"] = "unpause the emulator";
    m_commandHelp["step"] = "execute one instruction";
    m_commandHelp["breakpoint"] = "set a breakpoint";
    m_commandHelp["delete"] = "delete a breakpoint";
    m_commandHelp["info"] = "display information about the emulator";
    m_commandHelp["dump"] = "dump portions of physical or virtual memory";


    spinlock_acquire(&m_waitLock);

    m_thread = new std::thread(&DebugInterface::MainLoop, this);

    // wait for the thread to start
    spinlock_acquire(&m_waitLock);
}

void DebugInterface::InterfaceShutdown() {
}

void DebugInterface::InterfaceWrite() {
}

void DebugInterface::RaiseEvent(EventType type, void* data) {
    Event* event = new Event();
    event->type = type;
    event->data = data;
    m_eventQueue.lock();
    m_eventQueue.insert(event);
    m_eventQueue.unlock();
    m_eventPending.store(1);
    m_eventPending.notify_all();
}

void DebugInterface::MainLoop() {
    SetSignalHandler(SIGINT, [](int signal) {
        DebugInterface* debugInterface = Emulator::GetDebugInterface();
        if (debugInterface != nullptr) {
            if (debugInterface->m_handlingEvents.load() == 1)
                debugInterface->RaiseEvent(EventType::Signal, reinterpret_cast<void*>(static_cast<uint64_t>(signal)));
            else
                Emulator::Crash("SIGINT received");
        }
    });

    bool first = true;
    bool result = true;
    
    while (true) {
        while (result) {
            if (first) {
                // Pause the execution thread
                PauseExecution();
                g_IOInterfaceManager->Write(this, "Emulator paused\n");
            }

            // 1. print prompt
            g_IOInterfaceManager->Write(this, "debug > ");

            if (first) { // release the lock after printing the prompt
                spinlock_release(&m_waitLock);
                first = false;
            }

            // 2. read input
            std::stringstream input;
            char c = 0;
            do {
                g_IOInterfaceManager->Read(this, &c, 1);
                if (isprint(c)) // ignore non-printable characters
                    input << c;
            } while (c != '\n');

            // 3. parse input
            // need to break the input into tokens, each separated by a space
            // the first token is the command, the rest are arguments
            std::vector<std::string> tokens(std::istream_iterator<std::string>{input}, std::istream_iterator<std::string>());

            if (tokens.empty())
                continue;

            // get the real command
            auto it = m_commandAliases.find(tokens[0]);
            if (it == m_commandAliases.end()) {
                g_IOInterfaceManager->Write(this, "Unknown command\n");
                continue;
            }

            // get the command function
            auto it2 = m_commands.find(it->second);
            if (it2 == m_commands.end()) {
                g_IOInterfaceManager->Write(this, "Unknown command\n");
                continue;
            }

            // execute the command
            result = it2->second(std::vector<std::string_view>(tokens.begin() + 1, tokens.end()));
        }

        if (!result) {
            m_handlingEvents.store(1);
            m_eventPending.wait(0);
        }

        bool hasInterruptingEvent = false;

        m_eventQueue.lock();
        m_eventQueue.Enumerate([&](Event* event) {
            switch (event->type) {
            case EventType::Breakpoint:
                HandleBreakpoint((uint64_t)event->data);
                hasInterruptingEvent = true;
                break;
            case EventType::Signal:
                // handle signal
                if (reinterpret_cast<uint64_t>(event->data) == SIGINT)
                    g_IOInterfaceManager->Write(this, "SIGINT received\n");
                else {
                    g_IOInterfaceManager->WriteFormatted(this, "Unhandled signal %lu received\n", reinterpret_cast<uint64_t>(event->data));
                    Emulator::Crash("Unhandled signal received");
                }
                hasInterruptingEvent = true;
                break;
            }
            delete event;
        });
        m_eventQueue.clear();
        m_eventPending.store(0);
        m_eventQueue.unlock();

        if (hasInterruptingEvent) {
            // if we had an interrupting event, we need to pause the execution thread and allow for commands to be entered
            PauseExecution();
            g_IOInterfaceManager->Write(this, "Emulator paused\n");
            result = true;
            m_handlingEvents.store(0);
        }
    }
}

void DebugInterface::HandleBreakpoint(uint64_t address) {
    g_IOInterfaceManager->WriteFormatted(this, "Breakpoint hit at 0x%lx\n", address);
}

bool DebugInterface::Command_Help(const std::vector<std::string_view>& args) {
    if (!args.empty()) {
        // arg 0 is the command name
        // get the real command
        auto it = m_commandAliases.find(args[0]);
        if (it == m_commandAliases.end()) {
            g_IOInterfaceManager->Write(this, "Unknown command\n");
            return true;
        }

        // get the help message
        auto it2 = m_commandHelp.find(it->second);
        if (it2 == m_commandHelp.end()) {
            g_IOInterfaceManager->Write(this, "Unknown command\n");
            return true;
        }

        g_IOInterfaceManager->Write(this, it2->first.data());
        g_IOInterfaceManager->Write(this, " - ");
        g_IOInterfaceManager->Write(this, it2->second.data());
        g_IOInterfaceManager->Write(this, "\n");

        return true;
    }

    for (const auto& [command, help] : m_commandHelp) {
        g_IOInterfaceManager->Write(this, command.data());
        g_IOInterfaceManager->Write(this, " - ");
        g_IOInterfaceManager->Write(this, help.data());
        g_IOInterfaceManager->Write(this, "\n");
    }

    return true;
}

bool DebugInterface::Command_Quit(const std::vector<std::string_view>&) {
    g_IOInterfaceManager->Write(this, "Quit\n");
    Emulator::Crash("User requested quit");
}

bool DebugInterface::Command_Pause(const std::vector<std::string_view>&) {
    g_IOInterfaceManager->Write(this, "Paused\n");
    PauseExecution();
    return true;
}

bool DebugInterface::Command_Continue(const std::vector<std::string_view>&) {
    g_IOInterfaceManager->Write(this, "Continuing...\n");
    AllowExecution();
    return false;
}

bool DebugInterface::Command_Step(const std::vector<std::string_view>&) {
    g_IOInterfaceManager->Write(this, "Stepping...\n");
    AllowOneInstruction();
    uint64_t IP = Emulator::GetNextIP();
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Next IP: 0x%lx\n", IP);
    g_IOInterfaceManager->Write(this, buffer);
    return true;
}

bool DebugInterface::Command_Breakpoint(const std::vector<std::string_view>& args) {
    if (args.empty()) {
        g_IOInterfaceManager->Write(this, "Usage: breakpoint <address>\n");
        return true;
    }

    uint64_t address = strtoul(args[0].data(), nullptr, 0);
    g_IOInterfaceManager->WriteFormatted(this, "Setting breakpoint at %lu\n", address);
    AddBreakpoint(address, [this](uint64_t addr) {
        RaiseEvent(EventType::Breakpoint, (void*)addr);
    });
    return true;
}

bool DebugInterface::Command_Delete(const std::vector<std::string_view>& args) {
    if (args.empty()) {
        g_IOInterfaceManager->Write(this, "Usage: delete <address>\n");
        return true;
    }

    uint64_t address = strtoul(args[0].data(), nullptr, 0);
    g_IOInterfaceManager->WriteFormatted(this, "Deleting breakpoint at %lu\n", address);
    RemoveBreakpoint(address);
    return true;
}

void DI_WriteHandler(void* data, const char* format, ...) {
    va_list args;
    va_start(args, format);
    g_IOInterfaceManager->WritevFormatted(static_cast<DebugInterface*>(data), format, args);
    va_end(args);
}

bool DebugInterface::Command_Info(const std::vector<std::string_view>& args) {
    if (args.empty()) {
        g_IOInterfaceManager->Write(this, "Usage: info <command>\n");
        g_IOInterfaceManager->Write(this, "Available commands: registers, memory\n");
        return true;
    }

    const std::string_view& command = args[0];
    if (command == "registers")
        Emulator::DumpRegisters(DI_WriteHandler, this);
    else if (command == "memory")
        m_physicalMMU->PrintRegions(DI_WriteHandler, this);
    else
        g_IOInterfaceManager->Write(this, "Unknown command\n");

    return true;
}

bool DebugInterface::Command_Dump(const std::vector<std::string_view>& args) {
    if (args.size() < 2) {
        g_IOInterfaceManager->Write(this, "Usage: dump [phys|virt] <address> <size>\n");
        return true;
    }

    bool phys = true;
    bool explicit_request = false;

    if (args[0] == "virt") {
        phys = false;
        explicit_request = true;
    }
    else if (args[0] == "phys")
        explicit_request = true;

    uint64_t address = strtoul(args[explicit_request ? 1 : 0].data(), nullptr, 0);
    uint64_t size = strtoul(args[explicit_request ? 2 : 1].data(), nullptr, 0);
    uint64_t end = address + size;

    MMU* mmu = phys ? m_physicalMMU : m_virtualMMU;

    if (!mmu->ValidateRead(address, size)) {
        g_IOInterfaceManager->Write(this, "Invalid region\n");
        return true;
    }

    g_IOInterfaceManager->WriteFormatted(this, "Dumping %s memory from 0x%lx to 0x%lx\n", phys ? "physical" : "virtual", address, end);

    uint8_t buffer[16];
    uint8_t buffer_index = 0;
    uint8_t last_printed = 0;
    for (uint64_t i = address; i < end; i++, buffer_index++) {
        if ((i - address) % 16 == 0 && i != address) {
            buffer_index = 0;
            if (i > 16 && end - i > 16) {
                bool result = false;
                CMP16_B(buffer, last_printed, result);
                if (result)
                    continue; // skip printing this line
            }
            g_IOInterfaceManager->WriteFormatted(this, "%016lx: ", i - 16);
            for (uint64_t j = 0; j < 16; j++) {
                if (j == 8)
                    g_IOInterfaceManager->WriteFormatted(this, " ");
                g_IOInterfaceManager->WriteFormatted(this, "%02x ", buffer[j]);
            }
            g_IOInterfaceManager->WriteFormatted(this, " |");
            for (uint8_t c : buffer)
                g_IOInterfaceManager->WriteFormatted(this, "%c", c >= 32 && c <= 126 ? c : '.');
            last_printed = buffer[15];
            g_IOInterfaceManager->WriteFormatted(this, "|\n");
            buffer_index = 0;
        }
        buffer[buffer_index] = mmu->read8(i);
    }
    
    // print any remaining bytes
    if (buffer_index > 0) {
        g_IOInterfaceManager->WriteFormatted(this, "%016lx: ", end - buffer_index);
        for (uint64_t j = 0; j < buffer_index; j++) {
            if (j == 8)
                g_IOInterfaceManager->WriteFormatted(this, " ");
            g_IOInterfaceManager->WriteFormatted(this, "%02x ", buffer[j]);
        }
        for (uint64_t j = buffer_index; j < 16; j++) {
            if (j == 8)
                g_IOInterfaceManager->WriteFormatted(this, " ");
            g_IOInterfaceManager->WriteFormatted(this, "   ");
        }
        g_IOInterfaceManager->WriteFormatted(this, " |");
        for (uint64_t j = 0; j < buffer_index; j++)
            g_IOInterfaceManager->WriteFormatted(this, "%c", buffer[j] >= 32 && buffer[j] <= 126 ? buffer[j] : '.');
        for (uint64_t j = buffer_index; j < 16; j++)
            g_IOInterfaceManager->WriteFormatted(this, " ");
        g_IOInterfaceManager->WriteFormatted(this, "|\n");
    }

    return true;
}
