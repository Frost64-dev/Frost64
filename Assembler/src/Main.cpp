/*
Copyright (©) 2024-2025  Frosty515 & contributors

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

Authors:
- Frosty515
- KevinAlavik
*/

#include <cstdint>
#include <cstdio>
#include <cstring>

#include <Common/ArgsParser.hpp>
#include <Common/DataStructures/Buffer.hpp>

#include "Assembler.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "PreProcessor.hpp"

ArgsParser* g_args = nullptr;

int main(int argc, char** argv) {
    g_args = new ArgsParser();

    g_args->AddOption('p', "program", "Input program to assemble", true);
    g_args->AddOption('o', "output", "Output file", true);
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

    FILE* file = fopen(program.data(), "r");
    if (file == nullptr) {
        fprintf(stderr, "Error: could not open input file %s: \"%s\"\n", program.data(), strerror(errno));
        return 1;
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* fileContents = new uint8_t[fileSize];
    if (fread(fileContents, 1, fileSize, file) != fileSize) {
        fprintf(stderr, "Error: failed to read input file %s: \"%s\"\n", program.data(), strerror(errno));
        fclose(file);
        return 1;
    }
    fclose(file);

    PreProcessor preProcessor;
    preProcessor.process(reinterpret_cast<const char*>(fileContents), fileSize, program);
    size_t processedBufferSize = preProcessor.GetProcessedBufferSize();
    uint8_t* processedBufferData = new uint8_t[processedBufferSize];
    preProcessor.ExportProcessedBuffer(processedBufferData);
#ifdef ASSEMBLER_DEBUG
    preProcessor.GetReferencePoints().Enumerate([](PreProcessor::ReferencePoint* ref) {
        printf("Reference point: %s:%zu @ %zu\n", ref->fileName.c_str(), ref->line, ref->offset);
    });
#endif

    Lexer* lexer = new Lexer();
    lexer->tokenize(reinterpret_cast<const char*>(processedBufferData), processedBufferSize, preProcessor.GetReferencePoints());

    Parser parser;
    parser.SimplifyExpressions(lexer->GetTokens());
    parser.parse();
#ifdef ASSEMBLER_DEBUG
    parser.PrintSections(stdout);
    fflush(stdout);
#endif

    Assembler assembler;
    assembler.assemble(parser.GetLabels(), parser.GetBaseAddress());

    const Buffer& buffer = assembler.GetBuffer();
    size_t bufferSize = buffer.GetSize();
    uint8_t* bufferData = new uint8_t[bufferSize];
    buffer.Read(0, bufferData, bufferSize);

    FILE* outputFile = fopen(output.data(), "w");
    if (outputFile == nullptr) {
        fprintf(stderr, "Error: could not open output file %s: \"%s\"\n", output.data(), strerror(errno));
        return 1;
    }

    if (bufferSize != fwrite(bufferData, 1, bufferSize, outputFile)) {
        fprintf(stderr, "Error: could not write to output file %s: \"%s\"", output.data(), strerror(errno));
        return 1;
    }
    fclose(outputFile);

    delete[] fileContents;
    delete[] processedBufferData;
    delete[] bufferData;

    assembler.Clear();
    parser.Clear();
    lexer->Clear();

    delete lexer;
    return 0;
}