#include <iostream>
#include <csignal>
#include "kvstore.h"
#include "ttl_manager.h"
#include "server.h"

// Global server pointer for signal handling
Server* g_server = nullptr;

void signal_handler(int /*signal*/) {
    std::cout << "\n[Main] Shutting down..." << std::endl;
    if (g_server) g_server->stop();
}

int main(int argc, char* argv[]) {
    int port = 6379;
    if (argc > 1) {
        try { port = std::stoi(argv[1]); }
        catch (...) { std::cerr << "Invalid port, using default 6379" << std::endl; }
    }

    std::cout << "=================================" << std::endl;
    std::cout << "  Thread-Safe KV Store Server    " << std::endl;
    std::cout << "  Max Keys : 1000                " << std::endl;
    std::cout << "  Threads  : 4                   " << std::endl;
    std::cout << "  Port     : " << port << "              " << std::endl;
    std::cout << "=================================" << std::endl;

    // Core store with max 1000 keys
    KVStore store(1000);

    // TTL manager — purges expired keys every second
    TTLManager ttl_manager(store, 1);
    ttl_manager.start();

    // Server with thread pool
    Server server(store, port, 4);
    g_server = &server;

    // Handle Ctrl+C gracefully
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        server.start();  // Blocks until stopped
    } catch (const std::exception& e) {
        std::cerr << "[Error] " << e.what() << std::endl;
    }

    ttl_manager.stop();
    std::cout << "[Main] Server stopped cleanly." << std::endl;
    return 0;
}
