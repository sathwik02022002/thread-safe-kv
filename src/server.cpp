#include "server.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>

Server::Server(KVStore& store, int port, int num_threads)
    : store_(store), pool_(num_threads), port_(port), server_fd_(-1), running_(false) {}

Server::~Server() {
    stop();
}

void Server::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        throw std::runtime_error("Failed to bind to port " + std::to_string(port_));
    }

    if (listen(server_fd_, 10) < 0) {
        throw std::runtime_error("Failed to listen");
    }

    running_ = true;
    std::cout << "[Server] Listening on port " << port_ << std::endl;
    accept_loop();
}

void Server::stop() {
    running_ = false;
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
    pool_.stop();
}

void Server::accept_loop() {
    while (running_) {
        int client_fd = accept(server_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            if (running_) std::cerr << "[Server] Accept failed" << std::endl;
            break;
        }
        pool_.enqueue([this, client_fd]() {
            handle_client(client_fd);
        });
    }
}

void Server::handle_client(int client_fd) {
    char buffer[4096];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) break;

        std::string command(buffer, bytes);
        // Strip trailing newline/carriage return
        command.erase(std::remove(command.begin(), command.end(), '\r'), command.end());
        command.erase(std::remove(command.begin(), command.end(), '\n'), command.end());

        if (command == "QUIT") break;

        std::string response = process_command(command) + "\n";
        send(client_fd, response.c_str(), response.size(), 0);
    }
    close(client_fd);
}

std::string Server::process_command(const std::string& command) {
    std::istringstream iss(command);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) tokens.push_back(token);

    if (tokens.empty()) return "ERROR: Empty command";

    std::string cmd = tokens[0];
    // Convert command to uppercase
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "SET") {
        if (tokens.size() < 3) return "ERROR: SET requires key and value";
        int ttl = -1;
        if (tokens.size() >= 4) {
            try { ttl = std::stoi(tokens[3]); }
            catch (...) { return "ERROR: Invalid TTL"; }
        }
        store_.set(tokens[1], tokens[2], ttl);
        return "OK";

    } else if (cmd == "GET") {
        if (tokens.size() < 2) return "ERROR: GET requires a key";
        auto val = store_.get(tokens[1]);
        return val.has_value() ? val.value() : "nil";

    } else if (cmd == "DEL") {
        if (tokens.size() < 2) return "ERROR: DEL requires a key";
        return store_.del(tokens[1]) ? "1" : "0";

    } else if (cmd == "EXISTS") {
        if (tokens.size() < 2) return "ERROR: EXISTS requires a key";
        return store_.exists(tokens[1]) ? "1" : "0";

    } else if (cmd == "FLUSH") {
        store_.flush();
        return "OK";

    } else if (cmd == "SIZE") {
        return std::to_string(store_.size());

    } else {
        return "ERROR: Unknown command '" + cmd + "'";
    }
}
