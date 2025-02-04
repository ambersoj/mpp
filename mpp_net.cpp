/* MPP-Net */

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <libnet.h>
#include <pcap.h>

#define NET_SOCKET "/tmp/mpp_net.sock"
#define CORE_SOCKET "/tmp/mpp_core.sock"

libnet_t *libnetCtx;
char errbuf[LIBNET_ERRBUF_SIZE];

// Function Prototypes
void startServer();
void handleCommands(const std::string& cmd);
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
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    struct pcap_pkthdr header;
    const u_char *packet;

    // Open the network device for packet capture
    handle = pcap_open_live("eno1", 65535, 1, 1000, errbuf);
    if (handle == nullptr) {
        std::cerr << "[NET] ERROR: Could not open device eno1: " << errbuf << std::endl;
        return;
    }

    std::cout << "[NET] Packet capture started on eno1\n";

    while (true) {
        packet = pcap_next(handle, &header);
        if (packet == nullptr) continue;  // No packet, continue looping

        std::cout << "[NET] Captured Packet: " << header.len << " bytes\n";  // ✅ Real size
        std::string packetData = "[NET] Captured Packet: " + std::to_string(header.len) + " bytes";
        sendToCore(packetData);  // ✅ Send real packet data to Core
    }

    pcap_close(handle);
}

// Handles Incoming Commands
void handleCommands(const std::string& cmd) {
    if (cmd == "start") {
        std::cout << "[NET] Starting real packet capture...\n";
        std::thread(capturePackets).detach();  // ✅ Run real packet capture in a new thread
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
        ETHERTYPE_IP,  // Placeholder, could be configurable
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
    }

    libnet_clear_packet(libnetCtx);
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

    std::cout << "[NET] Libnet initialized on interface eno1.\n";
    startServer();

    libnet_destroy(libnetCtx);
    return 0;
}
