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

#include "XCBVideoBackend.hpp"

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xproto.h>

#include <cstring>
#include <Emulator.hpp>

xcb_format_t const* query_xcb_format_for_depth(xcb_connection_t* const m_connection, uint32_t depth) {
    xcb_setup_t const* const setup = xcb_get_setup(m_connection);
    xcb_format_iterator_t it;
    for (it = xcb_setup_pixmap_formats_iterator(setup); it.rem; xcb_format_next(&it)) {
        if (xcb_format_t const* const format = it.data; format->depth == depth)
            return format;
    }
    return nullptr;
}

shm_xcb_image_t* shm_xcb_image_create(xcb_connection_t* const m_connection, uint32_t width, uint32_t height, uint32_t depth) {
    xcb_generic_error_t* error = nullptr;

    shm_xcb_image_t* shm_xcb_image = static_cast<shm_xcb_image_t*>(calloc(1, sizeof(shm_xcb_image_t)));
    if (shm_xcb_image == nullptr) {
        return nullptr;
    }
    shm_xcb_image->connection = m_connection;

    xcb_format_t const *const fmt = query_xcb_format_for_depth(m_connection, depth);
    if (fmt == nullptr) {
        free(shm_xcb_image);
        return nullptr;
    }

    shm_xcb_image->image = xcb_image_create(width, height, XCB_IMAGE_FORMAT_Z_PIXMAP, fmt->scanline_pad, fmt->depth, fmt->bits_per_pixel, 0, XCB_IMAGE_ORDER_LSB_FIRST, XCB_IMAGE_ORDER_MSB_FIRST, nullptr, ~0, 0);
    if (shm_xcb_image->image == nullptr) {
        free(shm_xcb_image);
        return nullptr;
    }

    size_t const image_segment_size = shm_xcb_image->image->stride * shm_xcb_image->image->height;

    shm_xcb_image->shmid = shmget(IPC_PRIVATE, image_segment_size, IPC_CREAT | 0600);

    if (shm_xcb_image->shmid == -1) {
        xcb_image_destroy(shm_xcb_image->image);
        free(shm_xcb_image);
        return nullptr;
    }

    shm_xcb_image->image->data = static_cast<uint8_t*>(shmat(shm_xcb_image->shmid, nullptr, 0));
    if (shm_xcb_image->image->data == reinterpret_cast<uint8_t*>(-1)) {
        shmctl(shm_xcb_image->shmid, IPC_RMID, nullptr);
        xcb_image_destroy(shm_xcb_image->image);
        free(shm_xcb_image);
        return nullptr;
    }

    shm_xcb_image->shm_seg = xcb_generate_id(m_connection);

    error = xcb_request_check(m_connection, xcb_shm_attach_checked(m_connection, shm_xcb_image->shm_seg, shm_xcb_image->shmid, 0));

    if (error != nullptr) {
        shmdt(shm_xcb_image->image->data);
        shmctl(shm_xcb_image->shmid, IPC_RMID, nullptr);
        xcb_image_destroy(shm_xcb_image->image);
        free(shm_xcb_image);
        free(error);
        return nullptr;
    }
    free(error);

    return shm_xcb_image;
}

void shm_xcb_image_destroy(shm_xcb_image_t* const shm_xcb_image) {
    xcb_shm_detach(shm_xcb_image->connection, shm_xcb_image->shm_seg);
    shmdt(shm_xcb_image->image->data);
    shmctl(shm_xcb_image->shmid, IPC_RMID, nullptr);
    xcb_image_destroy(shm_xcb_image->image);
    free(shm_xcb_image);
}

struct XSizeHints {
    uint32_t flags;
    int32_t  x, y;
    int32_t  width, height;
    int32_t  min_width, min_height;
    int32_t  max_width, max_height;
    int32_t  width_inc, height_inc;
    int32_t  min_aspect_num, min_aspect_den;
    int32_t  max_aspect_num, max_aspect_den;
    int32_t  base_width, base_height;
    uint32_t win_gravity;
};

enum XSizeHintsFlag {
    X_SIZE_HINT_US_POSITION   = 1U << 0,
    X_SIZE_HINT_US_SIZE       = 1U << 1,
    X_SIZE_HINT_P_POSITION    = 1U << 2,
    X_SIZE_HINT_P_SIZE        = 1U << 3,
    X_SIZE_HINT_P_MIN_SIZE    = 1U << 4,
    X_SIZE_HINT_P_MAX_SIZE    = 1U << 5,
    X_SIZE_HINT_P_RESIZE_INC  = 1U << 6,
    X_SIZE_HINT_P_ASPECT      = 1U << 7,
    X_SIZE_HINT_BASE_SIZE     = 1U << 8,
    X_SIZE_HINT_P_WIN_GRAVITY = 1U << 9
};

void XCBBackend_Loop(void* data) {
    XCBVideoBackend* backend = static_cast<XCBVideoBackend*>(data);
    backend->EnterEventLoop();
}

XCBVideoBackend::XCBVideoBackend(const VideoMode& mode) : VideoBackend(mode), m_connection(nullptr), m_window(0), m_shm_xcb_image(nullptr), m_gc(0), m_framebuffer(nullptr), m_eventThread(nullptr), m_renderThread(nullptr), m_renderAllowed(true), m_renderRunning(false), m_framebufferDirty(false) {

}

XCBVideoBackend::~XCBVideoBackend() {

}

