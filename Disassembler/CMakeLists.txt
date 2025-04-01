# Copyright (©) 2024-2025  Frosty515

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

if(BUILD_ARCHITECTURE STREQUAL "x86_64")
    set(disassembler_sources
        
    )
endif()

set(disassembler_sources
    ${disassembler_sources}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Disassembler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileBuffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp
)

add_executable(Disassembler ${disassembler_sources})

set_target_properties(Disassembler PROPERTIES CXX_STANDARD 23)
set_target_properties(Disassembler PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(Disassembler PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(Disassembler PROPERTIES C_STANDARD 23)
set_target_properties(Disassembler PROPERTIES C_EXTENSIONS OFF)
set_target_properties(Disassembler PROPERTIES C_STANDARD_REQUIRED ON)

target_include_directories(Disassembler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(Disassembler PUBLIC ${CMAKE_SOURCE_DIR}/LibArch/include)
target_include_directories(Disassembler PUBLIC ${CMAKE_SOURCE_DIR}/LibCommon/include)

target_compile_options(Disassembler
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-g>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wall>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wextra>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wpedantic>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wno-packed-bitfield-compat>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fstack-protector>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-O3>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-omit-frame-pointer>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
)

target_link_libraries(Disassembler PRIVATE arch common)

if (BUILD_CONFIG STREQUAL "Debug")
   target_compile_definitions(Disassembler PRIVATE ASSEMBLER_DEBUG=1)
endif()

add_custom_target(RunDisassembler
    COMMAND $<TARGET_FILE:Disassembler>
    DEPENDS Disassembler
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS Disassembler DESTINATION ${CMAKE_SOURCE_DIR}/bin)
