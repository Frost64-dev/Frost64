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

#ifndef _IO_INPUT_DEVICE_HPP
#define _IO_INPUT_DEVICE_HPP

#include <cstdint>

#include <IO/IODevice.hpp>

#include <IO/Devices/Video/VideoDevice.hpp>

enum class InputDeviceType {
    KEYBOARD = 0,
    MOUSE = 1
};

enum class InputDeviceRegisters {
    COMMAND = 0,
    STATUS = 1,
    KEYBOARD = 2,
    MOUSE = 3
};

enum class InputDeviceCommands {
    INIT = 0,
    GET_DEV_INFO = 1,
    SET_DEV_INFO = 2,
    ACK_IRQ0 = 3,
    ACK_IRQ1 = 4,
};

struct [[gnu::packed]] Input_StatusRegister {
    uint8_t ERR : 1;
    uint8_t KBD_EN : 1;
    uint8_t KBD_INT : 1;
    uint8_t KBD_INTP : 1;
    uint8_t KBD_RDY : 1;
    uint8_t MSE_EN : 1;
    uint8_t MSE_INT : 1;
    uint8_t MSE_INTP : 1;
    uint8_t MSE_RDY : 1;
    uint64_t RSVD : 55;
};

enum class InputBackendType {
    NONE,
    XCB,
};

class Keyboard;
class Mouse;

class InputDeviceBus : public IODevice {
public:
    explicit InputDeviceBus(InputBackendType backendType, VideoDevice* videoDevice);
    virtual ~InputDeviceBus();

    uint8_t ReadByte(uint64_t address) override;
    uint16_t ReadWord(uint64_t address) override;
    uint32_t ReadDWord(uint64_t address) override;
    uint64_t ReadQWord(uint64_t address) override;

    void WriteByte(uint64_t address, uint8_t data) override;
    void WriteWord(uint64_t address, uint16_t data) override;
    void WriteDWord(uint64_t address, uint32_t data) override;
    void WriteQWord(uint64_t address, uint64_t data) override;

    InputBackendType GetBackendType() const { return m_backendType; }

    Input_StatusRegister GetStatus();
    void SetStatus(Input_StatusRegister status);

private:
    uint64_t ReadRegister(uint64_t offset);
    void WriteRegister(uint64_t offset, uint64_t data);

    void RunCommand(uint64_t command);

    InputBackendType m_backendType;
    VideoDevice* m_videoDevice;

    Keyboard* m_keyboard;
    Mouse* m_mouse;

    Input_StatusRegister m_status;
    uint64_t m_keyboardData;
    uint64_t m_mouseData;
    bool m_keyboardDataPendingRead;
    bool m_mouseDataPendingRead;
};

InputBackendType VideoBackendToInputBackend(VideoBackendType backendType);

#endif /* _IO_INPUT_DEVICE_HPP */