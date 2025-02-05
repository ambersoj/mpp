/* MPP-TUI */
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <ncurses.h>
#include <vector>
#include <thread>
#include <algorithm>  // 🔥 Make sure this is included at the top!
#include <sys/stat.h>

#define CORE_SOCKET "/tmp/mpp_core.sock"
#define TUI_SOCKET "/tmp/mpp_tui.sock"
#define LOGGER_SOCKET "/tmp/mpp_logger.sock"

std::vector<std::string> packetHistory;

void sendToLogger(const std::string& message);
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
    chmod(TUI_SOCKET, 0777);
    std::cout << "[TUI] Set permissions 777 for " << TUI_SOCKET << std::endl;
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
    clear();
    mvprintw(0, 0, "TUI Packet Viewer. Press 'q' to quit.");

    int max_rows = LINES - 2;  // 🔥 Prevent overflow
    int start = (packetHistory.size() > max_rows) ? packetHistory.size() - max_rows : 0;

    for (int i = 0; i < max_rows && (start + i) < packetHistory.size(); ++i) {
        std::string cleanLog = packetHistory[start + i];

        // ✅ Correctly remove newline characters using erase-remove idiom
        cleanLog.erase(std::remove(cleanLog.begin(), cleanLog.end(), '\n'), cleanLog.end());

        mvprintw(i + 1, 0, "%s", cleanLog.c_str());
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

        std::string message(buffer);
        if (message == "exit") {  // 🔥 Check if exit command received
            std::cout << "[TUI] Exiting...\n";
            sendToLogger("[TUI] Exiting...");
            endwin();  // 🔥 Exit ncurses properly
            exit(0);
        }

        packetHistory.push_back(message);
        displayPackets();

        close(client_sock);
    }

    close(server_sock);
}

void sendToLogger(const std::string& message) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/mpp_logger.sock", sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        send(sock, message.c_str(), message.size(), 0);
    }
    close(sock);
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
    sendToLogger("[TUI] MPP-TUI started.");
    startServer();
    return 0;
}

