#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <sstream>
#include <dlfcn.h>

#define CORE_SOCKET "/tmp/mpp_cmd.sock"
#define COMMANDS_DIR "./commands/"  // 🔥 Where external commands (.so) live

void sendToCore(const std::string& command);
void executeExternalCommand(const std::string& command);

int main() {
    std::cout << "[CMD] MPP Shell Ready. Type commands ('help' for info):\n";
    
    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);

        // 🔥 Trim whitespace
        input.erase(0, input.find_first_not_of(" \t\n\r"));
        input.erase(input.find_last_not_of(" \t\n\r") + 1);

        if (input.empty()) continue;

        // 🔥 Built-in commands
        if (input == "help" || input == "h" || input == "?") {
            std::cout << "[CMD] Usage: <command> <args>\n";
            std::cout << "[CMD] Built-in: help, quit, exit\n";
            std::cout << "[CMD] Plugins: " << COMMANDS_DIR << "*.so\n";
        }
        else if (input == "quit" || input == "q" || input == "exit") {
            std::cout << "[CMD] Exiting MPP Shell...\n";
            sendToCore("shutdown");
            break;
        }
        else {
            // 🔥 Delegate to external plugin
            executeExternalCommand(input);
        }
    }

    return 0;
}

// 🔥 Send commands to Core via UNIX socket
void sendToCore(const std::string& command) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[CMD] ERROR: Could not create socket\n";
        return;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CORE_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, command.c_str(), command.size(), 0);
        char buffer[256] = {0};
        read(sock, buffer, sizeof(buffer) - 1);
        std::cout << "[CMD] Response: " << buffer << "\n";
    } else {
        std::cerr << "[CMD] ERROR: Could not connect to Core\n";
    }

    close(sock);
}

void executeExternalCommand(const std::string& cmd) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[CMD] ERROR: Could not create socket\n";
        return;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CORE_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, cmd.c_str(), cmd.size(), 0);
        std::cout << "[CMD] Sent command to Core: " << cmd << std::endl;  // 🔥 Debugging

        // Read response
        char buffer[256] = {0};
        ssize_t bytes_read = read(sock, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            std::cout << "[CMD] Response from Core (" << bytes_read << " bytes):\n" << buffer << std::endl;
        } else {
            std::cerr << "[CMD] ERROR: No response received from Core (" << strerror(errno) << ")\n";
        }

        close(sock);
        return;
    }

    std::cerr << "[CMD] ERROR: Could not connect to core\n";
    close(sock);
}
