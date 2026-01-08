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

#include "XCBKeyboard.hpp"

#include <Emulator.hpp>

#include <xcb/xcb.h>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-x11.h>

XCBKeyboardBackend::XCBKeyboardBackend(xcb_connection_t* connection)
    : m_connection(connection), m_xkbContext(nullptr), m_xkbDeviceID(0), m_xkbState(nullptr), m_xkbKeymap(nullptr) {
}

XCBKeyboardBackend::~XCBKeyboardBackend() {

}

void XCBKeyboardBackend::Initialise() {
    int rc = xkb_x11_setup_xkb_extension(
        m_connection,
        XKB_X11_MIN_MAJOR_XKB_VERSION,
        XKB_X11_MIN_MINOR_XKB_VERSION,
        XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
        nullptr,
        nullptr,
        nullptr,
        nullptr);
    if (rc != 1)
        Emulator::Crash("Failed to setup XKB extension via XCB");

    m_xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (m_xkbContext == nullptr)
        Emulator::Crash("Failed to create XKB context via XCB");

    m_xkbDeviceID = xkb_x11_get_core_keyboard_device_id(m_connection);
    if (m_xkbDeviceID == -1)
        Emulator::Crash("Failed to get core keyboard device ID via XCB");

    m_xkbKeymap = xkb_x11_keymap_new_from_device(m_xkbContext, m_connection, m_xkbDeviceID, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (m_xkbKeymap == nullptr)
        Emulator::Crash("Failed to create XKB keymap via XCB");

    m_xkbState = xkb_x11_state_new_from_device(m_xkbKeymap, m_connection, m_xkbDeviceID);
    if (m_xkbState == nullptr)
        Emulator::Crash("Failed to create XKB state via XCB");


}

void XCBKeyboardBackend::Shutdown() {
    xkb_state_unref(m_xkbState);
    xkb_keymap_unref(m_xkbKeymap);
    xkb_context_unref(m_xkbContext);
}

void XCBKeyboardBackend::HandleKeyEvent(xcb_key_press_event_t* event, bool release) {
    HIDKeycode keycode;
    xkb_keysym_t keysym = xkb_state_key_get_one_sym(m_xkbState, event->detail);
    switch (keysym) {
#define CASE_KEYSYM(sym, code) \
    case sym:                    \
        keycode = code;         \
        break;
    CASE_KEYSYM(XKB_KEY_Escape, 0)
    CASE_KEYSYM(XKB_KEY_F1, 1)
    CASE_KEYSYM(XKB_KEY_F2, 2)
    CASE_KEYSYM(XKB_KEY_F3, 3)
    CASE_KEYSYM(XKB_KEY_F4, 4)
    CASE_KEYSYM(XKB_KEY_F5, 5)
    CASE_KEYSYM(XKB_KEY_F6, 6)
    CASE_KEYSYM(XKB_KEY_F7, 7)
    CASE_KEYSYM(XKB_KEY_F8, 8)
    CASE_KEYSYM(XKB_KEY_F9, 9)
    CASE_KEYSYM(XKB_KEY_F10, 10)
    CASE_KEYSYM(XKB_KEY_F11, 11)
    CASE_KEYSYM(XKB_KEY_F12, 12)
    CASE_KEYSYM(XKB_KEY_Print, 13)
    CASE_KEYSYM(XKB_KEY_Scroll_Lock, 14)
    CASE_KEYSYM(XKB_KEY_Pause, 15)
    CASE_KEYSYM(XKB_KEY_grave, 16)
    CASE_KEYSYM(XKB_KEY_1, 17)
    CASE_KEYSYM(XKB_KEY_2, 18)
    CASE_KEYSYM(XKB_KEY_3, 19)
    CASE_KEYSYM(XKB_KEY_4, 20)
    CASE_KEYSYM(XKB_KEY_5, 21)
    CASE_KEYSYM(XKB_KEY_6, 22)
    CASE_KEYSYM(XKB_KEY_7, 23)
    CASE_KEYSYM(XKB_KEY_8, 24)
    CASE_KEYSYM(XKB_KEY_9, 25)
    CASE_KEYSYM(XKB_KEY_0, 26)
    CASE_KEYSYM(XKB_KEY_minus, 27)
    CASE_KEYSYM(XKB_KEY_equal, 28)
    CASE_KEYSYM(XKB_KEY_BackSpace, 29)
    CASE_KEYSYM(XKB_KEY_Insert, 30)
    CASE_KEYSYM(XKB_KEY_Home, 31)
    CASE_KEYSYM(XKB_KEY_Page_Up, 32)
    CASE_KEYSYM(XKB_KEY_Num_Lock, 33)
    CASE_KEYSYM(XKB_KEY_KP_Divide, 34)
    CASE_KEYSYM(XKB_KEY_KP_Multiply, 35)
    CASE_KEYSYM(XKB_KEY_KP_Subtract, 36)
    CASE_KEYSYM(XKB_KEY_Tab, 37)
    CASE_KEYSYM(XKB_KEY_q, 38)
    CASE_KEYSYM(XKB_KEY_w, 39)
    CASE_KEYSYM(XKB_KEY_e, 40)
    CASE_KEYSYM(XKB_KEY_r, 41)
    CASE_KEYSYM(XKB_KEY_t, 42)
    CASE_KEYSYM(XKB_KEY_y, 43)
    CASE_KEYSYM(XKB_KEY_u, 44)
    CASE_KEYSYM(XKB_KEY_i, 45)
    CASE_KEYSYM(XKB_KEY_o, 46)
    CASE_KEYSYM(XKB_KEY_p, 47)
    CASE_KEYSYM(XKB_KEY_bracketleft, 48)
    CASE_KEYSYM(XKB_KEY_bracketright, 49)
    CASE_KEYSYM(XKB_KEY_backslash, 50)
    CASE_KEYSYM(XKB_KEY_Delete, 51)
    CASE_KEYSYM(XKB_KEY_End, 52)
    CASE_KEYSYM(XKB_KEY_Page_Down, 53)
    CASE_KEYSYM(XKB_KEY_KP_7, 54)
    CASE_KEYSYM(XKB_KEY_KP_Home, 54)
    CASE_KEYSYM(XKB_KEY_KP_8, 55)
    CASE_KEYSYM(XKB_KEY_KP_Up, 55)
    CASE_KEYSYM(XKB_KEY_KP_9, 56)
    CASE_KEYSYM(XKB_KEY_KP_Prior, 56)
    CASE_KEYSYM(XKB_KEY_KP_Add, 57)
    CASE_KEYSYM(XKB_KEY_Caps_Lock, 58)
    CASE_KEYSYM(XKB_KEY_a, 59)
    CASE_KEYSYM(XKB_KEY_s, 60)
    CASE_KEYSYM(XKB_KEY_d, 61)
    CASE_KEYSYM(XKB_KEY_f, 62)
    CASE_KEYSYM(XKB_KEY_g, 63)
    CASE_KEYSYM(XKB_KEY_h, 64)
    CASE_KEYSYM(XKB_KEY_j, 65)
    CASE_KEYSYM(XKB_KEY_k, 66)
    CASE_KEYSYM(XKB_KEY_l, 67)
    CASE_KEYSYM(XKB_KEY_semicolon, 68)
    CASE_KEYSYM(XKB_KEY_apostrophe, 69)
    CASE_KEYSYM(XKB_KEY_Return, 70)
    CASE_KEYSYM(XKB_KEY_KP_4, 71)
    CASE_KEYSYM(XKB_KEY_KP_Left, 71)
    CASE_KEYSYM(XKB_KEY_KP_5, 72)
    CASE_KEYSYM(XKB_KEY_KP_Begin, 72)
    CASE_KEYSYM(XKB_KEY_KP_6, 73)
    CASE_KEYSYM(XKB_KEY_KP_Right, 73)
    CASE_KEYSYM(XKB_KEY_Shift_L, 74)
    CASE_KEYSYM(XKB_KEY_z, 75)
    CASE_KEYSYM(XKB_KEY_x, 76)
    CASE_KEYSYM(XKB_KEY_c, 77)
    CASE_KEYSYM(XKB_KEY_v, 78)
    CASE_KEYSYM(XKB_KEY_b, 79)
    CASE_KEYSYM(XKB_KEY_n, 80)
    CASE_KEYSYM(XKB_KEY_m, 81)
    CASE_KEYSYM(XKB_KEY_comma, 82)
    CASE_KEYSYM(XKB_KEY_period, 83)
    CASE_KEYSYM(XKB_KEY_slash, 84)
    CASE_KEYSYM(XKB_KEY_Shift_R, 85)
    CASE_KEYSYM(XKB_KEY_Up, 86)
    CASE_KEYSYM(XKB_KEY_KP_1, 87)
    CASE_KEYSYM(XKB_KEY_KP_End, 87)
    CASE_KEYSYM(XKB_KEY_KP_2, 88)
    CASE_KEYSYM(XKB_KEY_KP_Down, 88)
    CASE_KEYSYM(XKB_KEY_KP_3, 89)
    CASE_KEYSYM(XKB_KEY_KP_Next, 89)
    CASE_KEYSYM(XKB_KEY_KP_Enter, 90)
    CASE_KEYSYM(XKB_KEY_Control_L, 91)
    CASE_KEYSYM(XKB_KEY_Super_L, 92)
    CASE_KEYSYM(XKB_KEY_Alt_L, 93)
    CASE_KEYSYM(XKB_KEY_space, 94)
    CASE_KEYSYM(XKB_KEY_Alt_R, 95)
    CASE_KEYSYM(XKB_KEY_Menu, 96)
    CASE_KEYSYM(XKB_KEY_Control_R, 97)
    CASE_KEYSYM(XKB_KEY_Left, 98)
    CASE_KEYSYM(XKB_KEY_Down, 99)
    CASE_KEYSYM(XKB_KEY_Right, 100)
    CASE_KEYSYM(XKB_KEY_KP_0, 101)
    CASE_KEYSYM(XKB_KEY_KP_Insert, 101)
    CASE_KEYSYM(XKB_KEY_KP_Decimal, 102)
    CASE_KEYSYM(XKB_KEY_KP_Delete, 102)
    default:
        return;
#undef CASE_KEYSYM
    }
    if (HIDKeyboard* keyboard = GetKeyboard(); keyboard != nullptr)
        keyboard->HandleKeyEvent(keycode, release);
}
