/* MPP-Cmd */
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>

#define CMD_SOCKET "/tmp/mpp_cmd.sock"
#define CORE_SOCKET "/tmp/mpp_cmd.sock"
#define LOGGER_SOCKET "/tmp/mpp_logger.sock"

void sendToLogger(const std::string& message);
void sendCommand(const std::string& command);
void commandLoop();

void sendToLogger(const std::string& message) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, LOGGER_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, message.c_str(), message.size(), 0);
    } else {
        std::cerr << "[CMD] ERROR: Could not send log to Logger.\n";
    }
    close(sock);
}

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
        sendToLogger("[CMD] Sent command: " + command);

        // Read Core's response
        char buffer[256] = {0};
        ssize_t bytes_read = read(sock, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            std::cout << "[CMD] Response (" << bytes_read << " bytes): " << buffer << std::endl;
            sendToLogger("[CMD] Response: " + std::string(buffer));
        } else {
            std::cerr << "[CMD] ERROR: No response received from Core (" << strerror(errno) << ")\n";
            sendToLogger("[CMD] ERROR: No response received from Core");
        }

        close(sock);
        return;
    }

    std::cerr << "[CMD] ERROR: Could not connect to Core\n";
    sendToLogger("[CMD] ERROR: Could not connect to Core");
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
            std::cout << "[CMD] Exiting...\n";
            sendToLogger("[CMD] Exiting...");
            exit(0);
        }

        sendCommand(command);
    }
}

int main() {
    sendToLogger("[CMD] MPP-Cmd started.");
    commandLoop();
    return 0;
}
