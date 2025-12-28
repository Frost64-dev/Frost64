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

#include <ArgsParser.hpp>

#include <cstring>

#include <string>

ArgsParser::ArgsParser() : m_helpMessageInitialised(false), m_programName(nullptr) {
}

ArgsParser::~ArgsParser() {
    
}

void ArgsParser::ParseArgs(int argc, char** argv) {
    m_programName = argv[0];
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (Option& opt : m_options) {
                if (opt.short_name == argv[i][1]) {
                    if (opt.hasParameter) {
                        if (argv[i][2] != '\0') {
                            m_parsed_options[opt.short_name] = &argv[i][2];
                            m_parsed_options_long[opt.option] = argv[i];
                        } else if (i + 1 < argc) {
                            m_parsed_options[opt.short_name] = argv[i + 1];
                            m_parsed_options_long[opt.option] = argv[i + 1];
                            i++;
                        }
                    } else if (argv[i][2] == '\0') {
                        m_parsed_options[opt.short_name] = argv[i];
                        m_parsed_options_long[opt.option] = argv[i];
                    }
                }
                else if (argv[i][1] == '-' && strcmp(opt.option, &argv[i][2]) == 0) {
                    fprintf(stderr, "Parsing extended arg");
                    if (opt.hasParameter) {
                        if (i + 1 < argc) {
                            if (opt.short_name != '\0')
                                m_parsed_options[opt.short_name] = argv[i + 1];
                            m_parsed_options_long[opt.option] = argv[i + 1];
                            i++;
                        }
                    } else {
                        m_parsed_options[opt.short_name] = argv[i];
                        m_parsed_options_long[opt.option] = argv[i];
                    }
                }
            }
        }
    }
}

void ArgsParser::AddOption(char short_name, const char* option, const char* description, bool required, bool hasParameter) {
    Option opt = {short_name, option, description, required, hasParameter};
    m_options.push_back(opt);
}

std::string_view ArgsParser::GetOption(char short_name) {
    return m_parsed_options[short_name];
}

std::string_view ArgsParser::GetOption(const char* option) {
    return m_parsed_options_long[option];
}

bool ArgsParser::HasOption(char short_name) const {
    return m_parsed_options.contains(short_name);
}

bool ArgsParser::HasOption(const char* option) const {
    return m_parsed_options_long.contains(option);
}

const std::string& ArgsParser::GetHelpMessage() const {
    if (!m_helpMessageInitialised) {
        m_helpMessage += "Usage: ";
        m_helpMessage += m_programName;
        m_helpMessage += " ";
        m_helpMessage += "[options]\n";
        m_helpMessage += "Options:\n";
        for (const Option& opt : m_options) {
            if (opt.short_name != '\0') {
                m_helpMessage += "-";
                m_helpMessage += opt.short_name;
            } else
                m_helpMessage += "  ";
            m_helpMessage += " --";
            m_helpMessage += opt.option;
            m_helpMessage += "  ";
            m_helpMessage += opt.description;
            if (opt.required)
                m_helpMessage += " (required)";
            m_helpMessage += "\n";
        }
    }
    return m_helpMessage;
}