CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread

SRC_DIR  = src
TEST_DIR = tests
BUILD    = build

SRCS     = $(SRC_DIR)/kvstore.cpp \
           $(SRC_DIR)/ttl_manager.cpp \
           $(SRC_DIR)/threadpool.cpp \
           $(SRC_DIR)/server.cpp \
           $(SRC_DIR)/main.cpp

TEST_SRCS = $(SRC_DIR)/kvstore.cpp \
            $(SRC_DIR)/ttl_manager.cpp \
            $(TEST_DIR)/test_kvstore.cpp

SERVER_BIN = $(BUILD)/kvstore_server
TEST_BIN   = $(BUILD)/test_kvstore

.PHONY: all server test clean

all: server test

$(BUILD):
	mkdir -p $(BUILD)

server: $(BUILD)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(SERVER_BIN)
	@echo "✅  Build successful → $(SERVER_BIN)"

test: $(BUILD)
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $(TEST_BIN)
	@echo "✅  Test build successful → $(TEST_BIN)"
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD)
	@echo "🧹  Cleaned build directory"
