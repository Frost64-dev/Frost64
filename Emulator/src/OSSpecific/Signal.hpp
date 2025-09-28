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

#ifndef _OSSPECIFIC_SIGNAL_HPP
#define _OSSPECIFIC_SIGNAL_HPP

typedef void (*SignalHandler_t)(int);

#ifdef __unix__
#include <csignal>

#include <sys/types.h>

#define USER_SIGNAL_1 SIGUSR1
#define USER_SIGNAL_2 SIGUSR2

typedef pthread_t ThreadID_t;
#else
#define USER_SIGNAL_1 0
#define USER_SIGNAL_2 0
#endif /* __unix__ */

void SetSignalHandler(int signal, SignalHandler_t handler);
void SendSignal(int signal, ThreadID_t thread_id);

// Callback will be called when a signal is received, with ctx as its argument, after internal handling has been attempted
void ConfigureEmulatorSignalHandlers(void (*callback)(void* ctx), void* ctx);

void GlobalSignalHandler(int signal);

#endif /* _OSSPECIFIC_SIGNAL_HPP */