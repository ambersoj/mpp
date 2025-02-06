/* MPP-Core */
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <atomic>
#include <sys/stat.h>
#include <ctime>  // For uptime calculation
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <arpa/inet.h>
#include <algorithm>

#define CMD_SOCKET "/tmp/mpp_cmd.sock"
#define NET_SOCKET "/tmp/mpp_net.sock"
#define CORE_SOCKET "/tmp/mpp_core.sock"
#define TUI_SOCKET "/tmp/mpp_tui.sock"
#define LOGGER_SOCKET "/tmp/mpp_logger.sock"
#define REMOTE_PORT 5000  // 🔥 Changeable in config later
#define MAX_CLIENTS 5

// 🔥 Global Statistics
static int packets_captured = 0;
static int packets_transmitted = 0;
static int errors = 0;
static time_t start_time = time(nullptr);  // Track when core starts

std::atomic<bool> capturing(false);

void sendToLogger(const std::string& message);
void handleCommand(int client_sock, const std::string& cmd);
void sendToNet(const std::string& message);
void receivePackets();
void receiveCoreConnections();
void sendToTUI(const std::string& message);
void sendToCmd(const std::string& message);
void startRemoteListener();
std::string handleRemoteCommand(const std::string& cmd);

std::string handleRemoteCommand(const std::string& cmd) {
    std::ostringstream response;

    if (cmd == "start") {
        sendToNet("start");
        response << "[CORE] Starting packet capture...\n";
    } 
    else if (cmd == "stop") {
        sendToNet("stop");
        response << "[CORE] Stopping packet capture...\n";
    } 
    else if (cmd == "stats") {
        time_t now = time(nullptr);
        int uptime_sec = static_cast<int>(difftime(now, start_time));

        response << "[CORE] Packets Captured: " << packets_captured << "\n";
        response << "[CORE] Packets Transmitted: " << packets_transmitted << "\n";
        response << "[CORE] Errors: " << errors << "\n";
        response << "[CORE] Uptime: " << (uptime_sec / 3600) << "h "
                 << ((uptime_sec % 3600) / 60) << "m "
                 << (uptime_sec % 60) << "s\n";
    }
    else if (cmd.find("tx ") == 0) {
        sendToNet(cmd);
        response << "[CORE] Forwarding TX command to NET...\n";
    }
    else if (cmd.find("filter ") == 0) {
        sendToNet(cmd);
        response << "[CORE] Forwarding filter command to NET...\n";
    }
    else {
        response << "[CORE] ERROR: Unknown command: " << cmd << "\n";
    }

    return response.str();
}

void startRemoteListener() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        std::cerr << "[CORE] ERROR: Failed to create remote socket\n";
        return;
    }

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(5000);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[CORE] ERROR: Failed to bind remote listener\n";
        close(server_sock);
        return;
    }

    listen(server_sock, 5);
    std::cout << "[CORE] Remote Listener started on port 5000\n";

    while (true) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            std::cerr << "[CORE] ERROR: Failed to accept remote connection\n";
            continue;
        }

        char buffer[512] = {0};
        ssize_t bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string command(buffer);
            command.erase(std::remove(command.begin(), command.end(), '\n'), command.end());

            std::cout << "[CORE] Remote Command Received: " << command << "\n";
            
            // 🔥 Send the command to `handleCommand` for processing
            std::string response = handleRemoteCommand(command);

            // 🔥 Send back response to remote client
            send(client_sock, response.c_str(), response.size(), 0);
        }

        close(client_sock);
    }
}

void sendToNet(const std::string& message) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, NET_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, message.c_str(), message.size(), 0);
        std::cout << "[CORE] Sent to NET: " << message << std::endl;
        sendToLogger("[CORE] Sent to NET: " + message);
    } else {
        std::cerr << "[CORE] ERROR: Could not connect to NET.\n";
        sendToLogger("[CORE] ERROR: Could not connect to NET.");
    }
    close(sock);
}

