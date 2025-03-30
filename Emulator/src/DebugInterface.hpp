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

#ifndef _DEBUG_INTERFACE_HPP
#define _DEBUG_INTERFACE_HPP

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <string_view>
#include <thread>
#include <vector>

#include <common/spinlock.h>

#include <common/Data-structures/LinkedList.hpp>

#include <IO/IOInterfaceItem.hpp>

#include <MMU/MMU.hpp>
#include <MMU/VirtualMMU.hpp>

class DebugInterface : public IOInterfaceItem {
public:
    enum class EventType {
        Breakpoint,
        Signal,
    };

    DebugInterface(IOInterfaceType type, MMU* physicalMMU, VirtualMMU* virtualMMU, const std::string_view& data = "");
    ~DebugInterface();

    void InterfaceInit() override;
    void InterfaceShutdown() override;

    // handle a write operation
    void InterfaceWrite() override;

    void RaiseEvent(EventType type, void* data);

private:
    void MainLoop();
    void HandleBreakpoint(uint64_t address);

#define COMMAND(name) bool Command_##name(const std::vector<std::string_view>&)

    COMMAND(Help);
    COMMAND(Quit);
    COMMAND(Pause);
    COMMAND(Continue);
    COMMAND(Step);
    COMMAND(Breakpoint);
    COMMAND(Delete);
    COMMAND(Info);
    COMMAND(Dump);

#undef COMMAND

private:
    struct Event {
        EventType type;
        void* data;
    };

    MMU* m_physicalMMU;
    VirtualMMU* m_virtualMMU;

    std::thread* m_thread;
    spinlock_t m_waitLock;

    std::unordered_map<std::string_view, std::function<bool (const std::vector<std::string_view>&)>> m_commands; // return value of command determines whether to continue reading commands, or enter event loop
    std::unordered_map<std::string_view, std::string_view> m_commandAliases;
    std::unordered_map<std::string_view, std::string_view> m_commandHelp;

    LinkedList::RearInsertLinkedList<Event> m_eventQueue;
    std::atomic_uchar m_eventPending;
    std::atomic_uchar m_handlingEvents = 0;
};

#endif /* _DEBUG_INTERFACE_HPP */