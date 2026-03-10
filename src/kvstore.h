#pragma once
#include <string>
#include <list>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <chrono>
#include <optional>

struct Entry {
    std::string value;
    std::chrono::steady_clock::time_point created_at;
    int ttl_seconds;  // -1 means no expiry
};

class KVStore {
public:
    KVStore(size_t max_keys = 1000);

    bool set(const std::string& key, const std::string& value, int ttl_seconds = -1);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    void flush();
    size_t size();

    // Called by TTL manager to purge expired keys
    void purge_expired();

private:
    bool is_expired(const Entry& entry) const;
    void evict_lru();

    std::unordered_map<std::string, Entry> store_;
    std::list<std::string> lru_order_;
    std::unordered_map<std::string, std::list<std::string>::iterator> lru_map_;

    mutable std::shared_mutex mutex_;
    size_t max_keys_;
};
