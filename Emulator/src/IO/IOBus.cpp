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

#include "IOBus.hpp"

#include "IODevice.hpp"
#include "IOMemoryRegion.hpp"

#ifdef EMULATOR_DEBUG
#include <stdio.h>
#endif

#include <Emulator.hpp>
#include <Exceptions.hpp>
#include <Interrupts.hpp>

void IOBus_HandleDeviceInterrupt(IODeviceID device, uint64_t index, void* data) {
    if (IOBus* bus = static_cast<IOBus*>(data); bus != nullptr)
        bus->HandleDeviceInterrupt(device, index);
}

IOBus* g_IOBus = nullptr;

IOBus::IOBus(MMU* mmu)
    : m_MMU(mmu), m_registers{0, {true, false, 0}, {0, 0, 0, 0}, {0, 0}}, m_interruptMap(USABLE_INTERRUPTS) {
    m_interruptMap.ClearAll();
}

IOBus::~IOBus() {
}

uint64_t IOBus::ReadRegister(uint64_t offset) {
#ifdef EMULATOR_DEBUG
    printf("IOBus::ReadRegister(%lu)\n", offset);
#endif
    Validate();
    switch (static_cast<IOBusRegister>(offset)) {
    case IOBusRegister::COMMAND:
        return 0;
    case IOBusRegister::STATUS: {
        uint64_t* temp = reinterpret_cast<uint64_t*>(&m_registers);
        return temp[1];
    }
    case IOBusRegister::DATA0:
    case IOBusRegister::DATA1:
    case IOBusRegister::DATA2:
    case IOBusRegister::DATA3:
        return m_registers.data[offset - static_cast<uint64_t>(IOBusRegister::DATA0)];
    }
    return 0;
}

void IOBus::WriteRegister(uint64_t offset, uint64_t data) {
#ifdef EMULATOR_DEBUG
    printf("IOBus::WriteRegister(%lu, %lu)\n", offset, data);
#endif
    Validate();
    switch (static_cast<IOBusRegister>(offset)) {
    case IOBusRegister::COMMAND:
        RunCommand(data);
        break;
    case IOBusRegister::STATUS: {
        IOBus_StatusRegister* temp = reinterpret_cast<IOBus_StatusRegister*>(&m_registers);
        m_registers.status = *temp;
        break;
    }
    case IOBusRegister::DATA0:
    case IOBusRegister::DATA1:
    case IOBusRegister::DATA2:
    case IOBusRegister::DATA3:
        m_registers.data[offset - static_cast<uint64_t>(IOBusRegister::DATA0)] = data;
        break;
    }
}

bool IOBus::AddDevice(IODevice* device) {
    if (FindDevice(device->GetID()) != nullptr)
        return false;
    m_devices.insert(device);
    device->SetInterruptCallback(IOBus_HandleDeviceInterrupt, this);
    for (uint64_t i = 0; i < device->GetInterruptCount(); i++)
        m_interruptMapping.insert({{device->GetID(), i}, 0});
    return true;
}

void IOBus::RemoveDevice(IODevice* device) {
    m_devices.remove(device);
    for (uint64_t i = 0; i < device->GetInterruptCount(); i++)
        m_interruptMapping.erase({device->GetID(), i});
    device->SetInterruptCallback(nullptr, nullptr);
}

void IOBus::HandleDeviceInterrupt(IODeviceID device, uint64_t index) {
    if (uint8_t SINT = m_interruptMapping[{device, index}]; SINT != 0)
        g_InterruptHandler->RaiseInterruptExternal(SINT);
}


void IOBus::Validate() const {
    if (Emulator::isInProtectedMode() && Emulator::isInUserMode())
        g_ExceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
}

