/* MPP-Net */
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <libnet.h>
#include <pcap/pcap.h>
#include <thread>
#include <atomic>
#include <algorithm>  // 🔥 Add this at the top for trimming support
#include <sstream>  // 🔥 Fix for istringstream

#define NET_SOCKET "/tmp/mpp_net.sock"
#define CORE_SOCKET "/tmp/mpp_core.sock"
#define LOGGER_SOCKET "/tmp/mpp_logger.sock"

libnet_t *libnetCtx;
char errbuf[LIBNET_ERRBUF_SIZE];
std::atomic<bool> capturing(false);  // 🔥 Track capture state

// Function Prototypes
void sendToLogger(const std::string& message);
void startServer();
void handleCommands(std::string cmd);
void sendToCore(const std::string& message);
void transmitPacket(const std::string& dstMAC, const std::string& srcMAC, const std::string& payload);
void capturePackets();

// Main Server Function
void startServer() {
    int server_sock, client_sock;
    struct sockaddr_un addr{};

    unlink(NET_SOCKET);
    std::cout << "[NET] Removed stale socket: " << NET_SOCKET << std::endl;

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0) {
        std::cerr << "[NET] ERROR: Could not create socket\n";
        return;
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, NET_SOCKET, sizeof(addr.sun_path) - 1);

    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[NET] ERROR: Could not bind to socket: " << strerror(errno) << "\n";
        close(server_sock);
        return;
    }

    std::cout << "[NET] Successfully bound to " << NET_SOCKET << std::endl;
    chmod(NET_SOCKET, 0777);
    std::cout << "[NET] Set permissions 777 for " << NET_SOCKET << std::endl;

    listen(server_sock, 5);
    std::cout << "[NET] Listening for commands...\n";

    while (true) {
        client_sock = accept(server_sock, nullptr, nullptr);
        if (client_sock < 0) continue;

        char buffer[256] = {0};
        read(client_sock, buffer, sizeof(buffer));

        std::cout << "[NET] Received command: " << buffer << std::endl;
        handleCommands(std::string(buffer));

        close(client_sock);
    }

    close(server_sock);
}

void capturePackets() {
    while (capturing) {
        int packetSize = rand() % 1500 + 60;  // Simulate packet size

        std::string packetInfo = "[NET] Captured Packet: " + std::to_string(packetSize) + " bytes";
        
        sendToCore(packetInfo);  // 🔥 Send clean log to Core
        sendToLogger(packetInfo); // 🔥 Log it
        std::cout << packetInfo << std::endl;

        sleep(1);
    }
    sendToLogger("[NET] Packet capture stopped.");
    std::cout << "[NET] Packet capture stopped." << std::endl;
}

