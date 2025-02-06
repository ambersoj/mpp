#include <iostream>
#include <ctime>

extern "C" void execute(const std::string&) {
    time_t now = time(0);
    std::cout << "[TIME] " << ctime(&now);
}