void XCBVideoBackend::Init() {
    m_connection = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(m_connection))
        Emulator::Crash("Failed to connect to X server via XCB");

    const xcb_query_extension_reply_t* extension = xcb_get_extension_data(m_connection, &xcb_shm_id);
    if (extension == nullptr || !extension->present) {
        xcb_disconnect(m_connection);
        Emulator::Crash("X server does not support the XCB SHM extension");
    }

    VideoMode mode = GetRawMode();
    if (mode.bpp != 32) {
        xcb_disconnect(m_connection);
        Emulator::Crash("XCB Video Backend only supports 32bpp modes");
    }

    m_shm_xcb_image = shm_xcb_image_create(m_connection, mode.width, mode.height, 24);
    if (m_shm_xcb_image == nullptr) {
        xcb_disconnect(m_connection);
        Emulator::Crash("Failed to create shared memory XCB image");
    }

    xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(m_connection)).data;

    uint32_t const mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t const values[] = {screen->white_pixel, XCB_EVENT_MASK_EXPOSURE};

    m_window = xcb_generate_id(m_connection);
    xcb_create_window(m_connection, XCB_COPY_FROM_PARENT, m_window, screen->root, 0, 0, mode.width, mode.height, 10, XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, values);

    m_gc = xcb_generate_id(m_connection);
    uint32_t const gc_mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
    uint32_t const gc_values[] = {screen->black_pixel, 0};
    xcb_create_gc(m_connection, m_gc, m_window, gc_mask, gc_values);

    xcb_map_window(m_connection, m_window);

    XSizeHints hints = {};
    hints.flags = X_SIZE_HINT_P_MIN_SIZE | X_SIZE_HINT_P_MAX_SIZE;
    hints.min_width = mode.width;
    hints.min_height = mode.height;
    hints.max_width = mode.width;
    hints.max_height = mode.height;

    xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_window, XCB_ATOM_WM_NORMAL_HINTS, XCB_ATOM_WM_SIZE_HINTS, 32, sizeof(XSizeHints) / 4, &hints);
    xcb_flush(m_connection);

    m_renderThread = new std::thread(&XCBVideoBackend::RenderLoop, this);
    m_eventThread = new std::thread(XCBBackend_Loop, this);
}

void XCBVideoBackend::SetMode(VideoMode mode) {
    m_renderAllowed.store(false);
    m_renderAllowed.notify_all();
    m_framebufferDirty.store(true);
    m_framebufferDirty.notify_all(); // wake the render thread if it's waiting
    while (m_renderRunning.load())
        ;

    m_renderThread->join();
    delete m_renderThread;

    SetRawMode(mode);

    if (mode.bpp != 32)
        Emulator::Crash("XCB Video Backend only supports 32bpp modes");

    shm_xcb_image_destroy(m_shm_xcb_image);

    uint32_t values[2] = {static_cast<uint32_t>(mode.width), static_cast<uint32_t>(mode.height)};
    xcb_configure_window(m_connection, m_window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);

    XSizeHints hints = {};
    hints.flags = X_SIZE_HINT_P_MIN_SIZE | X_SIZE_HINT_P_MAX_SIZE | X_SIZE_HINT_P_SIZE;
    hints.min_width = mode.width;
    hints.min_height = mode.height;
    hints.max_width = mode.width;
    hints.max_height = mode.height;

    xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_window, XCB_ATOM_WM_NORMAL_HINTS, XCB_ATOM_WM_SIZE_HINTS, 32, sizeof(XSizeHints) / 4, &hints);

    m_shm_xcb_image = shm_xcb_image_create(m_connection, mode.width, mode.height, 24);

    xcb_flush(m_connection);

    m_renderAllowed.store(true);
    m_renderThread = new std::thread(&XCBVideoBackend::RenderLoop, this);

    while (!m_renderRunning.load())
        ;
}

VideoMode XCBVideoBackend::GetMode() {
    return GetRawMode();
}

void XCBVideoBackend::Write(uint64_t offset, uint8_t* data, uint64_t size) {
    memcpy(m_shm_xcb_image->image->data + offset, data, size);
    m_framebufferDirty.store(true);
    m_framebufferDirty.notify_all();
}

void XCBVideoBackend::Read(uint64_t offset, uint8_t* data, uint64_t size) {
    memcpy(data, m_shm_xcb_image->image->data + offset, size);
}

void XCBVideoBackend::EnterEventLoop() {
    printf("Entering event loop\n");
    for (xcb_generic_event_t* event; (event = xcb_wait_for_event(m_connection)); free(event)) {
        switch (event->response_type & ~0x80) {
        case XCB_EXPOSE: {
            printf("expose event\n");
            Draw();
            break;
        }
        default:
            break;
        }
    }
}

void XCBVideoBackend::RenderLoop() {
    while (m_renderAllowed.load()) {
        if (!m_renderRunning.load()) {
            m_renderRunning.store(true);
            m_renderRunning.notify_all();
        }
        if (!m_framebufferDirty.load())
            m_framebufferDirty.wait(false);
        Draw();
        m_framebufferDirty.store(false);
    }
    fprintf(stderr, "exiting render loop\n");
    m_renderRunning.store(false);
    m_renderRunning.notify_all();
}

void XCBVideoBackend::Draw() {
    xcb_shm_put_image(m_connection, m_window, m_gc,
        m_shm_xcb_image->image->width, m_shm_xcb_image->image->height, 0, 0,
        m_shm_xcb_image->image->width, m_shm_xcb_image->image->height, 0, 0,
        m_shm_xcb_image->image->depth, m_shm_xcb_image->image->format, 0,
        m_shm_xcb_image->shm_seg, 0);

    xcb_flush(m_connection);
}
