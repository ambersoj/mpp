#ifndef SOCKET_HELPER_H
#define SOCKET_HELPER_H

#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>

#define CORE_SOCKET "/tmp/mpp_cmd.sock"  // 🔥 Ensure this matches `mpp_core.cpp`

inline std::string sendRequestToCore(const std::string& request) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[STATS] ERROR: Could not create socket\n";
        return "";
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CORE_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        std::cerr << "[STATS] ERROR: Could not connect to Core\n";
        close(sock);
        return "";
    }

    send(sock, request.c_str(), request.size(), 0);
    char buffer[512] = {0};
    read(sock, buffer, sizeof(buffer) - 1);
    close(sock);
    
    return std::string(buffer);
}

#endif
