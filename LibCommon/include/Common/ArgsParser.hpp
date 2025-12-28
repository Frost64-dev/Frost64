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

#ifndef _ARGS_PARSER_HPP
#define _ARGS_PARSER_HPP

#include <vector>
#include <unordered_map>
#include <string_view>
#include <string>

class ArgsParser {
public:
    ArgsParser();
    ~ArgsParser();

    void ParseArgs(int argc, char** argv);

    void AddOption(char short_name, const char* option, const char* description, bool required = false, bool hasParameter = true);

    std::string_view GetOption(char short_name);
    std::string_view GetOption(const char* option);

    bool HasOption(char short_name) const;
    bool HasOption(const char* option) const;

    const std::string& GetHelpMessage() const;

private:
    struct Option {
        char short_name;
        const char* option;
        const char* description;
        bool required;
        bool hasParameter;
    };

    std::vector<Option> m_options;
    std::unordered_map<char, std::string_view> m_parsed_options;
    std::unordered_map<std::string_view, std::string_view> m_parsed_options_long;

    mutable std::string m_helpMessage;
    bool m_helpMessageInitialised;

    char* m_programName;
};

#endif /* _ARGS_PARSER_HPP */