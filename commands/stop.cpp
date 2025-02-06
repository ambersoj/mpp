#include <iostream>
#include "plugin.h"

extern "C" {
    void execute(const std::string& args) {
        std::cout << "[STOP] Stopping packet capture...\n";
        // TODO: Send command to `mpp_net` to stop capture
    }

    std::string getHelp() {
        return "stop - Stops packet capture.\nUsage: stop";
    }
}
