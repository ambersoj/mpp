include "mpp.h"
#include <iostream>
#include <string>
#include <memory>
#include <sstream>

// Logger Class Implementation
Logger::Logger() {
    logFile.open("/usr/local/mpp/chatgptmpp/vscmpp/mpplog.txt", std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file." << std::endl;
        throw std::ios_base::failure("Log file could not be opened");
    }
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::update(const std::string& message) {
    if (logFile.is_open()) {
        logFile << "[LOG]: " << message << std::endl;
    }
    std::cout << message << std::endl; // Print to console for real-time feedback
}

// Command Class Implementation
void Command::addObserver(std::shared_ptr<Observer> observer) {
    observers.push_back(observer);
}

void Command::notifyObservers(const std::string& message) {
    for (const auto& observer : observers) {
        observer->update(message);
    }
}

// TxCommand Class Implementation
void TxCommand::execute() {
    char errbuf[LIBNET_ERRBUF_SIZE];
    libnet_t *l;

    l = libnet_init(LIBNET_LINK, NULL, errbuf);
    if (l == NULL) {
        notifyObservers("TX command failed: libnet_init failed.");
        return;
    }

    uint8_t dest_mac[6] = {0xBC, 0xE9, 0x2F, 0x80, 0x3B, 0x56};
    uint8_t src_mac[6] = {0xEC, 0xB1, 0xD7, 0x52, 0x8C, 0x52};
    char payload[] = "Ping message";
    uint16_t payload_size = sizeof(payload);

    libnet_ptag_t t = libnet_build_ethernet(
        dest_mac, src_mac, ETHERTYPE_IP,
        (uint8_t*)payload, payload_size, l, 0);

    if (t == -1) {
        libnet_destroy(l);
        notifyObservers("TX command failed: libnet_build_ethernet failed.");
        return;
    }

    int bytes_written = libnet_write(l);
    if (bytes_written == -1) {
        notifyObservers("TX command failed: libnet_write failed.");
    } else {
        notifyObservers("TX command succeeded: Packet sent.");
    }

    libnet_destroy(l);
}

// RxCommand Class Implementation
void RxCommand::execute() {
    char errbuf[PCAP_ERRBUF_SIZE];
    const char* dev = pcap_lookupdev(errbuf);
    if (dev == nullptr) {
        notifyObservers("RX command failed: No suitable device found.");
        return;
    }

    notifyObservers("RX command started. Capturing on device: " + std::string(dev));

    pcap_t* handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (handle == nullptr) {
        notifyObservers("RX command failed: Unable to open device.");
        return;
    }

    struct pcap_pkthdr header;
    const u_char* packet = pcap_next(handle, &header);
    if (packet == nullptr) {
        notifyObservers("RX command failed: No packets captured.");
    } else {
        notifyObservers("RX command succeeded: Packet captured. Length: " + std::to_string(header.len));
    }

    pcap_close(handle);
}

// QuitCommand Class Implementation
void QuitCommand::execute() {
    notifyObservers("Exiting program.");
    exit(0);
}

// UmlCommand Class Implementation
void UmlCommand::execute() {
    const std::string umlFilePath = "/usr/local/mpp/chatgptmpp/vscmpp/mpp_class_diagram.puml";
    std::ofstream umlFile(umlFilePath);
    if (!umlFile.is_open()) {
        notifyObservers("UML command failed: Could not open file for writing.");
        return;
    }

    umlFile << "@startuml\n";
    umlFile << "class Observer {\n";
    umlFile << "    + void update(const std::string&)\n";
    umlFile << "}\n";

    umlFile << "class Logger {\n";
    umlFile << "    - std::ofstream logFile\n";
    umlFile << "    + void update(const std::string&)\n";
    umlFile << "}\n";

    umlFile << "class Command {\n";
    umlFile << "    + void addObserver(std::shared_ptr<Observer>)\n";
    umlFile << "    + void notifyObservers(const std::string&)\n";
    umlFile << "    + void execute()\n";
    umlFile << "    # std::vector<std::shared_ptr<Observer>> observers\n";
    umlFile << "}\n";

    umlFile << "class CommandManager {\n";
    umlFile << "    + void registerCommand(const std::string&, std::shared_ptr<Command>)\n";
    umlFile << "    + void executeCommand(const std::string&)\n";
    umlFile << "    - std::unordered_map<std::string, std::shared_ptr<Command>> commands\n";
    umlFile << "}\n";

    umlFile << "class TxCommand {\n";
    umlFile << "    + void execute()\n";
    umlFile << "}\n";

    umlFile << "class RxCommand {\n";
    umlFile << "    + void execute()\n";
    umlFile << "}\n";

    umlFile << "class QuitCommand {\n";
    umlFile << "    + void execute()\n";
    umlFile << "}\n";

    umlFile << "class UmlCommand {\n";
    umlFile << "    + void execute()\n";
    umlFile << "}\n";

    umlFile << "class NetworkManager {\n";
    umlFile << "    - int sock\n";
    umlFile << "    - struct sockaddr_in server_addr\n";
    umlFile << "    + bool connectToServer(const std::string&, int)\n";
    umlFile << "    + bool sendData(const std::string&)\n";
    umlFile << "}\n";

    umlFile << "class RemoteTxCommand {\n";
    umlFile << "    + void execute()\n";
    umlFile << "    - NetworkManager& networkManager\n";
    umlFile << "}\n";

    umlFile << "class RemoteRxCommand {\n";
    umlFile << "    + void execute()\n";
    umlFile << "    - NetworkManager& networkManager\n";
    umlFile << "}\n";

    umlFile << "Observer <|-- Logger\n";
    umlFile << "Observer <|-- Command\n";
    umlFile << "CommandManager o-- Command\n";
    umlFile << "Command <|-- TxCommand\n";
    umlFile << "Command <|-- RxCommand\n";
    umlFile << "Command <|-- QuitCommand\n";
    umlFile << "Command <|-- UmlCommand\n";
    umlFile << "Command <|-- RemoteTxCommand\n";
    umlFile << "Command <|-- RemoteRxCommand\n";
    umlFile << "RemoteTxCommand o-- NetworkManager\n";
    umlFile << "RemoteRxCommand o-- NetworkManager\n";
    umlFile << "@enduml\n";

    umlFile.close();
    notifyObservers("UML command succeeded: UML diagram generated at " + umlFilePath);
}

// NetworkManager Class Implementation
bool NetworkManager::connectToServer(const std::string& ip, int port) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    return connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) >= 0;
}

