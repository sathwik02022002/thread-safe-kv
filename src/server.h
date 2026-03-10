#pragma once
#include <string>
#include <atomic>
#include "kvstore.h"
#include "threadpool.h"

class Server {
public:
    Server(KVStore& store, int port = 6379, int num_threads = 4);
    ~Server();

    void start();
    void stop();

private:
    void accept_loop();
    void handle_client(int client_fd);
    std::string process_command(const std::string& command);

    KVStore& store_;
    ThreadPool pool_;
    int port_;
    int server_fd_;
    std::atomic<bool> running_;
};
