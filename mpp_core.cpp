/* MPP-Core */

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <atomic>
#include <sys/stat.h>

#define CMD_SOCKET "/tmp/mpp_cmd.sock"
#define NET_SOCKET "/tmp/mpp_net.sock"
#define CORE_SOCKET "/tmp/mpp_core.sock"  // 🔥 Add this!
#define TUI_SOCKET "/tmp/mpp_tui.sock"  // 🔥 Add this

std::atomic<bool> capturing(false);

void handleCommand(int client_sock, const std::string& cmd);
void sendToNet(const std::string& message);
void receivePackets();
void receiveCoreConnections();
void sendToTUI(const std::string& message);

void sendToTUI(const std::string& message) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, TUI_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, message.c_str(), message.size(), 0);
        std::cout << "[CORE] Forwarded to TUI: " << message << std::endl;
    } else {
        std::cerr << "[CORE] ERROR: Could not connect to TUI.\n";
    }
    close(sock);
}

void receiveCommands() {
    int server_sock, client_sock;
    struct sockaddr_un addr{};
    
    std::cout << "[CORE] Checking if old socket exists: " << CMD_SOCKET << std::endl;
    if (access(CMD_SOCKET, F_OK) == 0) {
        std::cout << "[CORE] Old socket found. Removing...\n";
        unlink(CMD_SOCKET);
    }

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0) {
        std::cerr << "[CORE] ERROR: Could not create socket: " << strerror(errno) << "\n";
        return;
    }
    std::cout << "[CORE] Created socket.\n";

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CMD_SOCKET, sizeof(addr.sun_path) - 1);

    std::cout << "[CORE] Attempting to bind to " << CMD_SOCKET << std::endl;
    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[CORE] ERROR: Could not bind to socket: " << strerror(errno) << "\n";
        close(server_sock);
        return;
    }

    std::cout << "[CORE] Successfully bound to " << CMD_SOCKET << std::endl;
    
    // 🔥 Fix: Set permissions so other components can access it
    chmod(CMD_SOCKET, 0777);

    listen(server_sock, 5);
    std::cout << "[CORE] Listening for commands...\n";

    while (true) {
        std::cout << "[CORE] Waiting for connection...\n";
        client_sock = accept(server_sock, nullptr, nullptr);
        if (client_sock < 0) {
            std::cerr << "[CORE] ERROR: Accept failed: " << strerror(errno) << "\n";
            continue;
        }

        char buffer[256] = {0};
        read(client_sock, buffer, sizeof(buffer));

        std::cout << "[CORE] Received command: " << buffer << std::endl;
        handleCommand(client_sock, buffer);
        close(client_sock);
    }

    close(server_sock);
}

void handleCommand(int client_sock, const std::string& cmd) {
    std::string response;
    
    if (cmd == "start") {
        response = "[CORE] Starting packet capture...\n";
        capturing = true;
        sendToNet("start");
    } else if (cmd == "stop") {
        response = "[CORE] Stopping packet capture...\n";
        capturing = false;
        sendToNet("stop");
    } else if (cmd.find("tx ") == 0) {  // 🔥 NEW: Forward TX Commands
        response = "[CORE] Forwarding TX command to NET...\n";
        sendToNet(cmd);  // Forward TX command
    } else {
        response = "[CORE] Unknown command: " + cmd + "\n";
    }

    send(client_sock, response.c_str(), response.size(), 0);
}

void sendToNet(const std::string& message) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, NET_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, message.c_str(), message.size(), 0);
        std::cout << "[CORE] Sent to NET: " << message << std::endl;
    } else {
        std::cerr << "[CORE] ERROR: Could not connect to NET.\n";
    }
    close(sock);
}

void receivePackets() {
    while (capturing) {
        std::cout << "[CORE] Capturing packets...\n";
        sleep(1); // Simulate packet capturing
    }
    std::cout << "[CORE] Stopped packet capture.\n";
}

void receiveCoreConnections() {
    int core_sock, client_sock;
    struct sockaddr_un addr{};

    unlink(CORE_SOCKET);
    std::cout << "[CORE] Removed stale socket: " << CORE_SOCKET << std::endl;

    core_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (core_sock < 0) {
        std::cerr << "[CORE] ERROR: Could not create core socket: " << strerror(errno) << "\n";
        return;
    }
    std::cout << "[CORE] Created socket for NET connections.\n";

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CORE_SOCKET, sizeof(addr.sun_path) - 1);

    if (bind(core_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[CORE] ERROR: Could not bind to core socket: " << strerror(errno) << "\n";
        close(core_sock);
        return;
    }

    std::cout << "[CORE] Successfully bound to " << CORE_SOCKET << std::endl;
    chmod(CORE_SOCKET, 0777);

    listen(core_sock, 5);
    std::cout << "[CORE] Listening for NET connections...\n";

    while (true) {
        std::cout << "[CORE] Waiting for NET connection...\n";
        client_sock = accept(core_sock, nullptr, nullptr);
        if (client_sock < 0) {
            std::cerr << "[CORE] ERROR: Accept failed: " << strerror(errno) << "\n";
            continue;
        }

        char buffer[256] = {0};
        read(client_sock, buffer, sizeof(buffer));

        std::cout << "[CORE] Received NET packet: " << buffer << std::endl;
        sendToTUI(buffer);  // 🔥 Make sure this line executes

        close(client_sock);
    }

    close(core_sock);
}

int main() {
    std::thread cmdThread(receiveCommands);  // Handles CLI commands
    std::thread netThread(receiveCoreConnections);  // Handles packets from NET

    cmdThread.join();
    netThread.join();

    return 0;
}
