/*
Copyright (©) 2024-2025  Frosty515

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

#include <cstdint>
#include <cstdio>
#include <cstring>

#include <common/ArgsParser.hpp>

#include <common/Data-structures/Buffer.hpp>

#include "Assembler.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "PreProcessor.hpp"

ArgsParser* g_args = nullptr;

int main(int argc, char** argv) {
    g_args = new ArgsParser();

    g_args->AddOption('p', "program", "Input program to assemble", true);
    g_args->AddOption('o', "output", "Output file", true);
    g_args->AddOption('h', "help", "Print this help message", false);

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

    FILE* file = fopen(program.data(), "r");
    if (file == nullptr) {
        printf("Error: could not open file %s: %s\n", program.data(), strerror(errno));
        return 1;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* file_contents = new uint8_t[fileSize];
    if (fread(file_contents, 1, fileSize, file) != fileSize) {
        printf("Error: failed to read file %s: %s\n", program.data(), strerror(errno));
        fclose(file);
        return 1;
    }
    fclose(file);

    PreProcessor pre_processor;
    pre_processor.process(reinterpret_cast<const char*>(file_contents), fileSize, program);
    size_t processed_buffer_size = pre_processor.GetProcessedBufferSize();
    uint8_t* processed_buffer_data = new uint8_t[processed_buffer_size];
    pre_processor.ExportProcessedBuffer(processed_buffer_data);
#ifdef ASSEMBLER_DEBUG
    pre_processor.GetReferencePoints().Enumerate([](PreProcessor::ReferencePoint* ref) {
        printf("Reference point: %s:%zu @ %zu\n", ref->file_name.c_str(), ref->line, ref->offset);
    });
#endif

    Lexer* lexer = new Lexer();
    lexer->tokenize(reinterpret_cast<const char*>(processed_buffer_data), processed_buffer_size, pre_processor.GetReferencePoints());

    Parser parser;
    parser.parse(lexer->GetTokens());
#ifdef ASSEMBLER_DEBUG
    parser.PrintSections(stdout);
    fflush(stdout);
#endif

    Assembler assembler;
    assembler.assemble(parser.GetLabels(), parser.GetBaseAddress());

    const Buffer& buffer = assembler.GetBuffer();
    size_t buffer_size = buffer.GetSize();
    uint8_t* buffer_data = new uint8_t[buffer_size];
    buffer.Read(0, buffer_data, buffer_size);

    FILE* output_file = fopen(output.data(), "w");
    if (output_file == nullptr) {
        printf("Error: could not open output file %s: %s\n", output.data(), strerror(errno));
        return 1;
    }

    if (buffer_size != fwrite(buffer_data, 1, buffer_size, output_file)) {
        printf("Error: could not write to output file %s: %s", output.data(), strerror(errno));
        return 1;
    }
    fclose(output_file);

    delete[] file_contents;
    delete[] processed_buffer_data;
    delete[] buffer_data;

    assembler.Clear();
    parser.Clear();
    lexer->Clear();

    delete lexer;
    return 0;
}