/* MPP-TUI */

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <ncurses.h>
#include <vector>
#include <thread>

#define CORE_SOCKET "/tmp/mpp_core.sock"
#define TUI_SOCKET "/tmp/mpp_tui.sock"

std::vector<std::string> packetHistory;  // 🔥 Stores packet logs

void startUI();
void receivePackets();
void sendToCore(const std::string& message);
void displayPackets();

void startServer() {
    int server_sock, client_sock;
    struct sockaddr_un addr{};

    unlink(TUI_SOCKET);
    std::cout << "[TUI] Removed stale socket: " << TUI_SOCKET << std::endl;

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0) {
        std::cerr << "[TUI] ERROR: Could not create socket\n";
        return;
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, TUI_SOCKET, sizeof(addr.sun_path) - 1);

    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[TUI] ERROR: Could not bind to socket: " << strerror(errno) << "\n";
        close(server_sock);
        return;
    }

    std::cout << "[TUI] Successfully bound to " << TUI_SOCKET << std::endl;
    listen(server_sock, 5);
    std::cout << "[TUI] Listening for packets...\n";

    std::thread(receivePackets).detach();
    startUI();
}

void startUI() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    scrollok(stdscr, TRUE);

    mvprintw(0, 0, "TUI Packet Viewer. Press 'q' to quit.");
    refresh();

    while (true) {
        int ch = getch();
        if (ch == 'q') {
            endwin();
            sendToCore("shutdown");
            break;
        }
    }
}

void displayPackets() {
    int row = 1;  // Start displaying below title

    // Keep only last N packets (for scrolling effect)
    int max_rows = LINES - 2;
    if (packetHistory.size() > max_rows) {
        packetHistory.erase(packetHistory.begin(), packetHistory.begin() + (packetHistory.size() - max_rows));
    }

    for (const auto& packet : packetHistory) {
        mvprintw(row++, 0, "%s", packet.c_str());  // Append new packets
    }

    refresh();
}

void receivePackets() {
    int server_sock, client_sock;
    struct sockaddr_un addr{};

    unlink(TUI_SOCKET);
    std::cout << "[TUI] Removed stale socket: " << TUI_SOCKET << std::endl;

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0) {
        std::cerr << "[TUI] ERROR: Could not create socket\n";
        return;
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, TUI_SOCKET, sizeof(addr.sun_path) - 1);

    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[TUI] ERROR: Could not bind to socket: " << strerror(errno) << "\n";
        close(server_sock);
        return;
    }

    std::cout << "[TUI] Successfully bound to " << TUI_SOCKET << std::endl;
    listen(server_sock, 5);
    std::cout << "[TUI] Listening for packets...\n";

    while (true) {
        client_sock = accept(server_sock, nullptr, nullptr);
        if (client_sock < 0) continue;

        char buffer[256] = {0};
        read(client_sock, buffer, sizeof(buffer));

        std::cout << "[TUI] Received Packet: " << buffer << std::endl;  // 🔥 DEBUG
        packetHistory.push_back(buffer);
        displayPackets();  // 🔥 Refresh UI

        close(client_sock);
    }

    close(server_sock);
}

void sendToCore(const std::string& message) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, CORE_SOCKET, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, message.c_str(), message.size(), 0);
        std::cout << "[TUI] Sent to Core: " << message << std::endl;
    } else {
        std::cerr << "[TUI] ERROR: Could not connect to Core.\n";
    }
    close(sock);
}

int main() {
    startServer();
    return 0;
}
