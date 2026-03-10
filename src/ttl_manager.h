#pragma once
#include <thread>
#include <atomic>
#include "kvstore.h"

class TTLManager {
public:
    TTLManager(KVStore& store, int interval_seconds = 1);
    ~TTLManager();

    void start();
    void stop();

private:
    void run();

    KVStore& store_;
    int interval_seconds_;
    std::thread thread_;
    std::atomic<bool> running_;
};
