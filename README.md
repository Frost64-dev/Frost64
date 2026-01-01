# Frost64 - A Custom 64-bit Architecture

## COPYING

Copyright (©) 2024-2026  Frosty515

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

## Design

- 64-bit architecture
- Full design can be found in [docs/design.md](docs/design.md)
- Design of some internal aspects can be found in [docs/internals.md](docs/internals.md)

## Pre-requisites

### Requirements

- CMake
- Ninja
- A C/C++ compiler that supports C23 and C++23
- SDL3 if the SDL video backend is to be used
- xcb, xcb-image, and xcb-shm if the XCB video backend is to be used

### Linux (GNU C/C++)

#### Debian

- run `sudo apt update && sudo apt install build-essential cmake ninja-build` to install dependencies

#### Fedora/RHEL

- run `sudo dnf install gcc gcc-c++ cmake ninja-build` to install dependencies

#### Arch

- run `sudo pacman -Syu base-devel cmake ninja` to install dependencies

#### Gentoo

- run `sudo emerge --ask --verbose sys-devel/gcc sys-devel/binutils dev-build/cmake dev-build/ninja` to install dependencies

---

## Building

1. run `mkdir build && cd build` to create a build directory and enter it
2. run `cmake -GNinja ..` to generate the build files. The following options can be added:
   - `-DBUILD_CONFIG=<config>`, where `<config>` can be `Debug` or `Release`. It defaults to `Release`.
   - `-DBUILD_ARCHITECTURE=<arch>`, where `<arch>` is the architecture that it is being built for.  Currently, the only supported architecture is `x86_64`, which is the default.
   - `-DVIDEO_BACKENDS=<backends>`, where `<backends>` is a comma-separated list of video backends to build. Both `XCB` and `SDL` backends are supported. It defaults to `None`. Note that the SDL backend is very experimental and that if the XCB backend is used, the XCB dependencies must be installed, cmake will not detect there existence, and compiling will fail.
3. run `ninja install` to build and install to the src directory. The binaries will be in the `bin` directory in the src directory.

## Running the Assembler

- In the source directory, run `./bin/Assembler < -p path/to/assembly > < -o path/to/binary >` to assemble the assembly file. Note: The Assembler supports outputting either flat binary files or ELF files. 
- For more options, run `./bin/Assembler --help` to see the available options.

## Running the Emulator

- In the source directory, run `./bin/Emulator < -p path/to/binary > [ -m RAM size ]` to run the emulator.
- The RAM size is optional and defaults to 1 MiB.
- For more options, run `./bin/Emulator --help` to see the available options.

## Notes

- The assembler and emulator are still in development and may not work as expected. Please report any issues you find.

### Current todo list (in no particular order)

- make fancy docs
- add better macros
- firmware spec
- start writing firmware
- llvm backend
- make examples
- custom fs
- custom executable format
- more devices