void IOBus::RunCommand(uint64_t command) {
    IOBusCommands cmd = static_cast<IOBusCommands>(command);
    m_registers.status.error = false;
    m_registers.status.commandComplete = false;
    switch (cmd) {
    case IOBusCommands::GET_BUS_INFO: {
        IOBus_GetBusInfoResponse response{m_devices.getCount()};
        m_registers.data[0] = *reinterpret_cast<uint64_t*>(&response);
        break;
    }
    case IOBusCommands::GET_DEVICE_INFO: {
        // data[0] = index
        uint64_t index = m_registers.data[0];
        if (index >= m_devices.getCount()) {
            m_registers.status.error = true;
            break;
        }
        IODevice* device = m_devices.get(index);
        if (device == nullptr) {
            m_registers.status.error = true;
            break;
        }
        IOBus_GetDeviceInfoResponse response;
        response.ID = static_cast<uint64_t>(device->GetID());
        response.baseAddress = device->GetBaseAddress();
        response.size = device->GetSize();
        response.INTCount = device->GetInterruptCount();
        m_registers.data[0] = *reinterpret_cast<uint64_t*>(&response);
        break;
    }
    case IOBusCommands::SET_DEVICE_INFO: {
        // data[0] = ID
        // data[1] = baseAddress
        IODeviceID ID = static_cast<IODeviceID>(m_registers.data[0]);
        uint64_t baseAddress = m_registers.data[1];
        IODevice* device = FindDevice(ID);
        if (device == nullptr) {
            m_registers.status.error = true;
            break;
        }
        if (uint64_t oldBaseAddress = device->GetBaseAddress(); oldBaseAddress != 0) {
            // need to delete the old region
            if (IOMemoryRegion* region = device->GetMemoryRegion(); region != nullptr) {
                m_MMU->RemoveMemoryRegion(region);
                delete region;
                m_MMU->ReaddRegionSegment(device->GetReplacingRegionData());
            }
        }
        device->SetBaseAddress(baseAddress);
        if (baseAddress != 0) {
            uint64_t start = baseAddress;
            uint64_t end = baseAddress + device->GetSize() * 8;
            void* data_out = nullptr;
            if (!m_MMU->RemoveRegionSegment(start, end, &data_out)) {
                m_registers.status.error = true;
                break;
            }
            IOMemoryRegion* region = new IOMemoryRegion(start, end, device);
            m_MMU->AddMemoryRegion(region);
            device->SetMemoryRegion(region);
            device->SetReplacingRegionData(data_out);
        }
        break;
    }
    case IOBusCommands::GET_INTERRUPT_MAPPING: {
        // data[0] = deviceID
        // data[1] = interrupt
        IOBus_GetInterruptMappingRequest request;
        request.deviceID = m_registers.data[0];
        request.interrupt = m_registers.data[1];
        IODevice* device = FindDevice(static_cast<IODeviceID>(request.deviceID));
        if (device == nullptr) {
            m_registers.status.error = true;
            break;
        }
        if (request.interrupt >= device->GetInterruptCount()) {
            m_registers.status.error = true;
            break;
        }
        m_registers.data[0] = m_interruptMapping[{static_cast<IODeviceID>(request.deviceID), request.interrupt}];
        break;
    }
    case IOBusCommands::SET_INTERRUPT_MAPPING: {
        // data[0] = deviceID
        // data[1] = interrupt
        // data[2] = SINT
        IOBus_SetInterruptMappingRequest request;
        request.deviceID = m_registers.data[0];
        request.interrupt = m_registers.data[1];
        request.SINT = m_registers.data[2];
        IODevice* device = FindDevice(static_cast<IODeviceID>(request.deviceID));
        if (device == nullptr) {
            m_registers.status.error = true;
            break;
        }
        if (request.interrupt >= device->GetInterruptCount()) {
            m_registers.status.error = true;
            break;
        }
        if (request.SINT < RESERVED_INTERRUPTS && request.SINT != 0) {
            m_registers.status.error = true;
            break;
        }
        if (request.SINT != 0) {
            if (m_interruptMap.Test(request.SINT)) {
                m_registers.status.error = true;
                break;
            }
            m_interruptMap.Set(request.SINT);
        } else
            m_interruptMap.Clear(request.SINT);
        m_interruptMapping[{static_cast<IODeviceID>(request.deviceID), request.interrupt}] = request.SINT;
        break;
    }
    }
    m_registers.status.commandComplete = true;
}

IODevice* IOBus::FindDevice(IODeviceID ID) {
    for (uint64_t i = 0; i < m_devices.getCount(); i++) {
        if (IODevice* device = m_devices.get(i); device->GetID() == ID)
            return device;
    }
    return nullptr;
}
