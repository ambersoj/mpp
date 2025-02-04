/* MPP-Cmd */

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

#define CORE_SOCKET "/tmp/mpp_cmd.sock"

void sendCommand(const std::string& command) {
    int sock;
    struct sockaddr_un addr{};

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[CMD] ERROR: Could not create socket\n";
        return;
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CORE_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, command.c_str(), command.size(), 0);
        std::cout << "[CMD] Sent command: " << command << std::endl;

        // Read entire response
        char buffer[256] = {0};
        ssize_t bytes_read = read(sock, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            std::cout << "[CMD] Response (" << bytes_read << " bytes): " << buffer << std::endl;
        } else {
            std::cerr << "[CMD] ERROR: No response received from Core (" << strerror(errno) << ")\n";
        }

        close(sock);
        return;
    }

    std::cerr << "[CMD] ERROR: Could not connect to core\n";
    close(sock);
}

void commandLoop() {
    std::string command;
    std::cout << "[CMD] Type commands (start, stop, filter <keyword>, quit):" << std::endl;

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, command);

        if (command == "quit") {
            sendCommand("shutdown");
            break;
        }

        sendCommand(command);
    }
}

int main() {
    commandLoop();
    return 0;
}
