/*
Copyright (©) 2023-2025  Frosty515

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

#ifndef _REGISTER_HPP
#define _REGISTER_HPP

#include <cstdint>

enum class OperandSize;

enum class RegisterType {
    GeneralPurpose,
    Instruction,
    Stack,
    Status,
    Control,
    Unknown // Should never be used, only here for error checking
};

enum RegisterID {
    RegisterID_R0 = 0,
    RegisterID_R1,
    RegisterID_R2,
    RegisterID_R3,
    RegisterID_R4,
    RegisterID_R5,
    RegisterID_R6,
    RegisterID_R7,
    RegisterID_R8,
    RegisterID_R9,
    RegisterID_R10,
    RegisterID_R11,
    RegisterID_R12,
    RegisterID_R13,
    RegisterID_R14,
    RegisterID_R15,
    RegisterID_SCP,
    RegisterID_SBP,
    RegisterID_STP,
    RegisterID_CR0 = 0x20,
    RegisterID_CR1,
    RegisterID_CR2,
    RegisterID_CR3,
    RegisterID_CR4,
    RegisterID_CR5,
    RegisterID_CR6,
    RegisterID_CR7,
    RegisterID_STS,
    RegisterID_IP,
    RegisterID_UNKNOWN = 0xFF
};

class Register {
public:
    Register();
    Register(uint8_t ID, bool writable, uint64_t value = 0);
    Register(RegisterType type, uint8_t index, bool writable, uint64_t value = 0);
    ~Register();

    [[gnu::always_inline,nodiscard]] inline RegisterType GetType() const { return m_type; }
    [[gnu::always_inline,nodiscard]] inline uint8_t GetIndex() const { return m_index; }
    [[gnu::always_inline,nodiscard]] inline uint8_t GetID() const { return m_ID; }

    virtual uint64_t GetValue() const;
    virtual uint64_t GetValue(OperandSize size) const;
    virtual bool SetValue(uint64_t value, bool = false);
    virtual bool SetValue(uint64_t value, OperandSize size);

    const char* GetName() const;

    [[gnu::always_inline]] inline void SetDirty(bool dirty) { m_dirty = dirty; }
    [[gnu::always_inline,nodiscard]] inline bool IsDirty() const { return m_dirty; }

    [[gnu::always_inline]] inline Register& operator=(uint64_t value) { SetValue(value); return *this; }
    [[gnu::always_inline]] inline operator uint64_t() const { return GetValue(); }
    [[gnu::always_inline]] inline uint64_t operator+=(uint64_t value) { SetValue(GetValue() + value); return GetValue(); }
    [[gnu::always_inline]] inline uint64_t operator-=(uint64_t value) { SetValue(GetValue() - value); return GetValue(); }

protected:
    bool m_dirty;
    bool m_writable;
    uint64_t m_value;
    RegisterType m_type;

private:
    void DecodeID(uint8_t ID);

private:
    uint8_t m_index;
    uint8_t m_ID;
};

class SyncingRegister : public Register {
public:
    SyncingRegister();
    SyncingRegister(uint8_t ID, bool writable, uint64_t value = 0);
    SyncingRegister(RegisterType type, uint8_t index, bool writable, uint64_t value = 0);
    ~SyncingRegister();

    bool SetValue(uint64_t value, bool = false) override;
    bool SetValue(uint64_t value, OperandSize size) override;
};

class SafeSyncingRegister : public Register {
public:
    SafeSyncingRegister();
    SafeSyncingRegister(uint8_t ID, bool writable, uint64_t value = 0);
    SafeSyncingRegister(RegisterType type, uint8_t index, bool writable, uint64_t value = 0);
    ~SafeSyncingRegister();

    bool SetValue(uint64_t value, bool force = false) override;
    bool SetValue(uint64_t value, OperandSize size) override;

    uint64_t GetValue() const override;
    uint64_t GetValue(OperandSize size) const override;
};

class SafeRegister : public Register {
public:
    SafeRegister();
    SafeRegister(uint8_t ID, bool writable, uint64_t value = 0);
    SafeRegister(RegisterType type, uint8_t index, bool writable, uint64_t value = 0);
    ~SafeRegister();

    bool SetValue(uint64_t value, bool force = false) override;
    bool SetValue(uint64_t value, OperandSize size) override;
    void SetValueNoCheck(uint64_t value);

    uint64_t GetValue() const override;
    uint64_t GetValue(OperandSize size) const override;
    uint64_t GetValueNoCheck() const { return m_value; }
};

#endif /* _REGISTER_HPP */