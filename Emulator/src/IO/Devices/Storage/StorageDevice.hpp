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

#ifndef _STORAGE_IO_DEVICE_HPP
#define _STORAGE_IO_DEVICE_HPP

#include <IO/IODevice.hpp>

#include "StorageFile.hpp"

class PhysicalRegionListBuffer;
enum class StorageDeviceRegisters {
    COMMAND = 0,
    STATUS = 1,
    DATA = 2
};

struct [[gnu::packed]] StorageDeviceStatus {
    uint8_t EN    : 1;
    uint8_t ERR   : 1;
    uint8_t RDY   : 1;
    uint8_t TRN   : 1;
    uint8_t INTE  : 1;
    uint8_t INTP  : 1;
    uint64_t RSVD : 59;
};

enum class StorageDeviceCommands {
    CONFIGURE = 0,
    GET_DEVICE_INFO = 1,
    READ = 2,
    WRITE = 3
};

struct [[gnu::packed]] StorageDevice_ConfigureRequest {
    uint8_t EN    : 1;
    uint8_t INTE  : 1;
    uint64_t RSVD : 62;
};

struct [[gnu::packed]] StorageDevice_GetDeviceInfoResponse {
    uint64_t size;
    uint64_t blocks;
};

// Same for read and write
struct [[gnu::packed]] StorageDevice_TransferRequest {
    uint64_t LBA;
    uint64_t COUNT;
    uint64_t PRLS;
    uint64_t PRLNC;
    struct [[gnu::packed]] SD_TRQ_FLAGS {
        uint8_t INT   : 1;
        uint64_t RSVD : 63;
    } FLAGS;
};

class PhysicalRegionListBuffer;

class StorageDevice : public IODevice {
   public:
    explicit StorageDevice(MMU* PhysicalMMU, const char* path);
    ~StorageDevice() override;

    void Initialise();
    void Destroy();

    virtual uint8_t ReadByte(uint64_t address) override;
    virtual uint16_t ReadWord(uint64_t address) override;
    virtual uint32_t ReadDWord(uint64_t address) override;
    virtual uint64_t ReadQWord(uint64_t address) override;

    virtual void WriteByte(uint64_t address, uint8_t data) override;
    virtual void WriteWord(uint64_t address, uint16_t data) override;
    virtual void WriteDWord(uint64_t address, uint32_t data) override;
    virtual void WriteQWord(uint64_t address, uint64_t data) override;

    void StartTransfer();

   private:
    void HandleCommand(StorageDeviceCommands command);

   private:
    MMU* m_PhysicalMMU;
    uint64_t m_command;
    StorageDeviceStatus m_status;
    uint64_t m_data;
    PhysicalRegionListBuffer* m_buffer;
    StorageFile m_file;
    struct TransferCommandStatus {
        uint64_t LBA;
        uint64_t Count;
        bool INT;
        bool write;
    } m_transferCommandStatus;
};

#endif /* _STORAGE_IO_DEVICE_HPP */