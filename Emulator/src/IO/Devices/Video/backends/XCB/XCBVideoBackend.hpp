/*
Copyright (©) 2025  Frosty515

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

#ifndef _XCB_VIDEO_BACKEND_HPP
#define _XCB_VIDEO_BACKEND_HPP

#include <thread>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_image.h>

#include "../../VideoBackend.hpp"

struct shm_xcb_image_t {
    xcb_connection_t* connection;
    xcb_image_t* image;
    xcb_shm_seg_t shm_seg;
    int shmid;
};

class XCBVideoBackend : public VideoBackend {
   public:
    explicit XCBVideoBackend(const VideoMode& mode = NATIVE_VIDEO_MODE);
    ~XCBVideoBackend() override;

    void Init() override;
    void SetMode(VideoMode mode) override;
    VideoMode GetMode() override;

    void Write(uint64_t offset, uint8_t* data, uint64_t size) override;
    void Read(uint64_t offset, uint8_t* data, uint64_t size) override;

    void EnterEventLoop();
    void RenderLoop();

   private:
    void Draw();

   private:
    xcb_connection_t* m_connection;
    xcb_drawable_t m_window;
    shm_xcb_image_t* m_shm_xcb_image;
    xcb_gcontext_t m_gc;

    uint8_t* m_framebuffer;

    std::thread* m_eventThread;
    std::thread* m_renderThread;

    std::atomic_bool m_renderAllowed;
    std::atomic_bool m_renderRunning;
    std::atomic_bool m_framebufferDirty;
};

#endif /* _XCB_VIDEO_BACKEND_HPP */