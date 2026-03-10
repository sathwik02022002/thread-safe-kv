# Thread-Safe In-Memory Key-Value Store

A Redis-inspired, thread-safe in-memory key-value store built in C++ with a Python client. Built to demonstrate systems programming concepts including multithreading, LRU eviction, TTL expiry, and TCP socket communication.

---

## Features

- **Thread-safe reads/writes** using `std::shared_mutex` (multiple concurrent readers, exclusive writers)
- **TTL expiry** — keys auto-expire via a background thread + lazy deletion on access
- **LRU eviction** — evicts the least recently used key when the store reaches capacity (1000 keys)
- **Thread pool** — handles concurrent client connections efficiently using a fixed pool of 4 worker threads
- **TCP socket server** — accepts connections on port 6379
- **Python client** — simple API to interact with the server

---

## Architecture

```
Python Client
     │ TCP (port 6379)
Socket Server
     │
Thread Pool (4 workers)
     │
KV Store Core
  ├── Hash Map (std::unordered_map)
  ├── LRU Tracker (std::list + std::unordered_map)
  └── TTL Manager (background std::thread)
```

---

## Requirements

- macOS with Xcode Command Line Tools (`xcode-select --install`)
- `g++` with C++17 support (`g++ --version`)
- Python 3 (for the client demo)

---

## Build & Run

### Build everything
```bash
make all
```

### Run the server
```bash
./build/kvstore_server
# Optional: specify a custom port
./build/kvstore_server 7000
```

### Run tests
```bash
make test
```

### Run the Python client demo
```bash
# In a separate terminal, while the server is running:
python3 client/client.py
```

---

## Supported Commands

| Command            | Example                  | Description                        |
|--------------------|--------------------------|------------------------------------|
| `SET key val [ttl]`| `SET name Alice 60`      | Set key with optional TTL (seconds)|
| `GET key`          | `GET name`               | Get value, returns `nil` if missing|
| `DEL key`          | `DEL name`               | Delete a key, returns `1` or `0`   |
| `EXISTS key`       | `EXISTS name`            | Returns `1` if exists, else `0`    |
| `FLUSH`            | `FLUSH`                  | Delete all keys                    |
| `SIZE`             | `SIZE`                   | Return number of stored keys       |
| `QUIT`             | `QUIT`                   | Close client connection            |

You can also test with `telnet` or `nc`:
```bash
nc 127.0.0.1 6379
SET name Alice
GET name
SET session abc 5
EXISTS session
QUIT
```

---

## Design Decisions

### Why `std::shared_mutex`?
Read operations (GET, EXISTS) are far more frequent than writes. `shared_mutex` allows multiple threads to read simultaneously while ensuring exclusive access for writes — better throughput than a plain `std::mutex`.

### Why LRU with `std::list` + `std::unordered_map`?
This gives O(1) access, update, and eviction. The list maintains insertion/access order; the map gives instant lookup of list iterators for any key.

### Why lazy TTL deletion + background purge?
Lazy deletion catches expired keys immediately on access (no stale reads). The background TTL manager periodically cleans keys that are never accessed — preventing memory leaks from abandoned keys.

### Why a thread pool instead of thread-per-connection?
Spawning a new OS thread per client is expensive and doesn't scale. A fixed pool of 4 threads processes requests from a shared queue, reusing threads efficiently.

---

## Project Structure

```
kv-store/
├── src/
│   ├── main.cpp          # Entry point, wires all components
│   ├── kvstore.h/.cpp    # Core store: hash map, LRU, TTL logic
│   ├── ttl_manager.h/.cpp# Background thread for expiry purge
│   ├── threadpool.h/.cpp # Fixed worker thread pool
│   └── server.h/.cpp     # TCP socket server + command parser
├── client/
│   └── client.py         # Python client with demo
├── tests/
│   └── test_kvstore.cpp  # Unit tests for all components
├── Makefile
└── README.md
```
