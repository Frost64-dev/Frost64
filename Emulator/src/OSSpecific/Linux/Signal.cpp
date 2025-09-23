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

#include "Platform/Signal.hpp"

#include <csignal>
#include <cstring>
#include <sstream>

#include <Emulator.hpp>

void (*g_signalCallback)(void* ctx) = nullptr;
void* g_signalCallbackCtx = nullptr;

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

void Linux_GeneralSignalHandler(int signal, siginfo_t* info, void* ctx) {
    Linux_InstructionSignalHandler(signal, info, ctx);
    if (g_signalCallback != nullptr)
        g_signalCallback(g_signalCallbackCtx);
    std::stringstream ss = std::stringstream();
    ss << "Unhandled signal " << signal;
    Emulator::Crash(ss.str().c_str());
}

void ConfigureEmulatorSignalHandlers(void (*callback)(void* ctx), void* ctx) {
    g_signalCallback = callback;
    g_signalCallbackCtx = ctx;
    struct sigaction sa = {};
    sa.sa_sigaction = Linux_GeneralSignalHandler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
#define ConfigureSignal(signum)\
    rc = sigaction(signum, &sa, nullptr);\
    if (rc < 0) {\
        std::stringstream ss = std::stringstream();\
        ss << "Failed to set signal handler for signal " << signum << " with error: " << strerror(errno);\
        Emulator::Crash(ss.str().c_str());\
    }
    int rc = 0;
    // don't handle abort (SIGABRT) or alarm (SIGALRM) as these are used internally by the C library
    ConfigureSignal(SIGBUS);
    ConfigureSignal(SIGFPE);
    ConfigureSignal(SIGILL);
    // need to check if we should configure SIGINT, as the debug interface may want to handle it
    if (Emulator::GetDebugInterface() == nullptr) {
        ConfigureSignal(SIGINT);
    }
    ConfigureSignal(SIGPIPE);
    ConfigureSignal(SIGQUIT);
    ConfigureSignal(SIGSEGV);
    ConfigureSignal(SIGSYS);
    ConfigureSignal(SIGTERM);
}

void GlobalSignalHandler(int signal) {
    // can't do full handling as we don't have siginfo_t
    if (g_signalCallback != nullptr)
        g_signalCallback(g_signalCallbackCtx);
    std::stringstream ss = std::stringstream();
    ss << "Unhandled signal " << signal;
    Emulator::Crash(ss.str().c_str());
}
