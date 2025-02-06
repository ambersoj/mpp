#include <iostream>
#include <string>

extern "C" void execute(const std::string& args) {
    std::cout << "[ECHO] " << args << std::endl;
}
