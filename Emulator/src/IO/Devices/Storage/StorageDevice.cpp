/*
Copyright (©) 2024  Frosty515

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

#include "StorageDevice.hpp"

#include <Emulator.hpp>

#include "PhysicalRegionListBuffer.hpp"

StorageDevice::StorageDevice(MMU* PhysicalMMU, const char* path)
    : IODevice(IODeviceID::STORAGE, 3, 1), m_PhysicalMMU(PhysicalMMU), m_command(0), m_status{0, 0, 0, 0, 0, 0, 0}, m_data(0), m_buffer(nullptr), m_file(path), m_transferCommandStatus{0, 0, false, false} {
}

StorageDevice::~StorageDevice() {
}

void StorageDevice::Initialise() {
    m_file.Initialise();
    m_buffer = new PhysicalRegionListBuffer(this, m_PhysicalMMU);
}

void StorageDevice::Destroy() {
    m_file.Destroy();
    m_buffer->ClearList();
    delete m_buffer;
}

uint8_t StorageDevice::ReadByte(uint64_t address) {
    return ReadQWord(address) & 0xFF;
}

uint16_t StorageDevice::ReadWord(uint64_t address) {
    return ReadQWord(address) & 0xFFFF;
}

uint32_t StorageDevice::ReadDWord(uint64_t address) {
    return ReadQWord(address) & 0xFFFF'FFFF;
}

uint64_t StorageDevice::ReadQWord(uint64_t address) {
    switch (static_cast<StorageDeviceRegisters>(address)) {
    case StorageDeviceRegisters::COMMAND:
        return m_command;
    case StorageDeviceRegisters::STATUS:
        return *reinterpret_cast<uint8_t*>(&m_status);
    case StorageDeviceRegisters::DATA:
        return m_data;
    default:
        return 0;
    }
}

void StorageDevice::WriteByte(uint64_t address, uint8_t data) {
    WriteQWord(address, data);
}

void StorageDevice::WriteWord(uint64_t address, uint16_t data) {
    WriteQWord(address, data);
}

void StorageDevice::WriteDWord(uint64_t address, uint32_t data) {
    WriteQWord(address, data);
}

void StorageDevice::WriteQWord(uint64_t address, uint64_t data) {
    switch (static_cast<StorageDeviceRegisters>(address)) {
    case StorageDeviceRegisters::COMMAND:
        m_command = data;
        HandleCommand(static_cast<StorageDeviceCommands>(m_command));
        break;
    case StorageDeviceRegisters::STATUS:
        *reinterpret_cast<uint8_t*>(&m_status) = data;
        break;
    case StorageDeviceRegisters::DATA:
        m_data = data;
        break;
    default:
        break;
    }
}

void StorageDevice::StartTransfer() {
    if (!m_buffer->ParseList()) {
        m_status.ERR = 1;
        m_status.TRN = 0;
        m_status.RDY = 1;
        return;
    }

    if (m_transferCommandStatus.write)
        m_buffer->Read(0, reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(m_file.GetData()) + (m_transferCommandStatus.LBA << 9)), m_transferCommandStatus.Count << 9);
    else
        m_buffer->Write(0, reinterpret_cast<const uint8_t*>(reinterpret_cast<uint64_t>(m_file.GetData()) + (m_transferCommandStatus.LBA << 9)), m_transferCommandStatus.Count << 9);

    m_status.TRN = 0;
    m_status.ERR = 0;
    m_status.RDY = 1;

    if (m_transferCommandStatus.INT) {
        m_status.INTP = 1;
        RaiseInterrupt(0);
    }
}

void StorageDevice::HandleCommand(StorageDeviceCommands command) {
    // if (!m_status.RDY)
    //     return;

    switch (command) {
    case StorageDeviceCommands::CONFIGURE: {
        m_status.RDY = 0;
        StorageDevice_ConfigureRequest* request = reinterpret_cast<StorageDevice_ConfigureRequest*>(&m_data);
        m_status.EN = request->EN;
        m_status.INTE = request->INTE;
        m_status.ERR = 0;
        m_status.RDY = 1;
        break;
    }
    case StorageDeviceCommands::GET_DEVICE_INFO: {
        m_status.RDY = 0;
        uint64_t addr = m_data;
        if (!m_PhysicalMMU->ValidateWrite(addr, sizeof(StorageDevice_GetDeviceInfoResponse))) {
            m_status.ERR = 1;
            m_status.RDY = 1;
            return;
        }
        size_t size = m_file.GetSize();
        StorageDevice_GetDeviceInfoResponse response{size, size >> 9 /* 512 bytes per block */};
        m_PhysicalMMU->WriteBuffer(addr, reinterpret_cast<uint8_t*>(&response), sizeof(StorageDevice_GetDeviceInfoResponse));
        m_status.ERR = 0;
        m_status.RDY = 1;
        break;
    }
    case StorageDeviceCommands::READ: {
        m_status.RDY = 0;
        m_status.TRN = 0;
        uint64_t addr = m_data;
        if (!m_PhysicalMMU->ValidateRead(addr, sizeof(StorageDevice_TransferRequest))) {
            m_status.ERR = 1;
            m_status.RDY = 1;
            return;
        }
        StorageDevice_TransferRequest request;
        m_PhysicalMMU->ReadBuffer(addr, reinterpret_cast<uint8_t*>(&request), sizeof(StorageDevice_TransferRequest));
        if (request.FLAGS.INT && !m_status.INTE) {
            m_status.ERR = 1;
            m_status.RDY = 1;
            return;
        }
        if (request.COUNT == 0) {
            m_status.ERR = 1;
            m_status.RDY = 1;
            return;
        }
        if (request.LBA + request.COUNT > m_file.GetSize() >> 9) {
            m_status.ERR = 1;
            m_status.RDY = 1;
            return;
        }
        m_buffer->ClearList();
        m_buffer->ResetList(request.PRLS, request.PRLNC, request.COUNT << 9);
        m_status.TRN = 1;
        m_status.ERR = 0;
        m_transferCommandStatus.LBA = request.LBA;
        m_transferCommandStatus.Count = request.COUNT;
        m_transferCommandStatus.INT = request.FLAGS.INT;
        m_transferCommandStatus.write = false;
        Emulator::RaiseEvent({Emulator::EventType::StorageTransfer, reinterpret_cast<uint64_t>(this)});
        break;
    }
    case StorageDeviceCommands::WRITE: {
        m_status.RDY = 0;
        m_status.TRN = 0;
        uint64_t addr = m_data;
        if (!m_PhysicalMMU->ValidateRead(addr, sizeof(StorageDevice_TransferRequest))) {
            m_status.ERR = 1;
            m_status.RDY = 1;
            return;
        }
        StorageDevice_TransferRequest request;
        m_PhysicalMMU->ReadBuffer(addr, reinterpret_cast<uint8_t*>(&request), sizeof(StorageDevice_TransferRequest));
        if (request.FLAGS.INT && !m_status.INTE) {
            m_status.ERR = 1;
            m_status.RDY = 1;
            return;
        }
        if (request.COUNT == 0) {
            m_status.ERR = 1;
            m_status.RDY = 1;
            return;
        }
        if (request.LBA + request.COUNT > m_file.GetSize() >> 9) {
            m_status.ERR = 1;
            m_status.RDY = 1;
            return;
        }
        m_buffer->ClearList();
        m_buffer->ResetList(request.PRLS, request.PRLNC, request.COUNT << 9);
        m_status.TRN = 1;
        m_status.ERR = 0;
        m_transferCommandStatus.LBA = request.LBA;
        m_transferCommandStatus.Count = request.COUNT;
        m_transferCommandStatus.INT = request.FLAGS.INT;
        m_transferCommandStatus.write = true;
        Emulator::RaiseEvent({Emulator::EventType::StorageTransfer, reinterpret_cast<uint64_t>(this)});
        break;
    }
    }
}
