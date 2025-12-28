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

if (BUILD_ARCHITECTURE STREQUAL "x86_64")
    set(exec_sources

    )
endif ()

set(exec_sources
    ${exec_sources}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Exec.cpp
)

add_library(exec STATIC ${exec_sources})

set_target_properties(exec PROPERTIES CXX_STANDARD 23)
set_target_properties(exec PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(exec PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(exec PROPERTIES C_STANDARD 23)
set_target_properties(exec PROPERTIES C_EXTENSIONS OFF)
set_target_properties(exec PROPERTIES C_STANDARD_REQUIRED ON)

target_include_directories(exec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(exec PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include) # doing things a bit differently here due to conflicting system includes
target_include_directories(exec PUBLIC ${CMAKE_SOURCE_DIR}/LibCommon/include)

target_compile_options(exec
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

if (BUILD_CONFIG STREQUAL "Debug")
    target_compile_definitions(exec PRIVATE LIBEXEC_DEBUG=1)
endif()
