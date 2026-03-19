cmake_minimum_required(VERSION 3.15)
project(kvstore CXX)
set(CMAKE_CXX_STANDARD 17)

add_executable(kvstore_server
    src/main.cpp
    src/kvstore.cpp
    src/ttl_manager.cpp
    src/threadpool.cpp
    src/server.cpp
)
target_include_directories(kvstore_server PRIVATE src)

enable_testing()
add_executable(test_kvstore tests/test_kvstore.cpp src/kvstore.cpp src/ttl_manager.cpp)
target_include_directories(test_kvstore PRIVATE src)
add_test(NAME kvstore_tests COMMAND test_kvstore)
