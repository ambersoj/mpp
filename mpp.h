#ifndef MPP_H
#define MPP_H

#include <iostream>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <string>
#include <cstring>
#include <libnet.h>
#include <pcap.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Observer Interface
class Observer {
public:
    virtual void update(const std::string& message) = 0;
    virtual ~Observer() = default;
};

// Concrete Observer: Logger
class Logger : public Observer {
private:
    std::ofstream logFile;

public:
    Logger();
    ~Logger();
    void update(const std::string& message) override;
};

// Command Interface
class Command {
protected:
    std::vector<std::shared_ptr<Observer>> observers;

public:
    void addObserver(std::shared_ptr<Observer> observer);
    void notifyObservers(const std::string& message);
    virtual void execute() = 0;
    virtual ~Command() = default;
};

// Concrete Commands
class TxCommand : public Command {
public:
    void execute() override;
};

class RxCommand : public Command {
public:
    void execute() override;
};

class QuitCommand : public Command {
public:
    void execute() override;
};

class UmlCommand : public Command {
public:
    void execute() override;
};

// Network Manager
class NetworkManager {
private:
    int sock;
    struct sockaddr_in server_addr;

public:
    bool connectToServer(const std::string& ip, int port);
    bool sendData(const std::string& data);
    ~NetworkManager();
};

// Remote Commands
class RemoteTxCommand : public Command {
private:
    NetworkManager& networkManager;

public:
    explicit RemoteTxCommand(NetworkManager& nm);
    void execute() override;
};

class RemoteRxCommand : public Command {
private:
    NetworkManager& networkManager;

public:
    explicit RemoteRxCommand(NetworkManager& nm);
    void execute() override;
};

// Command Manager
class CommandManager {
private:
    std::unordered_map<std::string, std::shared_ptr<Command>> commands;

public:
    void registerCommand(const std::string& name, std::shared_ptr<Command> command);
    void executeCommand(const std::string& name);
};

#endif // MPP_H
