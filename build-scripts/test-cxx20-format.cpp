#include <format>
#include <iostream>
#include <string>

int main(int argc, char**) {
    std::string str = std::format("This is a test, argc = {}", argc);
    std::cout << str;
    return 0;
}
