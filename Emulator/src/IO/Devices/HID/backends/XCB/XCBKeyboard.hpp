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

#ifndef _XCB_KEYBOARD_BACKEND_HPP
#define _XCB_KEYBOARD_BACKEND_HPP

#include <cstdint>

#include <xcb/xcb.h>

#include <xkbcommon/xkbcommon.h>

#include <IO/Devices/HID/Keyboard.hpp>

class XCBKeyboardBackend : public HIDKeyboardBackend {
public:
    explicit XCBKeyboardBackend(xcb_connection_t* connection);
    ~XCBKeyboardBackend() override;

    void Initialise() override;
    void Shutdown() override;

    void HandleKeyEvent(xcb_key_press_event_t* event, bool release);

private:

    // XCB & XKB stuff
    xcb_connection_t* m_connection;
    xkb_context* m_xkbContext;
    int m_xkbDeviceID;
    xkb_state* m_xkbState;
    xkb_keymap* m_xkbKeymap;
};

#endif /* _XCB_KEYBOARD_BACKEND_HPP */