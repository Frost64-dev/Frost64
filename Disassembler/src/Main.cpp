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

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include <Common/ArgsParser.hpp>

#include "Disassembler.hpp"
#include "FileBuffer.hpp"

FILE* g_out_file = nullptr;

int main(int argc, char** argv) {
    ArgsParser args_parser;
    args_parser.AddOption('p', "program", "Path to the program to disassemble", true);
    args_parser.AddOption('o', "output", "Path to the output file", true);
    args_parser.AddOption('h', "help", "Show this help message", false, false);
    args_parser.ParseArgs(argc, argv);

    if (args_parser.HasOption('h')) {
        fputs(args_parser.GetHelpMessage().c_str(), stdout);
        return 0;
    }

    if (!args_parser.HasOption('p') || !args_parser.HasOption('o')) {
        fputs(args_parser.GetHelpMessage().c_str(), stdout);
        return 1;
    }

    const char* program_path = args_parser.GetOption('p').data();
    const char* output_path = args_parser.GetOption('o').data();
    FILE* file = fopen(program_path, "rb");

    if (file == nullptr) {
        perror("Failed to open program file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    g_out_file = fopen(output_path, "w");

    if (g_out_file == nullptr) {
        perror("Failed to open output file");
        fclose(file);
        return 1;
    }

    FileBuffer file_buffer(file, file_size);
    Disassembler disassembler(file_buffer);
    disassembler.Disassemble([&](){
        const std::vector<std::string>& instructions = disassembler.GetInstructions();
        for (auto& ins : instructions)
            fprintf(g_out_file, "%s\n", ins.c_str());

        fclose(g_out_file);
        fclose(file);
    });

    const std::vector<std::string>& instructions = disassembler.GetInstructions();

    for (auto& ins : instructions)
        fprintf(g_out_file, "%s\n", ins.c_str());

    fclose(g_out_file);
    fclose(file);
    return 0;
}