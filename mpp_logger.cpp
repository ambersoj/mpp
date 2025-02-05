/* MPP-Logger */
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

#define SOCKET_PATH "/tmp/mpp_logger.sock"
#define LOG_FILE "mpp.log"

// 🕒 Get timestamped log format
std::string getTimestamp() {
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", &tstruct);
    return std::string(buf);
}

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

    chmod(SOCKET_PATH, 0777);  // 🔥 Allow other processes to send logs

    listen(server_sock, 5);
    std::cout << "[LOGGER] MPP-Logger is running...\n";

    std::ofstream logFile(LOG_FILE, std::ios::app);
    if (!logFile) {
        std::cerr << "[LOGGER] ERROR: Failed to open log file!\n";
        return;
    }

    while (true) {
        client_sock = accept(server_sock, nullptr, nullptr);
        if (client_sock < 0) continue;

        char buffer[512] = {0};
        ssize_t bytes_read = read(client_sock, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            std::string logMessage = getTimestamp() + " " + buffer;
            std::cout << "[LOGGER] " << logMessage << std::endl;  // Print to screen
            logFile << logMessage << std::endl;  // Write to file
            logFile.flush();  // Ensure immediate write
        }

        close(client_sock);
    }

    logFile.close();
    close(server_sock);
}

int main() {
    runLogger();
    return 0;
}