void sendToCmd(const std::string& response) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[CORE] ERROR: Unable to create socket for CMD: " << strerror(errno) << std::endl;
        return;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CMD_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[CORE] ERROR: Unable to connect to CMD: " << strerror(errno) << std::endl;
        close(sock);
        return;
    }

    // 🔥 Send entire response (with newline termination)
    std::string formatted_response = response + "\n";
    if (send(sock, formatted_response.c_str(), formatted_response.size(), 0) == -1) {
        std::cerr << "[CORE] ERROR: Failed to send response to CMD: " << strerror(errno) << std::endl;
    }

    close(sock);
}

void sendToLogger(const std::string& message) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, LOGGER_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, message.c_str(), message.size(), 0);
    } else {
        std::cerr << "[CORE] ERROR: Could not send log to Logger.\n";
    }
    close(sock);
}

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
    chmod(CORE_SOCKET, 0777);
    std::cout << "[CORE] Set permissions 777 for " << CORE_SOCKET << std::endl;


    if (chmod(CORE_SOCKET, 0777) == -1) {
        std::cerr << "[CORE] ERROR: Failed to set permissions on " << CORE_SOCKET << ": " << strerror(errno) << std::endl;
    } else {
        std::cout << "[CORE] Set permissions 777 for " << CORE_SOCKET << std::endl;
    }
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
        sendToNet("start");
    } 
    else if (cmd == "stop") {
        response = "[CORE] Stopping packet capture...\n";
        sendToNet("stop");
    } 
    else if (cmd == "stats") {
        time_t now = time(nullptr);
        int uptime_sec = static_cast<int>(difftime(now, start_time));

        std::ostringstream stats;
        stats << "[CORE] Packets Captured: " << packets_captured << "\n";
        stats << "[CORE] Packets Transmitted: " << packets_transmitted << "\n";
        stats << "[CORE] Errors: " << errors << "\n";
        stats << "[CORE] Uptime: " << (uptime_sec / 3600) << "h " 
              << ((uptime_sec % 3600) / 60) << "m " 
              << (uptime_sec % 60) << "s\n";

        response = stats.str();
        sendToCmd(response); 
    }
    else if (cmd.find("tx ") == 0) {
        response = "[CORE] Forwarding TX command to NET...\n";
        sendToNet(cmd);
    } 
    else if (cmd.find("filter ") == 0) {
        response = "[CORE] Forwarding filter command to NET...\n";
        sendToNet(cmd);
    } 
    else {
        response = "[CORE] ERROR: Unknown command: " + cmd + "\n";
    }

    send(client_sock, response.c_str(), response.size(), 0);
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

    std::cout << "[CORE] Attempting to bind to " << CORE_SOCKET << std::endl;
    if (bind(core_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[CORE] ERROR: Could not bind to core socket: " << strerror(errno) << "\n";
        close(core_sock);
        return;
    }

    // 🔥 Ensure we only set permissions after binding
    if (chmod(CORE_SOCKET, 0777) == -1) {
        std::cerr << "[CORE] ERROR: Failed to set permissions on " << CORE_SOCKET << ": " << strerror(errno) << std::endl;
    } else {
        std::cout << "[CORE] Set permissions 777 for " << CORE_SOCKET << std::endl;
    }

    listen(core_sock, 5);
    std::cout << "[CORE] Listening for NET connections...\n";

    while (true) {
        std::cout << "[CORE] Waiting for NET connection...\n";
        client_sock = accept(core_sock, nullptr, nullptr);
        if (client_sock < 0) {
            std::cerr << "[CORE] ERROR: Accept failed: " << strerror(errno) << "\n";
            continue;
        }

        std::cout << "[CORE] Accepted NET connection!\n";

        char buffer[256] = {0};
        read(client_sock, buffer, sizeof(buffer));

        std::cout << "[CORE] Received NET packet: " << buffer << std::endl;
        sendToTUI(buffer);

        close(client_sock);
    }

    close(core_sock);
}

int main() {
    sendToLogger("[CORE] MPP-Core is starting...");
    std::thread cmdThread(receiveCommands);
    std::thread netThread(receiveCoreConnections);
    std::thread remoteThread(startRemoteListener);
    remoteThread.detach();  // Run in the background

    cmdThread.join();
    netThread.join();
    return 0;
}
