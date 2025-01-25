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

#include "SDLVideoBackend.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include <Emulator.hpp>

#include "IO/devices/Video/VideoBackend.hpp"

void EventHandler(void* data) {
    SDLVideoBackend* backend = static_cast<SDLVideoBackend*>(data);
    backend->EnterEventLoop();
}

SDLVideoBackend::SDLVideoBackend(const VideoMode& mode)
    : VideoBackend(mode), m_window(nullptr), m_renderer(nullptr), m_texture(nullptr), m_framebuffer(nullptr), m_eventThread(nullptr), m_renderThread(nullptr), m_renderAllowed(true), m_renderRunning(false), m_framebufferDirty(false) {
}

SDLVideoBackend::~SDLVideoBackend() {
}

void SDLVideoBackend::Init() {
    SDL_SetAppMetadata("Frost64 Emulator", "1.0-dev", "com.github.frost64-dev.frost64");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Failed to initialise SDL: %s\n", SDL_GetError());
        exit(1);
    }

    VideoMode mode = GetRawMode();

    if (!SDL_CreateWindowAndRenderer("Emulator", mode.width, mode.height, 0, &m_window, &m_renderer)) {
        printf("Failed to create window and renderer: %s\n", SDL_GetError());
        exit(1);
    }

    m_renderAllowed.store(true);

    m_renderThread = new std::thread(&SDLVideoBackend::RenderLoop, this);
    m_eventThread = new std::thread(EventHandler, this);

    while (!m_renderRunning.load())
        ;
}

void SDLVideoBackend::SetMode(VideoMode mode) {
    m_renderAllowed.store(false);
    while (m_renderRunning.load())
        ;

    m_renderThread->join();
    delete m_renderThread;

    SetRawMode(mode);

    SDL_DestroyTexture(m_texture);
    delete[] m_framebuffer;

    m_renderAllowed.store(true);
    m_renderThread = new std::thread(&SDLVideoBackend::RenderLoop, this);

    while (!m_renderRunning.load())
        ;
}

VideoMode SDLVideoBackend::GetMode() {
    return GetRawMode();
}

void SDLVideoBackend::Write(uint64_t offset, uint8_t* data, uint64_t size) {
    memcpy(m_framebuffer + offset, data, size);
    m_framebufferDirty.store(true);
}

void SDLVideoBackend::Read(uint64_t offset, uint8_t* data, uint64_t size) {
    memcpy(data, m_framebuffer + offset, size);
}

void SDLVideoBackend::EnterEventLoop() {
    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
                Emulator::Crash("User closed window");
            default:
                break;
            }
        }
    }
}

void SDLVideoBackend::Draw() {
    if (!m_framebufferDirty.load())
        return;

    VideoMode mode = GetRawMode();

    void* pixels;
    int pitch;

    if (SDL_LockTexture(m_texture, nullptr, &pixels, &pitch)) {
        memcpy(pixels, m_framebuffer, mode.width * mode.height * 4);
        SDL_UnlockTexture(m_texture);
        // SDL_RenderClear(m_renderer);
        SDL_RenderTexture(m_renderer, m_texture, nullptr, nullptr);
        SDL_RenderPresent(m_renderer);
        m_framebufferDirty.store(false);
    }
}

void SDLVideoBackend::RenderLoop() {
    VideoMode mode = GetRawMode();

    m_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, mode.width, mode.height);
    if (m_texture == nullptr) {
        printf("Failed to create texture: %s\n", SDL_GetError());
        exit(1);
    }

    m_framebuffer = new uint8_t[mode.width * mode.height * 4];
    memset(m_framebuffer, 0, mode.width * mode.height * 4);

    SDL_SetWindowSize(m_window, mode.width, mode.height);

    m_renderRunning.store(true);
    while (m_renderAllowed.load()) {
        Draw();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // roughly 60fps
    }
    m_renderRunning.store(false);
}
