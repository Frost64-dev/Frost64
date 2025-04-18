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

#ifndef _VIDEO_DEVICE_BACKEND_HPP
#define _VIDEO_DEVICE_BACKEND_HPP

#include <stdint.h>

#include "VideoDevice.hpp"

enum class VideoBackendType {
    NONE,
    SDL
};

class VideoBackend {
   public:
    explicit VideoBackend(const VideoMode& mode = NATIVE_VIDEO_MODE);

    virtual ~VideoBackend();

    virtual void Init() = 0;
    virtual void SetMode(VideoMode mode) = 0;
    virtual VideoMode GetMode() = 0;

    virtual void Write(uint64_t offset, uint8_t* data, uint64_t size) = 0;
    virtual void Read(uint64_t offset, uint8_t* data, uint64_t size) = 0;

   protected:
    VideoMode GetRawMode();
    void SetRawMode(const VideoMode& mode);

   private:
    VideoMode m_mode;
};

#endif /* _VIDEO_DEVICE_BACKEND_HPP */