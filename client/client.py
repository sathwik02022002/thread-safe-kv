import socket
import time


class KVClient:
    """Python client for the Thread-Safe KV Store."""

    def __init__(self, host: str = "127.0.0.1", port: int = 6379):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))
        print(f"[Client] Connected to {host}:{port}")

    def _send(self, command: str) -> str:
        self.sock.sendall((command + "\n").encode())
        response = self.sock.recv(4096).decode().strip()
        return response

    def set(self, key: str, value: str, ttl: int = -1) -> bool:
        cmd = f"SET {key} {value}" if ttl == -1 else f"SET {key} {value} {ttl}"
        return self._send(cmd) == "OK"

    def get(self, key: str):
        result = self._send(f"GET {key}")
        return None if result == "nil" else result

    def delete(self, key: str) -> bool:
        return self._send(f"DEL {key}") == "1"

    def exists(self, key: str) -> bool:
        return self._send(f"EXISTS {key}") == "1"

    def flush(self) -> bool:
        return self._send("FLUSH") == "OK"

    def size(self) -> int:
        return int(self._send("SIZE"))

    def close(self):
        self._send("QUIT")
        self.sock.close()
        print("[Client] Disconnected")


# ── Demo ───────────────────────────────────────────────────────────────────
if __name__ == "__main__":
    client = KVClient()

    print("\n--- Basic Operations ---")
    client.set("name", "Alice")
    client.set("role", "Engineer")
    client.set("company", "NetApp")
    print(f"GET name    → {client.get('name')}")
    print(f"GET role    → {client.get('role')}")
    print(f"EXISTS name → {client.exists('name')}")
    print(f"SIZE        → {client.size()}")

    print("\n--- TTL Demo (key expires in 3 seconds) ---")
    client.set("session", "abc123", ttl=3)
    print(f"GET session (now)      → {client.get('session')}")
    print("Waiting 4 seconds...")
    time.sleep(4)
    print(f"GET session (after TTL) → {client.get('session')}")

    print("\n--- Delete ---")
    client.delete("name")
    print(f"GET name after DEL → {client.get('name')}")

    print("\n--- Flush ---")
    client.flush()
    print(f"SIZE after FLUSH → {client.size()}")

    client.close()
