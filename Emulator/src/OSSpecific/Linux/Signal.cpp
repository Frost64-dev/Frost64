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
#include <cstring>
#include <sstream>

#include <Emulator.hpp>

void SetSignalHandler(int signal, SignalHandler_t handler) {
    struct sigaction sa{};
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    if (sigaction(signal, &sa, nullptr) == -1) {
        std::stringstream ss = std::stringstream();
        ss << "Failed to set signal handler for signal " << signal << " with error: " << strerror(errno);
        Emulator::Crash(ss.str().c_str());
    }
}

void SendSignal(int signal, ThreadID_t thread_id) {
    if (pthread_kill(thread_id, signal) != 0) {
        std::stringstream ss = std::stringstream();
        ss << "Failed to send signal " << signal << " to thread " << thread_id << " with error: " << strerror(errno);
        Emulator::Crash(ss.str().c_str());
    }
}