#include <iostream>
#include "plugin.h"

extern "C" {
    void execute(const std::string& args) {
        std::cout << "[FILTER] Applying filter: " << args << "\n";
        // TODO: Send command to `mpp_net` to apply a BPF filter
    }

    std::string getHelp() {
        return "filter - Applies a packet filter.\nUsage: filter <expression>";
    }
}
