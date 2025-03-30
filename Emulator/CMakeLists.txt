# Copyright (©) 2024-2025  Frosty515 & contributors

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# Authors
#   - Frosty515 <frosty515yt@gmail.com>
#   - fwpirate <alavik@disroot.org>

if (ENABLE_SDL STREQUAL "ON")
    message(WARNING "SDL3 is enabled, this is very experimental and may not work as expected.")
    find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
endif ()

if (BUILD_ARCHITECTURE STREQUAL "x86_64")
    set(emulator_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arch/x86_64/ALUInstruction.asm
    )
endif ()

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(emulator_sources
        ${emulator_sources}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/OSSpecific/Linux/File.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/OSSpecific/Linux/Memory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/OSSpecific/Linux/Network.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/OSSpecific/Linux/Signal.cpp
    )
else (CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(emulator_sources
        ${emulator_sources}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/OSSpecific/Darwin/Memory.cpp
    )
endif ()

set(emulator_sources
    ${emulator_sources}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Instruction/Instruction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Instruction/InstructionBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Instruction/Operand.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IO/IOBus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IO/IOInterfaceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IO/IOMemoryRegion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IO/devices/ConsoleDevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IO/devices/Storage/PhysicalRegionListBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IO/devices/Storage/StorageDevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IO/devices/Storage/StorageFile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IO/devices/Video/VideoBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IO/devices/Video/VideoDevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/IO/devices/Video/VideoMemoryRegion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MMU/BIOSMemoryRegion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MMU/MemoryRegion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MMU/MMU.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MMU/StandardMemoryRegion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MMU/VirtualMMU.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DebugInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Emulator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Exceptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Interrupts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Register.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Stack.cpp
)

if (NOT DEFINED ENABLE_SDL)
    set(ENABLE_SDL "OFF")
endif ()

if (ENABLE_SDL STREQUAL "ON")
    set(emulator_sources
        ${emulator_sources}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/IO/devices/Video/backends/SDL/SDLVideoBackend.cpp
    )
endif ()

add_executable(Emulator ${emulator_sources})

set_target_properties(Emulator PROPERTIES CXX_STANDARD 23)
set_target_properties(Emulator PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(Emulator PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(Emulator PROPERTIES C_STANDARD 23)
set_target_properties(Emulator PROPERTIES C_EXTENSIONS OFF)
set_target_properties(Emulator PROPERTIES C_STANDARD_REQUIRED ON)

target_include_directories(Emulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(Emulator PUBLIC ${CMAKE_SOURCE_DIR}/LibArch/include)


if (BUILD_CONFIG STREQUAL "Debug")
    target_compile_definitions(Emulator PRIVATE EMULATOR_DEBUG=1)
endif ()

if (ENABLE_SDL STREQUAL "ON")
    target_link_libraries(Emulator PRIVATE arch common SDL3::SDL3-shared)
    target_compile_definitions(Emulator PRIVATE ENABLE_SDL=1)
else ()
    target_link_libraries(Emulator PRIVATE arch common)
endif ()

target_compile_options(Emulator
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-g>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wall>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wextra>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wpedantic>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wno-packed-bitfield-compat>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fstack-protector>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-O1>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-omit-frame-pointer>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    # PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fsanitize=undefined,address>
)

# target_link_options(Emulator
#     PRIVATE $<$<LINK_LANGUAGE:CXX,C>:-fsanitize=undefined,address>
# )

install(TARGETS Emulator DESTINATION ${CMAKE_SOURCE_DIR}/bin)
