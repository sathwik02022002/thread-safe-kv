#include <iostream>
#include <thread>
#include <vector>
#include <cassert>
#include <chrono>
#include "../src/kvstore.h"
#include "../src/ttl_manager.h"

int passed = 0;
int failed = 0;

void test(const std::string& name, bool condition) {
    if (condition) {
        std::cout << "  [PASS] " << name << std::endl;
        passed++;
    } else {
        std::cout << "  [FAIL] " << name << std::endl;
        failed++;
    }
}

// ── Basic SET / GET / DEL ──────────────────────────────────────────────────
void test_basic_operations() {
    std::cout << "\n[Test] Basic Operations" << std::endl;
    KVStore store(1000);

    store.set("name", "Alice");
    auto val = store.get("name");
    test("SET and GET", val.has_value() && val.value() == "Alice");

    test("EXISTS returns true", store.exists("name"));
    test("EXISTS returns false for missing key", !store.exists("missing"));

    store.del("name");
    test("DEL removes key", !store.get("name").has_value());

    store.set("a", "1");
    store.set("b", "2");
    store.flush();
    test("FLUSH clears store", store.size() == 0);
}

// ── TTL Expiry ─────────────────────────────────────────────────────────────
void test_ttl() {
    std::cout << "\n[Test] TTL Expiry" << std::endl;
    KVStore store(1000);

    store.set("temp", "value", 1);  // expires in 1 second
    test("Key exists before TTL", store.get("temp").has_value());

    std::this_thread::sleep_for(std::chrono::seconds(2));
    test("Key expired after TTL", !store.get("temp").has_value());

    store.set("permanent", "value", -1);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    test("Key without TTL does not expire", store.get("permanent").has_value());
}

// ── LRU Eviction ──────────────────────────────────────────────────────────
void test_lru_eviction() {
    std::cout << "\n[Test] LRU Eviction" << std::endl;
    KVStore store(3);  // tiny store for testing

    store.set("a", "1");
    store.set("b", "2");
    store.set("c", "3");

    // Access "a" to make it recently used; "b" becomes LRU
    store.get("a");
    store.get("c");

    // Adding "d" should evict "b" (least recently used)
    store.set("d", "4");

    test("LRU key evicted", !store.get("b").has_value());
    test("Recently used key kept", store.get("a").has_value());
    test("New key added", store.get("d").has_value());
}

// ── Concurrent Access ──────────────────────────────────────────────────────
void test_concurrent_access() {
    std::cout << "\n[Test] Concurrent Access" << std::endl;
    KVStore store(1000);
    const int NUM_THREADS = 8;
    const int OPS_PER_THREAD = 100;

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&store, i]() {
            for (int j = 0; j < OPS_PER_THREAD; ++j) {
                std::string key = "key_" + std::to_string(i) + "_" + std::to_string(j);
                store.set(key, "value_" + std::to_string(j));
                store.get(key);
            }
        });
    }
    for (auto& t : threads) t.join();

    test("No crash under concurrent writes", true);
    test("Store size within bounds", store.size() <= 1000);
}

// ── TTL Manager Purge ─────────────────────────────────────────────────────
void test_ttl_manager() {
    std::cout << "\n[Test] TTL Manager Background Purge" << std::endl;
    KVStore store(1000);
    TTLManager mgr(store, 1);
    mgr.start();

    store.set("expires", "soon", 1);
    std::this_thread::sleep_for(std::chrono::seconds(3));

    test("Background purge removed expired key", store.size() == 0);
    mgr.stop();
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "       KV Store Test Suite              " << std::endl;
    std::cout << "========================================" << std::endl;

    test_basic_operations();
    test_ttl();
    test_lru_eviction();
    test_concurrent_access();
    test_ttl_manager();

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Results: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return failed > 0 ? 1 : 0;
}
