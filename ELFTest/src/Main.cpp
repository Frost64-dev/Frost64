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
#include <cstdint>
#include <cstdio>
#include <string_view>

#include <Common/ArgsParser.hpp>

#include <LibExec/elf.h>
#include <LibExec/Exec.hpp>

ArgsParser* g_args = nullptr;

int main(int argc, char** argv) {
    g_args = new ArgsParser();

    g_args->AddOption('p', "program", "Input program to convert", true);
    g_args->AddOption('o', "output", "Output ELF file", true);
    g_args->AddOption('h', "help", "Print this help message", false, false);

    g_args->ParseArgs(argc, argv);

    if (g_args->HasOption('h')) {
        printf("%s", g_args->GetHelpMessage().c_str());
        return 0;
    }

    if (!g_args->HasOption('p') || !g_args->HasOption('o')) {
        printf("%s", g_args->GetHelpMessage().c_str());
        return 1;
    }

    std::string_view program = g_args->GetOption('p');
    std::string_view output = g_args->GetOption('o');

    FILE* file = fopen(program.data(), "rb");
    if (file == nullptr) {
        fprintf(stderr, "Error: could not open input file %s\n", program.data());
        return 1;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* fileContents = new uint8_t[fileSize];
    if (fread(fileContents, 1, fileSize, file) != fileSize) {
        fprintf(stderr, "Error: failed to read input file %s\n", program.data());
        fclose(file);
        return 1;
    }

    fclose(file);

    ELFExecutable elf;
    if (!elf.Create()) {
        fprintf(stderr, "Error: failed to create ELF executable\n");
        return 1;
    }

    ELFProgramSection* textSection = elf.CreateNewProgramSection();
    textSection->SetType(PT_LOAD);
    textSection->SetFlags(PF_R | PF_X);
    textSection->SetVirtAddr(0xF0000000);
    textSection->SetPhysAddr(0xF0000000);
    textSection->SetAlignment(0x1000);
    textSection->SetData(fileContents, fileSize);

    ELFSection* textSecHeader = elf.CreateNewSection();
    textSecHeader->SetName(".text");
    textSecHeader->SetType(SHT_PROGBITS);
    textSecHeader->SetFlags(SHF_ALLOC | SHF_EXECINSTR);
    textSecHeader->SetRegion(0xF0000000, fileSize, 0x1000);
    textSecHeader->SetProgSection(textSection);

    elf.SetEntryPoint(0xF0000000);

    if (!elf.WriteToFile(output.data())) {
        fprintf(stderr, "Error: failed to write ELF executable to file %s\n", output.data());
        return 1;
    }

    return 0;
}