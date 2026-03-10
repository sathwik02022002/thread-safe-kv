#include "ttl_manager.h"
#include <chrono>

TTLManager::TTLManager(KVStore& store, int interval_seconds)
    : store_(store), interval_seconds_(interval_seconds), running_(false) {}

TTLManager::~TTLManager() {
    stop();
}

void TTLManager::start() {
    running_ = true;
    thread_ = std::thread(&TTLManager::run, this);
}

void TTLManager::stop() {
    running_ = false;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void TTLManager::run() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds_));
        store_.purge_expired();
    }
}
