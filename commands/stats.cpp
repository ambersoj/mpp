#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include "plugin.h"
#include "socket_helper.h"  // Utility to talk to core

#define CMD_SOCKET "/tmp/mpp_cmd.sock"

extern "C" void execute(const std::string& args) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[STATS] ERROR: Unable to create socket\n";
        return;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CMD_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[STATS] ERROR: Unable to connect to CORE\n";
        close(sock);
        return;
    }

    std::string command = "stats\n";  // 🔥 Ensure command is sent properly
    send(sock, command.c_str(), command.size(), 0);

    char buffer[512] = {0};
    ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        std::string response(buffer);

        // 🔥 Check if the response is prefixed with [CORE-STATS]
        if (response.find("[CORE-STATS]") != std::string::npos) {
            response.erase(0, std::string("[CORE-STATS]\n").size());  // Remove the prefix
            std::cout << response;  // ✅ Correctly formatted output
        } else {
            std::cerr << "[STATS] ERROR: Invalid response from Core\n";
        }
    } else {
        std::cerr << "[STATS] ERROR: Unable to retrieve statistics from CORE\n";
    }
 
    close(sock);
}



