/*
Copyright (©) 2025-2026  Frosty515

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

#ifndef _IO_KEYBOARD_HPP
#define _IO_KEYBOARD_HPP

#include <cstdint>

#include "InputDeviceBus.hpp"

typedef uint8_t Keycode;

struct KeyboardModifiers {
    uint8_t L_CTRL  : 1;
    uint8_t R_CTRL  : 1;
    uint8_t L_SHIFT : 1;
    uint8_t R_SHIFT : 1;
    uint8_t L_ALT   : 1;
    uint8_t R_ALT   : 1;
    uint8_t SUPER   : 1;
    uint8_t MENU    : 1;
};

struct [[gnu::packed]] KeyboardEvent {
    Keycode keycode;
    KeyboardModifiers Modifiers;
    uint8_t Released : 1;
    uint64_t Reserved : 47;
};

class KeyboardBackend;

class Keyboard {
public:
    Keyboard(InputDeviceBus* bus, VideoBackend* videoBackend);
    ~Keyboard();

    void Init();

    void HandleKeyEvent(Keycode keycode, bool release);

    uint64_t Read();

private:
    InputDeviceBus* m_bus;
    VideoBackend* m_videoBackend;

    KeyboardBackend* m_backend;

    KeyboardModifiers m_modifiers;

    KeyboardEvent m_currentEvent;
    bool m_dataRead;
};

enum class KeyboardBackendType {
    XCB = 0,
};

class KeyboardBackend {
public:
    KeyboardBackend() : m_keyboard(nullptr) {}
    virtual ~KeyboardBackend() {}

    virtual void Initialise() = 0;
    virtual void Shutdown() = 0;

    void SetKeyboard(Keyboard* keyboard) { m_keyboard = keyboard; }

    Keyboard* GetKeyboard() { return m_keyboard; }

private:
    Keyboard* m_keyboard;
};

#endif /* _IO_KEYBOARD_HPP */