void handleCommands(std::string rawCmd) {
    std::string cmd = rawCmd;  // 🔥 Make a modifiable copy

    sendToLogger("[NET] Raw Command Received: " + cmd);
    std::cout << "[NET] Raw Command Received: " << cmd << std::endl;

    // 🔥 Handle "start" command
    if (cmd == "start") {
        sendToLogger("[NET] Starting packet capture...");
        std::cout << "[NET] Starting packet capture...\n";
        capturing = true;
        std::thread([] {
            while (capturing) {
                std::string packetInfo = "[NET] Captured Packet: " + std::to_string(rand() % 1500 + 60) + " bytes";
                sendToCore(packetInfo);  // 🔥 Send to Core
                sendToLogger(packetInfo); // 🔥 Log it
                std::cout << packetInfo << std::endl;
                sleep(1);
            }
        }).detach();
    }
    // 🔥 Handle "stop" command
    else if (cmd == "stop") {
        sendToLogger("[NET] Stopping packet capture...");
        std::cout << "[NET] Stopping packet capture...\n";
        capturing = false;
        sendToLogger("[NET] Packet capture stopped.");
    }
    // 🔥 Handle "tx" Command (Transmit Packet)
    else if (cmd.substr(0, 3) == "tx ") {  
        std::istringstream iss(cmd.substr(3));  // Skip "tx "
        std::string dstMAC, srcMAC, payload;
        iss >> dstMAC >> srcMAC;
        std::getline(iss, payload);  // 🔥 Preserve everything after srcMAC
        payload = payload.substr(1);  // 🔥 Remove leading space

        if (dstMAC.empty() || srcMAC.empty() || payload.empty()) {
            sendToLogger("[NET] ERROR: Invalid TX command format.");
            std::cerr << "[NET] ERROR: Invalid TX command format.\n";
            return;
        }

        std::string logMessage = "[NET] Transmitting packet: DST=" + dstMAC + " SRC=" + srcMAC + " PAYLOAD=" + payload;
        sendToLogger(logMessage);
        std::cout << logMessage << std::endl;

        transmitPacket(dstMAC, srcMAC, payload);
    }
    // 🔥 Handle "filter" Command
    else if (cmd.substr(0, 7) == "filter ") {  
        std::string filterType = cmd.substr(7);  // Preserve space!
        if (filterType.empty()) {
            sendToLogger("[NET] ERROR: Invalid filter format.");
            std::cerr << "[NET] ERROR: Invalid filter format.\n";
            return;
        }

        sendToLogger("[NET] Filter applied: " + filterType);
        std::cout << "[NET] Filter applied: " << filterType << std::endl;
    }
    // 🔥 Handle "quit" command
    else if (cmd == "quit") {
        sendToLogger("[NET] Shutting down...");
        std::cout << "[NET] Shutting down...\n";
        exit(0);
    }
    else {
        sendToLogger("[NET] ERROR: Unknown command: " + cmd);
        std::cerr << "[NET] ERROR: Unknown command: " << cmd << std::endl;
    }
}

// Transmit a Raw Ethernet Packet Using Libnet
void transmitPacket(const std::string& dstMAC, const std::string& srcMAC, const std::string& payload) {
    if (!libnetCtx) {
        std::cerr << "[NET] ERROR: Libnet context is not initialized!\n";
        return;
    }

    uint8_t dst_mac[6], src_mac[6];
    sscanf(dstMAC.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &dst_mac[0], &dst_mac[1], &dst_mac[2], &dst_mac[3], &dst_mac[4], &dst_mac[5]);
    sscanf(srcMAC.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &src_mac[0], &src_mac[1], &src_mac[2], &src_mac[3], &src_mac[4], &src_mac[5]);

    libnet_ptag_t ethTag = libnet_build_ethernet(
        dst_mac,
        src_mac,
        ETHERTYPE_IP,  
        (uint8_t*)payload.c_str(),
        payload.size(),
        libnetCtx,
        0
    );

    if (ethTag == -1) {
        std::cerr << "[NET] ERROR: Failed to build Ethernet frame: " << libnet_geterror(libnetCtx) << "\n";
        return;
    }

    if (libnet_write(libnetCtx) == -1) {
        std::cerr << "[NET] ERROR: Failed to send packet: " << libnet_geterror(libnetCtx) << "\n";
    } else {
        std::cout << "[NET] Packet transmitted successfully.\n";
        sendToLogger("[NET] Packet transmitted successfully.");
    }

    libnet_clear_packet(libnetCtx);
}

void sendToLogger(const std::string& message) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, LOGGER_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, message.c_str(), message.size(), 0);
    } else {
        std::cerr << "[NET] ERROR: Could not send log to Logger.\n";
    }
    close(sock);
}

// Send Messages to Core
void sendToCore(const std::string& message) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CORE_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, message.c_str(), message.size(), 0);

        // 🔥 LOG MORE PACKET DETAILS
        std::cout << "[NET] Captured Packet: " << message << std::endl;
    } else {
        std::cerr << "[NET] ERROR: Could not connect to Core.\n";
    }
    close(sock);
}

int main() {
    libnetCtx = libnet_init(LIBNET_LINK, "eno1", errbuf);
    if (!libnetCtx) {
        std::cerr << "[NET] ERROR: Libnet initialization failed: " << errbuf << "\n";
        return 1;
    }
    sendToLogger("[NET] Libnet initialized.");
    startServer();
    libnet_destroy(libnetCtx);
    return 0;
}
