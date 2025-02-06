#include <iostream>
#include "plugin.h"

extern "C" {
    void execute(const std::string& args) {
        std::cout << "[TX] Transmitting packet: " << args << "\n";
        // TODO: Parse args & send a real packet via `mpp_net`
    }

    std::string getHelp() {
        return "tx - Sends a raw packet.\nUsage: tx <dst> <src> <payload>";
    }
}
