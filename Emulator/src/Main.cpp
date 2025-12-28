/*
Copyright (©) 2023-2025  Frosty515

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

#include <Common/ArgsParser.hpp>
#include <Common/Util.hpp>

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <Emulator.hpp>
#include <IO/Devices/Video/VideoBackend.hpp>

#define MAX_PROGRAM_FILE_SIZE 0x1000'0000
#define MIN_PROGRAM_FILE_SIZE 1

#define DEFAULT_RAM MiB(1)

#if defined(ENABLE_SDL) && defined(ENABLE_XCB)
    #define DISPLAY_HELP_TEXT R"(Display mode. Valid values are "sdl", "xcb", or "none" (case insensitive).)"
#define DISPLAY_TYPE_CHECKS if (display == "sdl") displayType = VideoBackendType::SDL; else if (display == "xcb") displayType = VideoBackendType::XCB;
#elif defined(ENABLE_SDL)
    #define DISPLAY_HELP_TEXT R"(Display mode. Valid values are "sdl" or "none" (case insensitive).)"
#define DISPLAY_TYPE_CHECKS if (display == "sdl") displayType = VideoBackendType::SDL;
#elif defined(ENABLE_XCB)
    #define DISPLAY_HELP_TEXT R"(Display mode. Valid values are "xcb" or "none" (case insensitive).)"
#define DISPLAY_TYPE_CHECKS if (display == "xcb") displayType = VideoBackendType::XCB;
#else
    #define DISPLAY_HELP_TEXT R"(Display mode. Valid value is "none" (case insensitive).)"
#define DISPLAY_TYPE_CHECKS if constexpr (false) ;
#endif

ArgsParser* g_args = nullptr;
int main(int argc, char** argv) {
    // Setup program arguments
    g_args = new ArgsParser();
    g_args->AddOption('p', "program", "Program file to run", true);
    g_args->AddOption('m', "ram", "RAM size in bytes", false);
    g_args->AddOption('d', "display", DISPLAY_HELP_TEXT, false);
    g_args->AddOption('D', "drive", "File to use as a storage drive.", false);
    g_args->AddOption('c', "console", R"(Console device location. Valid values are "stdio", "file:<path>", or "port:<port>" (case insensitive).)", false);
    g_args->AddOption(0, "debug", R"(Debug console location. Valid values are "disabled", "stdio", "file:<path>", or "port:<port>" (case insensitive). Default is "disabled".)", false);
    g_args->AddOption('h', "help", "Print this help message", false, false);

    g_args->ParseArgs(argc, argv);

    // Handle special arguments
    if (g_args->HasOption('h')) {
        printf("%s", g_args->GetHelpMessage().c_str());
        return 0;
    }

    if (!g_args->HasOption('p')) {
        printf("%s", g_args->GetHelpMessage().c_str());
        return 1;
    }

    // Handle emulation arguments
    std::string_view program = g_args->GetOption('p');

    size_t ramSize;
    if (g_args->HasOption('m'))
        ramSize = strtoull(g_args->GetOption('m').data(), nullptr, 0); // automatically detects base
    else
        ramSize = DEFAULT_RAM;

    // Open and read program file
    FILE* fp = fopen(program.data(), "r");
    if (fp == nullptr) {
        fprintf(stderr, "Error: could not open file %s: %s\n", program.data(), strerror(errno));
        return 1;
    }

    int rc = fseek(fp, 0, SEEK_END);
    if (rc != 0) {
        fprintf(stderr, "Error: could not seek end of file %s: %s\n", program.data(), strerror(errno));
        return 1;
    }

    size_t fileSize = ftell(fp);
    if (fileSize < MIN_PROGRAM_FILE_SIZE) {
        printf("File is too small to be a valid program.\n");
        return 1;
    }

    if (fileSize > MAX_PROGRAM_FILE_SIZE) {
        printf("File is too large to be a valid program.\n");
        return 1;
    }

    rc = fseek(fp, 0, SEEK_SET);
    if (rc != 0) {
        fprintf(stderr, "Error: could not seek start of file %s: %s\n", program.data(), strerror(errno));
        return 1;
    }

    uint8_t* data = new uint8_t[fileSize];
    size_t bytesRead = fread(data, 1, fileSize, fp);
    if (bytesRead != fileSize) {
        fprintf(stderr, "Error: failed to read file %s: %s\n", program.data(), strerror(errno));
        delete[] data;
        return 1;
    }

    fclose(fp);

    // Get the display type
    VideoBackendType displayType = VideoBackendType::NONE;
    bool hasDisplay = g_args->HasOption('d');
    if (hasDisplay) {
        std::string_view rawDisplay = g_args->GetOption('d');
        std::string display;

        // Convert display to lowercase
        for (char c : rawDisplay) {
            display += std::tolower(static_cast<unsigned char>(c));
        }

        DISPLAY_TYPE_CHECKS
        else if (display == "none")
            displayType = VideoBackendType::NONE;
        else {
            fprintf(stderr, "Error: Invalid display type: %s\n", display.c_str());
            return 1;
        }
    }

    std::string_view drive;
    bool hasDrive = g_args->HasOption('D');
    if (hasDrive)
        drive = g_args->GetOption('D');

    // Get the console type
    std::string_view console = "stdio";
    if (g_args->HasOption('c'))
        console = g_args->GetOption('c');

    // Get the debug console type
    std::string_view debug = "disabled";
    if (g_args->HasOption("debug"))
        debug = g_args->GetOption("debug");

    // Delete the args parser
    delete g_args;

    // Actually start emulator
    if (int status = Emulator::Start(data, fileSize, ramSize, console, debug, hasDisplay, displayType, hasDrive, drive.data()); status != 0) {
        fprintf(stderr, "Error: Emulator failed to start: %d\n", status);
        return 1;
    }

    // Cleanup
    delete[] data;

    return 0;
}