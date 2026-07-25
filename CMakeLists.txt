cmake_minimum_required(VERSION 3.20)
project(bitnulled VERSION 0.1.0 LANGUAGES CXX)

# Require strict C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. Core Library Target
add_library(bitnulled_core
    src/bencode.cpp
    src/torrent.cpp
    src/utils.cpp
)
target_include_directories(bitnulled_core PUBLIC include)

# 2. Main Executable Application Target
add_executable(bitnulled_app
    src/main.cpp
)
target_link_libraries(bitnulled_app PRIVATE bitnulled_core)

# 3. Unit Tests
enable_testing()

add_executable(test_bencode tests/test_bencode.cpp)
target_link_libraries(test_bencode PRIVATE bitnulled_core)
add_test(NAME TestBencode COMMAND test_bencode)

add_executable(test_sha1 tests/test_sha1.cpp)
target_link_libraries(test_sha1 PRIVATE bitnulled_core)
add_test(NAME TestSHA1 COMMAND test_sha1)

add_executable(test_torrent tests/test_torrent.cpp)
target_link_libraries(test_torrent PRIVATE bitnulled_core)
add_test(NAME TestTorrent COMMAND test_torrent)
