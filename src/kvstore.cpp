#include "kvstore.h"
#include <mutex>

KVStore::KVStore(size_t max_keys) : max_keys_(max_keys) {}

bool KVStore::is_expired(const Entry& entry) const {
    if (entry.ttl_seconds == -1) return false;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - entry.created_at).count();
    return elapsed >= entry.ttl_seconds;
}

void KVStore::evict_lru() {
    // Evict least recently used key (back of lru_order_)
    if (lru_order_.empty()) return;
    std::string lru_key = lru_order_.back();
    lru_order_.pop_back();
    lru_map_.erase(lru_key);
    store_.erase(lru_key);
}

bool KVStore::set(const std::string& key, const std::string& value, int ttl_seconds) {
    std::unique_lock lock(mutex_);

    // If key exists, remove from LRU tracking first
    if (store_.count(key)) {
        lru_order_.erase(lru_map_[key]);
        lru_map_.erase(key);
    }

    // Evict if at capacity and key is new
    if (store_.size() >= max_keys_ && !store_.count(key)) {
        evict_lru();
    }

    Entry entry;
    entry.value = value;
    entry.created_at = std::chrono::steady_clock::now();
    entry.ttl_seconds = ttl_seconds;

    store_[key] = entry;

    // Add to front of LRU (most recently used)
    lru_order_.push_front(key);
    lru_map_[key] = lru_order_.begin();

    return true;
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::unique_lock lock(mutex_);

    auto it = store_.find(key);
    if (it == store_.end()) return std::nullopt;

    if (is_expired(it->second)) {
        // Lazy deletion on access
        lru_order_.erase(lru_map_[key]);
        lru_map_.erase(key);
        store_.erase(it);
        return std::nullopt;
    }

    // Move to front (most recently used)
    lru_order_.erase(lru_map_[key]);
    lru_order_.push_front(key);
    lru_map_[key] = lru_order_.begin();

    return it->second.value;
}

bool KVStore::del(const std::string& key) {
    std::unique_lock lock(mutex_);

    auto it = store_.find(key);
    if (it == store_.end()) return false;

    lru_order_.erase(lru_map_[key]);
    lru_map_.erase(key);
    store_.erase(it);
    return true;
}

bool KVStore::exists(const std::string& key) {
    std::shared_lock lock(mutex_);
    auto it = store_.find(key);
    if (it == store_.end()) return false;
    return !is_expired(it->second);
}

void KVStore::flush() {
    std::unique_lock lock(mutex_);
    store_.clear();
    lru_order_.clear();
    lru_map_.clear();
}

size_t KVStore::size() {
    std::shared_lock lock(mutex_);
    return store_.size();
}

void KVStore::purge_expired() {
    std::unique_lock lock(mutex_);
    for (auto it = store_.begin(); it != store_.end(); ) {
        if (is_expired(it->second)) {
            lru_order_.erase(lru_map_[it->first]);
            lru_map_.erase(it->first);
            it = store_.erase(it);
        } else {
            ++it;
        }
    }
}