bool NetworkManager::sendData(const std::string& data) {
    return send(sock, data.c_str(), data.size(), 0) >= 0;
}

NetworkManager::~NetworkManager() {
    close(sock);
}

// RemoteTxCommand Class Implementation
RemoteTxCommand::RemoteTxCommand(NetworkManager& nm) : networkManager(nm) {}

void RemoteTxCommand::execute() {
    notifyObservers(networkManager.sendData("tx") ?
        "RTx command sent successfully." :
        "RTx command failed to send.");
}

// RemoteRxCommand Class Implementation
RemoteRxCommand::RemoteRxCommand(NetworkManager& nm) : networkManager(nm) {}

void RemoteRxCommand::execute() {
    notifyObservers(networkManager.sendData("rx") ?
        "RRx command sent successfully." :
        "RRx command failed to send.");
}

// CommandManager Class Implementation
void CommandManager::registerCommand(const std::string& name, std::shared_ptr<Command> command) {
    commands[name] = std::move(command);
}

void CommandManager::executeCommand(const std::string& name) {
    auto it = commands.find(name);
    if (it != commands.end()) {
        it->second->execute();
    } else {
        std::cout << "Unknown command: " << name << std::endl;
    }
}

// Main function: CLI Implementation
int main() {
    CommandManager commandManager;
    auto logger = std::make_shared<Logger>();

    auto txCommand = std::make_shared<TxCommand>();
    auto rxCommand = std::make_shared<RxCommand>();
    auto quitCommand = std::make_shared<QuitCommand>();
    auto umlCommand = std::make_shared<UmlCommand>();

    NetworkManager networkManager;
    auto remoteTxCommand = std::make_shared<RemoteTxCommand>(networkManager);
    auto remoteRxCommand = std::make_shared<RemoteRxCommand>(networkManager);

    txCommand->addObserver(logger);
    rxCommand->addObserver(logger);
    quitCommand->addObserver(logger);
    umlCommand->addObserver(logger);
    remoteTxCommand->addObserver(logger);
    remoteRxCommand->addObserver(logger);

    commandManager.registerCommand("tx", txCommand);
    commandManager.registerCommand("rx", rxCommand);
    commandManager.registerCommand("quit", quitCommand);
    commandManager.registerCommand("uml", umlCommand);
    commandManager.registerCommand("rtx", remoteTxCommand);
    commandManager.registerCommand("rrx", remoteRxCommand);

    std::string userInput;
    std::cout << "Enter command (tx, rx, quit, uml, rtx, rrx): ";
    while (true) {
        std::getline(std::cin, userInput);
        if (userInput == "quit") {
            commandManager.executeCommand("quit");
            break;
        }
        commandManager.executeCommand(userInput);
        std::cout << "Enter command (tx, rx, quit, uml, rtx, rrx): ";
    }

    return 0;
}
