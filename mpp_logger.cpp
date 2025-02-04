/* MPP-Logger */

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fstream>
#include <cstring>

#define SOCKET_PATH "/tmp/mpp_logger.sock"
#define LOG_FILE "mpp.log"

void runLogger() {
    int server_sock, client_sock;
    struct sockaddr_un addr{};
    
    unlink(SOCKET_PATH);  // Ensure the socket doesn’t already exist

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0) {
        std::cerr << "[LOGGER] ERROR: Could not create socket\n";
        return;
    }

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKET_PATH);

    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[LOGGER] ERROR: Could not bind socket\n";
        return;
    }

    listen(server_sock, 5);
    std::cout << "[LOGGER] MPP-Logger is running...\n";

    std::ofstream logFile(LOG_FILE, std::ios::app);

    while (true) {
        client_sock = accept(server_sock, nullptr, nullptr);
        if (client_sock < 0) continue;

        char buffer[256] = {0};
        read(client_sock, buffer, sizeof(buffer));
        std::cout << "[LOGGER] Received: " << buffer << std::endl;
        logFile << buffer << std::endl;

        close(client_sock);
    }

    logFile.close();
    close(server_sock);
}

int main() {
    runLogger();
    return 0;
}
