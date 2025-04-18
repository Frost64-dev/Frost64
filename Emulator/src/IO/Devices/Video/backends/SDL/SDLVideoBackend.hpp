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

#ifndef _SDL_VIDEO_BACKEND_HPP
#define _SDL_VIDEO_BACKEND_HPP

#include <SDL3/SDL.h>

#include <thread>

#include "../../VideoBackend.hpp"

class SDLVideoBackend : public VideoBackend {
   public:
    explicit SDLVideoBackend(const VideoMode& mode = NATIVE_VIDEO_MODE);
    ~SDLVideoBackend() override;

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
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    SDL_Texture* m_texture;

    uint8_t* m_framebuffer;

    std::thread* m_eventThread;
    std::thread* m_renderThread;

    std::atomic_bool m_renderAllowed;
    std::atomic_bool m_renderRunning;
    std::atomic_bool m_framebufferDirty;
};

#endif /* _SDL_VIDEO_BACKEND_HPP */