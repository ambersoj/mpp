#include <iostream>
#include "plugin.h"

extern "C" {
    void execute(const std::string& args) {
        std::cout << "[START] Starting packet capture...\n";
        // TODO: Send command to `mpp_net` to start capture
    }

    std::string getHelp() {
        return "start - Begins packet capture.\nUsage: start";
    }
}
