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

#ifndef _IO_HID_KEYBOARD_HPP
#define _IO_HID_KEYBOARD_HPP

#include <cstdint>

#include "HIDDeviceBus.hpp"

typedef uint8_t HIDKeycode;

struct HIDKeyboardModifiers {
    uint8_t L_CTRL  : 1;
    uint8_t R_CTRL  : 1;
    uint8_t L_SHIFT : 1;
    uint8_t R_SHIFT : 1;
    uint8_t L_ALT   : 1;
    uint8_t R_ALT   : 1;
    uint8_t SUPER   : 1;
    uint8_t MENU    : 1;
};

struct [[gnu::packed]] HIDKeyboardEvent {
    HIDKeycode Keycode;
    HIDKeyboardModifiers Modifiers;
    uint8_t Released : 1;
    uint64_t Reserved : 47;
};

class HIDKeyboardBackend;

class HIDKeyboard {
public:
    HIDKeyboard(HIDDeviceBus* bus, VideoBackend* videoBackend);
    ~HIDKeyboard();

    void Init();

    void HandleKeyEvent(HIDKeycode keycode, bool release);

    uint64_t Read();

private:
    HIDDeviceBus* m_bus;
    VideoBackend* m_videoBackend;

    HIDKeyboardBackend* m_backend;

    HIDKeyboardModifiers m_modifiers;

    HIDKeyboardEvent m_currentEvent;
    bool m_dataRead;
};

enum class HIDKeyboardBackendType {
    XCB = 0,
};

class HIDKeyboardBackend {
public:
    HIDKeyboardBackend() : m_keyboard(nullptr) {}
    virtual ~HIDKeyboardBackend() {}

    virtual void Initialise() = 0;
    virtual void Shutdown() = 0;

    void SetKeyboard(HIDKeyboard* keyboard) { m_keyboard = keyboard; }

    HIDKeyboard* GetKeyboard() { return m_keyboard; }

private:
    HIDKeyboard* m_keyboard;
};

#endif /* _IO_HID_KEYBOARD_HPP */