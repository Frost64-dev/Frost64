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
    set(assembler_sources
        ${CMAKE_CURRENT_SOURCE_DIR}/src/arch/x86_64/util.asm
    )
endif()

set(CMAKE_REQUIRED_FLAGS "-std=c++20")

include(CheckSourceCompiles)
check_source_compiles(CXX
"#include <format>
#include <string>

int main(int argc, char**) {
    std::string str = std::format(\"This is a test, argc = {}\", argc);
    return 0;
}"
HAVE_CXX20_FORMAT)

unset(CMAKE_REQUIRED_FLAGS)

set(assembler_sources
    ${assembler_sources}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Assembler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Lexer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PreProcessor.cpp
)

add_executable(Assembler ${assembler_sources})

set_target_properties(Assembler PROPERTIES CXX_STANDARD 23)
set_target_properties(Assembler PROPERTIES CXX_STANDARD_REQUIRED ON)
set_target_properties(Assembler PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(Assembler PROPERTIES C_STANDARD 23)
set_target_properties(Assembler PROPERTIES C_EXTENSIONS OFF)
set_target_properties(Assembler PROPERTIES C_STANDARD_REQUIRED ON)

target_include_directories(Assembler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(Assembler PUBLIC ${CMAKE_SOURCE_DIR}/LibArch/include)
target_include_directories(Assembler PUBLIC ${CMAKE_SOURCE_DIR}/LibCommon/include)

target_compile_options(Assembler
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

target_link_libraries(Assembler PRIVATE arch common)

if (BUILD_CONFIG STREQUAL "Debug")
   target_compile_definitions(Assembler PRIVATE ASSEMBLER_DEBUG=1)
endif()

if (HAVE_CXX20_FORMAT EQUAL 1)
    target_compile_definitions(Assembler PRIVATE ENABLE_CXX20_FORMAT=1)
endif()

add_custom_target(RunAssembler
    COMMAND $<TARGET_FILE:Assembler>
    DEPENDS Assembler
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS Assembler DESTINATION ${CMAKE_SOURCE_DIR}/bin)
