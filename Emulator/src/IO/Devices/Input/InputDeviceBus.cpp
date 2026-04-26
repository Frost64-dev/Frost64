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

#include <IO/Devices/Video/VideoDevice.hpp>
#include <IO/Devices/Video/VideoBackend.hpp>

InputDeviceBus::InputDeviceBus(InputBackendType backendType, VideoDevice* videoDevice)
 : IODevice(IODeviceID::INPUT, 0x20, 2), m_backendType(backendType), m_videoDevice(videoDevice), m_keyboard(nullptr), m_mouse(nullptr), m_status(), m_keyboardData(0), m_mouseData(0), m_keyboardDataPendingRead(false), m_mouseDataPendingRead(false) {
}

InputDeviceBus::~InputDeviceBus() {

}

uint8_t InputDeviceBus::ReadByte(uint64_t address) {
    return ReadRegister(address) & 0xFF;
}

uint16_t InputDeviceBus::ReadWord(uint64_t address) {
    return ReadRegister(address) & 0xFFFF;
}

uint32_t InputDeviceBus::ReadDWord(uint64_t address) {
    return ReadRegister(address) & 0xFFFFFFFF;
}

uint64_t InputDeviceBus::ReadQWord(uint64_t address) {
    return ReadRegister(address);
}

void InputDeviceBus::WriteByte(uint64_t address, uint8_t data) {
    uint64_t current = ReadRegister(address);
    current &= ~0xFF;
    current |= data;
    WriteRegister(address, current);
}

void InputDeviceBus::WriteWord(uint64_t address, uint16_t data) {
    uint64_t current = ReadRegister(address);
    current &= ~0xFFFF;
    current |= data;
    WriteRegister(address, current);
}

void InputDeviceBus::WriteDWord(uint64_t address, uint32_t data) {
    uint64_t current = ReadRegister(address);
    current &= ~0xFFFFFFFF;
    current |= data;
    WriteRegister(address, current);
}

void InputDeviceBus::WriteQWord(uint64_t address, uint64_t data) {
    WriteRegister(address, data);
}

Input_StatusRegister InputDeviceBus::GetStatus() {
    return m_status;
}

void InputDeviceBus::SetStatus(Input_StatusRegister status) {
    m_status = status;
}

uint64_t InputDeviceBus::ReadRegister(uint64_t offset) {
    switch (static_cast<InputDeviceRegisters>(offset)) {
    case InputDeviceRegisters::COMMAND:
        return 0;
    case InputDeviceRegisters::STATUS:
        return *reinterpret_cast<uint64_t*>(&m_status);
    case InputDeviceRegisters::KEYBOARD:
        if (m_keyboardDataPendingRead) {
            m_keyboardDataPendingRead = false;
            return m_keyboardData;
        }
        if (m_keyboard != nullptr)
            return m_keyboard->Read();
        return 0;
    case InputDeviceRegisters::MOUSE:
        return m_mouseData;
    default:
        return 0;
    }
}

void InputDeviceBus::WriteRegister(uint64_t offset, uint64_t data) {
    switch (static_cast<InputDeviceRegisters>(offset)) {
    case InputDeviceRegisters::COMMAND:
        RunCommand(data);
        break;
    case InputDeviceRegisters::STATUS:
        break;
    case InputDeviceRegisters::KEYBOARD:
        m_keyboardData = data;
        break;
    case InputDeviceRegisters::MOUSE:
        m_mouseData = data;
        break;
    }
}

bool IsBackendTypeEqual(VideoBackendType backendType, InputBackendType deviceType) {
    if (backendType == VideoBackendType::XCB && deviceType == InputBackendType::XCB)
        return true;
    return false;
}

void InputDeviceBus::RunCommand(uint64_t command) {
    m_status.ERR = 0;
    switch (static_cast<InputDeviceCommands>(command)) {
    case InputDeviceCommands::INIT: {
        // Step 1: Check if the video device is initialised and using the same backend type
        if (m_videoDevice == nullptr) {
            m_status.ERR = 1;
            break;
        }
        if (VideoBackendToInputBackend(m_videoDevice->GetBackendType()) != m_backendType || !m_videoDevice->isInitialised()) {
            m_status.ERR = 1;
            break;
        }

        if ((m_keyboardData & 1) > 0) { // keyboard enabled
            m_status.KBD_RDY = 0;
            m_status.KBD_INT = m_keyboardData >> 1 & 1;
            m_status.KBD_INTP = 0;

            VideoBackend* videoBackend = m_videoDevice->GetBackend();
            if (videoBackend == nullptr)
                m_status.ERR = 1;

            m_keyboard = new Keyboard(this, videoBackend);
            m_keyboard->Init();

            m_status.KBD_EN = 1;
        } else {
            m_status.KBD_EN = 0;
            m_status.KBD_RDY = 0;
        }

        break;
    }
    case InputDeviceCommands::GET_DEV_INFO: {
        m_keyboardData = m_status.KBD_EN | m_status.KBD_INT << 1;
        m_mouseData = m_status.MSE_EN | m_status.MSE_INT << 1;
        m_keyboardDataPendingRead = true;
        m_mouseDataPendingRead = true;
        m_status.ERR = 0;
        break;
    }
    case InputDeviceCommands::SET_DEV_INFO: {
        m_status.KBD_EN = m_keyboardData & 1;
        m_status.KBD_INT = m_keyboardData >> 1 & 1;
        m_status.MSE_EN = m_mouseData & 1;
        m_status.MSE_INT = m_mouseData >> 1 & 1;
        m_status.ERR = 0;
        break;
    }
    case InputDeviceCommands::ACK_IRQ0: {
        m_status.KBD_INTP = 0;
        m_status.ERR = 0;
        break;
    }
    case InputDeviceCommands::ACK_IRQ1: {
        m_status.MSE_INTP = 0;
        m_status.ERR = 0;
        break;
    }
    }
}

InputBackendType VideoBackendToInputBackend(VideoBackendType backendType) {
#ifdef ENABLE_XCB
    if (backendType == VideoBackendType::XCB)
        return InputBackendType::XCB;
#endif
    return InputBackendType::NONE;
}
