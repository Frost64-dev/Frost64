/*
Copyright (©) 2023-2026  Frosty515

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

#include "Register.hpp"

#include "Emulator.hpp"
#include "Exceptions.hpp"
#include "Instruction/Operand.hpp"

Register::Register()
    : m_dirty(false), m_writable(false), m_value(0), m_type(RegisterType::Unknown), m_index(0), m_ID(0xFF) {
}

Register::Register(uint8_t ID, bool writable, uint64_t value)
    : m_dirty(false), m_writable(writable), m_value(value), m_type(RegisterType::Unknown), m_index(0), m_ID(ID) {
    DecodeID(ID);
}

Register::Register(RegisterType type, uint8_t index, bool writable, uint64_t value)
    : m_dirty(false), m_writable(writable), m_value(value), m_type(type), m_index(index), m_ID(0xFF) {
    switch (type) {
    case RegisterType::GeneralPurpose:
        m_ID = index;
        break;
    case RegisterType::Stack:
        m_ID = 0x10 | index;
        break;
    case RegisterType::Control:
        m_ID = 0x20 | index;
        break;
    case RegisterType::Status:
        m_ID = 0x24;
        break;
    case RegisterType::Instruction:
        m_ID = 0x25;
        break;
    default:
        m_ID = 0xFF;
        break;
    }
}

Register::~Register() {
}

uint64_t Register::GetValue() const {
    return m_value;
}

uint64_t Register::GetValue(OperandSize size) const {
    switch (size) {
    case OperandSize::BYTE:
        return m_value & 0xFF;
    case OperandSize::WORD:
        return m_value & 0xFFFF;
    case OperandSize::DWORD:
        return m_value & 0xFFFF'FFFF;
    case OperandSize::QWORD:
        return m_value;
    default:
        return 0;
    }
}

bool Register::SetValue(uint64_t value, bool) {
    m_value = value;
    return true;
}

bool Register::SetValue(uint64_t value, OperandSize size) {
    switch (size) {
    case OperandSize::BYTE:
        m_value = (m_value & 0xFFFF'FFFF'FFFF'FF00) | (value & 0xFF);
        break;
    case OperandSize::WORD:
        m_value = (m_value & 0xFFFF'FFFF'FFFF'0000) | (value & 0xFFFF);
        break;
    case OperandSize::DWORD:
        m_value = (m_value & 0xFFFF'FFFF'0000'0000) | (value & 0xFFFF'FFFF);
        break;
    case OperandSize::QWORD:
        m_value = value;
        break;
    default:
        return false;
    }
    return true;
}

const char* Register::GetName() const {
    switch (m_type) {
    case RegisterType::GeneralPurpose:
        switch (m_index) {
        case 0:
            return "R0";
        case 1:
            return "R1";
        case 2:
            return "R2";
        case 3:
            return "R3";
        case 4:
            return "R4";
        case 5:
            return "R5";
        case 6:
            return "R6";
        case 7:
            return "R7";
        case 8:
            return "R8";
        case 9:
            return "R9";
        case 10:
            return "R10";
        case 11:
            return "R11";
        case 12:
            return "R12";
        case 13:
            return "R13";
        case 14:
            return "R14";
        case 15:
            return "R15";
        default:
            return "Unknown";
        }
    case RegisterType::Stack:
        switch (m_index) {
        case 0:
            return "SCP";
        case 1:
            return "SBP";
        case 2:
            return "STP";
        default:
            return "Unknown";
        }
    case RegisterType::Control:
        switch (m_index) {
        case 0:
            return "CR0";
        case 1:
            return "CR1";
        case 2:
            return "CR2";
        case 3:
            return "CR3";
        case 4:
            return "CR4";
        case 5:
            return "CR5";
        case 6:
            return "CR6";
        case 7:
            return "CR7";
        default:
            return "Unknown";
        }
    case RegisterType::Status:
        return "STS";
    case RegisterType::Instruction:
        return "IP";
    default:
        return "Unknown";
    }
}


void Register::DecodeID(uint8_t ID) {
    uint8_t type = ID >> 4;
    uint8_t index = ID & 0x0F;
    m_type = RegisterType::Unknown;
    switch (type) {
    case 0:
        m_type = RegisterType::GeneralPurpose;
        m_index = index;
        break;
    case 1:
        m_type = RegisterType::Stack;
        m_index = index;
        break;
    case 2:
        if (index < 8) {
            m_type = RegisterType::Control;
            m_index = index;
        } else if (index == 8) {
            m_type = RegisterType::Status;
            m_index = 0;
        } else if (index == 9) {
            m_type = RegisterType::Instruction;
            m_index = 0;
        }
        break;
    default:
        break;
    }
}

SyncingRegister::SyncingRegister()
    : Register(), m_callback(nullptr, nullptr) {
}

SyncingRegister::SyncingRegister(uint8_t ID, bool writable, RegisterSyncCallback callback, uint64_t value)
    : Register(ID, writable, value), m_callback(callback) {
}

SyncingRegister::SyncingRegister(RegisterType type, uint8_t index, bool writable, RegisterSyncCallback callback, uint64_t value)
    : Register(type, index, writable, value), m_callback(callback) {
}

SyncingRegister::~SyncingRegister() {
}

bool SyncingRegister::SetValue(uint64_t value, bool) {
    m_value = value;
    m_dirty = true;
    if (m_callback.callback != nullptr)
        m_callback.callback(m_callback.data, m_value);
    return true;
}

bool SyncingRegister::SetValue(uint64_t value, OperandSize size) {
    switch (size) {
    case OperandSize::BYTE:
        m_value = (m_value & 0xFFFF'FFFF'FFFF'FF00) | (value & 0xFF);
        break;
    case OperandSize::WORD:
        m_value = (m_value & 0xFFFF'FFFF'FFFF'0000) | (value & 0xFFFF);
        break;
    case OperandSize::DWORD:
        m_value = (m_value & 0xFFFF'FFFF'0000'0000) | (value & 0xFFFF'FFFF);
        break;
    case OperandSize::QWORD:
        m_value = value;
        break;
    default:
        return false;
    }
    m_dirty = true;
    if (m_callback.callback != nullptr)
        m_callback.callback(m_callback.data, m_value);
    return true;
}

void SyncingRegister::SetCallback(RegisterSyncCallback callback) {
    m_callback = callback;
}

SafeSyncingRegister::SafeSyncingRegister()
    : Register(), m_cpu(nullptr), m_callback(nullptr, nullptr) {
}

SafeSyncingRegister::SafeSyncingRegister(Emulator::CPUState* cpu, uint8_t ID, bool writable, RegisterSyncCallback callback, uint64_t value)
    : Register(ID, writable, value), m_cpu(cpu), m_callback(callback) {
}

SafeSyncingRegister::SafeSyncingRegister(Emulator::CPUState* cpu, RegisterType type, uint8_t index, bool writable, RegisterSyncCallback callback, uint64_t value)
    : Register(type, index, writable, value), m_cpu(cpu), m_callback(callback) {
}

SafeSyncingRegister::~SafeSyncingRegister() {
}

bool SafeSyncingRegister::SetValue(uint64_t value, bool force) {
    if (!force && !m_writable)
        return false;
    if (m_type == RegisterType::Control) {
        if (Emulator::isInProtectedMode(m_cpu) && Emulator::isInUserMode(m_cpu))
            m_cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    }
    m_value = value;
    m_dirty = true;
    if (m_callback.callback != nullptr)
        m_callback.callback(m_callback.data, m_value);
    return true;
}

bool SafeSyncingRegister::SetValue(uint64_t value, OperandSize size) {
    if (!m_writable)
        return false;
    if (m_type == RegisterType::Control) {
        if (Emulator::isInProtectedMode(m_cpu) && Emulator::isInUserMode(m_cpu))
            m_cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    }
    switch (size) {
    case OperandSize::BYTE:
        m_value = (m_value & 0xFFFF'FFFF'FFFF'FF00) | (value & 0xFF);
        break;
    case OperandSize::WORD:
        m_value = (m_value & 0xFFFF'FFFF'FFFF'0000) | (value & 0xFFFF);
        break;
    case OperandSize::DWORD:
        m_value = (m_value & 0xFFFF'FFFF'0000'0000) | (value & 0xFFFF'FFFF);
        break;
    case OperandSize::QWORD:
        m_value = value;
        break;
    default:
        return false;
    }
    m_dirty = true;
    if (m_callback.callback != nullptr)
        m_callback.callback(m_callback.data, m_value);
    return true;
}

uint64_t SafeSyncingRegister::GetValue() const {
    if (m_type == RegisterType::Control) {
        if (Emulator::isInProtectedMode(m_cpu) && Emulator::isInUserMode(m_cpu))
            m_cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    }
    return m_value;
}

uint64_t SafeSyncingRegister::GetValue(OperandSize size) const {
    if (m_type == RegisterType::Control) {
        if (Emulator::isInProtectedMode(m_cpu) && Emulator::isInUserMode(m_cpu))
            m_cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    }
    switch (size) {
    case OperandSize::BYTE:
        return m_value & 0xFF;
    case OperandSize::WORD:
        return m_value & 0xFFFF;
    case OperandSize::DWORD:
        return m_value & 0xFFFF'FFFF;
    case OperandSize::QWORD:
        return m_value;
    default:
        return 0;
    }
}

void SafeSyncingRegister::SetCallback(RegisterSyncCallback callback) {
    m_callback = callback;
}

SafeRegister::SafeRegister()
    : Register() {
}

SafeRegister::SafeRegister(Emulator::CPUState* cpu, uint8_t ID, bool writable, uint64_t value)
    : Register(ID, writable, value), m_cpu(cpu) {
}

SafeRegister::SafeRegister(Emulator::CPUState* cpu, RegisterType type, uint8_t index, bool writable, uint64_t value)
    : Register(type, index, writable, value), m_cpu(cpu) {
}

SafeRegister::~SafeRegister() {
}

bool SafeRegister::SetValue(uint64_t value, bool force) {
    if (!force && !m_writable)
        return false;
    if (m_type == RegisterType::Control) {
        if (Emulator::isInProtectedMode(m_cpu) && Emulator::isInUserMode(m_cpu))
            m_cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    }
    m_value = value;
    return true;
}

bool SafeRegister::SetValue(uint64_t value, OperandSize size) {
    if (!m_writable)
        return false;
    if (m_type == RegisterType::Control) {
        if (Emulator::isInProtectedMode(m_cpu) && Emulator::isInUserMode(m_cpu))
            m_cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    }
    switch (size) {
    case OperandSize::BYTE:
        m_value = (m_value & 0xFFFF'FFFF'FFFF'FF00) | (value & 0xFF);
        break;
    case OperandSize::WORD:
        m_value = (m_value & 0xFFFF'FFFF'FFFF'0000) | (value & 0xFFFF);
        break;
    case OperandSize::DWORD:
        m_value = (m_value & 0xFFFF'FFFF'0000'0000) | (value & 0xFFFF'FFFF);
        break;
    case OperandSize::QWORD:
        m_value = value;
        break;
    default:
        return false;
    }
    return true;
}

void SafeRegister::SetValueNoCheck(uint64_t value) {
    m_value = value;
}


uint64_t SafeRegister::GetValue() const {
    if (m_type == RegisterType::Control) {
        if (Emulator::isInProtectedMode(m_cpu) && Emulator::isInUserMode(m_cpu))
            m_cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    }
    return m_value;
}

uint64_t SafeRegister::GetValue(OperandSize size) const {
    if (m_type == RegisterType::Control) {
        if (Emulator::isInProtectedMode(m_cpu) && Emulator::isInUserMode(m_cpu))
            m_cpu->exceptionHandler->RaiseException(Exception::USER_MODE_VIOLATION);
    }
    switch (size) {
    case OperandSize::BYTE:
        return m_value & 0xFF;
    case OperandSize::WORD:
        return m_value & 0xFFFF;
    case OperandSize::DWORD:
        return m_value & 0xFFFF'FFFF;
    case OperandSize::QWORD:
        return m_value;
    default:
        return 0;
    }
}

LockableSafeRegister::LockableSafeRegister()
    : SafeRegister() {
}

LockableSafeRegister::LockableSafeRegister(Emulator::CPUState* cpu, uint8_t ID, bool writable, uint64_t value)
    : SafeRegister(cpu, ID, writable, value) {
}

LockableSafeRegister::LockableSafeRegister(Emulator::CPUState* cpu, RegisterType type, uint8_t index, bool writable, uint64_t value)
    : SafeRegister(cpu, type, index, writable, value) {
}

LockableSafeRegister::~LockableSafeRegister() {
}

bool LockableSafeRegister::SetValue(uint64_t value, bool force, bool lock) {
    if (lock)
        m_mutex.lock();
    bool result = SafeRegister::SetValue(value, force);
    if (lock)
        m_mutex.unlock();
    return result;
}

bool LockableSafeRegister::SetValue(uint64_t value, OperandSize size, bool lock) {
    if (lock)
        m_mutex.lock();
    bool result = SafeRegister::SetValue(value, size);
    if (lock)
        m_mutex.unlock();
    return result;
}

void LockableSafeRegister::SetValueNoCheck(uint64_t value, bool lock) {
    if (lock)
        m_mutex.lock();
    m_value = value;
    if (lock)
        m_mutex.unlock();
}


uint64_t LockableSafeRegister::GetValue(bool lock) const {
    if (lock)
        m_mutex.lock();
    uint64_t result = SafeRegister::GetValue();
    if (lock)
        m_mutex.unlock();
    return result;
}

uint64_t LockableSafeRegister::GetValue(OperandSize size, bool lock) const {
    if (lock)
        m_mutex.lock();
    uint64_t result = SafeRegister::GetValue(size);
    if (lock)
        m_mutex.unlock();
    return result;
}

uint64_t LockableSafeRegister::GetValueNoCheck(bool lock) const {
    if (lock)
        m_mutex.lock();
    uint64_t result = m_value;
    if (lock)
        m_mutex.unlock();
    return result;
}
