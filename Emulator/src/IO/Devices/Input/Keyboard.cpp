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

#include "InputDeviceBus.hpp"
#include "Keyboard.hpp"

#include <chrono>
#include <thread>

#include <IO/Devices/Video/VideoBackend.hpp>

#ifdef ENABLE_XCB
#include "backends/XCB/XCBKeyboard.hpp"

#include <IO/Devices/Video/backends/XCB/XCBVideoBackend.hpp>
#endif

Keyboard::Keyboard(InputDeviceBus* bus, VideoBackend* videoBackend)
    : m_bus(bus), m_videoBackend(videoBackend), m_backend(nullptr), m_modifiers{0, 0, 0, 0, 0, 0, 0, 0}, m_currentEvent({255, m_modifiers, 0, 0}), m_dataRead(true) {
}

Keyboard::~Keyboard() {
}

void Keyboard::Init() {
#ifdef ENABLE_XCB
    if (m_bus->GetBackendType() == InputBackendType::XCB) {
        XCBVideoBackend* videoBackend = dynamic_cast<XCBVideoBackend*>(m_videoBackend);
        XCBKeyboardBackend* backend = new XCBKeyboardBackend(videoBackend->GetXCBConnection());
        m_backend = backend;
        backend->SetKeyboard(this);
        backend->Initialise();
        videoBackend->SetKeyboard(backend);
    }
#endif
}

void Keyboard::HandleKeyEvent(Keycode keycode, bool release) {
    Input_StatusRegister status = m_bus->GetStatus();
    if (status.KBD_EN == 0)
        return;

    if (!m_dataRead)
        return;


    m_currentEvent.keycode = keycode;
    m_currentEvent.Released = release;

    switch (keycode) {
#define MOD_CASE(num, name) case num: m_modifiers.name = !release; break;
    MOD_CASE(91, L_CTRL)
    MOD_CASE(97, R_CTRL)
    MOD_CASE(74, L_SHIFT)
    MOD_CASE(85, R_SHIFT)
    MOD_CASE(93, L_ALT)
    MOD_CASE(95, R_ALT)
    MOD_CASE(92, SUPER)
    MOD_CASE(96, MENU)
    default:
        break;
    }

    m_currentEvent.Modifiers = m_modifiers;

    status.KBD_RDY = 1;
    m_dataRead = false;

    bool wasInterruptPending = status.KBD_INTP == 1;
    if (status.KBD_INT == 1)
        status.KBD_INTP = 1;

    m_bus->SetStatus(status); // set the status before triggering the interrupt

    if (status.KBD_INT == 1 && !wasInterruptPending) {
        m_bus->RaiseInterrupt(0);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"

uint64_t Keyboard::Read() {
    m_dataRead = true;
    uint64_t* result = reinterpret_cast<uint64_t*>(&m_currentEvent);
    return *result;
}

#pragma GCC diagnostic pop