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

#ifndef _VIDEO_IO_DEVICE_HPP
#define _VIDEO_IO_DEVICE_HPP

#include <stdint.h>
#include <vector>

#include <MMU/MMU.hpp>

#include <IO/IODevice.hpp>

#include "VideoMemoryRegion.hpp"

struct VideoMode {
    uint64_t width;
    uint64_t height;
    uint64_t refreshRate;
    uint8_t bpp;
    uint64_t pitch;
};

#define NATIVE_VIDEO_MODE {1024, 768, 60, 32, 4096}

class VideoBackend;
enum class VideoBackendType;

enum class VideoDeviceCommands {
    INITIALISE = 0,
    GET_SCREEN_INFO = 1,
    GET_MODE = 2,
    SET_MODE = 3
};

enum class VideoDevicePorts {
    COMMAND = 0,
    DATA = 1,
    STATUS = 2
};

namespace VideoCommand {
    struct [[gnu::packed]] GetScreenInfoResponse {
        uint32_t width;
        uint32_t height;
        uint16_t hz;
        uint16_t bpp;
        uint16_t modes;
        uint16_t current_mode;
    };

    struct [[gnu::packed]] GetModeRequest {
        uint64_t address;
        uint16_t index;
    };

    struct [[gnu::packed]] GetModeResponse {
        uint32_t width;
        uint32_t height;
        uint16_t bpp;
        uint32_t pitch;
        uint16_t hz;
    };

    struct [[gnu::packed]] SetModeRequest {
        uint64_t address;
        uint16_t mode;
        uint8_t reserved[6];
    };
}

class VideoDevice : public IODevice {
public:
    VideoDevice(VideoBackendType backendType, MMU& mmu);
    virtual ~VideoDevice();

    virtual void Init();

    virtual uint8_t ReadByte(uint64_t address) override;
    virtual uint16_t ReadWord(uint64_t address) override;
    virtual uint32_t ReadDWord(uint64_t address) override;
    virtual uint64_t ReadQWord(uint64_t address) override;

    virtual void WriteByte(uint64_t address, uint8_t data) override;
    virtual void WriteWord(uint64_t address, uint16_t data) override;
    virtual void WriteDWord(uint64_t address, uint32_t data) override;
    virtual void WriteQWord(uint64_t address, uint64_t data) override;

    void HandleMemoryOperation(bool write, uint64_t address, uint8_t* buffer, uint64_t size);

    VideoBackendType GetBackendType();
    VideoBackend* GetBackend();

    bool isInitialised();

private:
    void HandleCommand();

private:
    VideoMemoryRegion* m_memoryRegion;
    VideoBackendType m_backendType;
    VideoBackend* m_backend;
    MMU& m_mmu;

    uint64_t m_command;
    uint64_t m_data;
    uint64_t m_status;

    bool m_initialised;

    VideoMode m_currentMode;
    uint64_t m_currentModeIndex;

    std::vector<VideoMode> m_modes;

    void* m_MemoryOverrideData;
};

#endif /* _VIDEO_IO_DEVICE_HPP */