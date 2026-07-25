cmake_minimum_required(VERSION 3.20)
project(bitnulled VERSION 0.1.0 LANGUAGES CXX)

# Exigir padrão C++20 estrito
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Gerar banco de dados de compilação para o clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Opções de compilação adicionais para exibir avisos úteis (Warnings)
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. Biblioteca Core (Código do BitTorrent: Bencode, Tracker, Peer Wire, etc.)
add_library(bitnulled_core
    src/bencode.cpp
)
target_include_directories(bitnulled_core PUBLIC include)

# 2. Executável Principal (App)
add_executable(bitnulled_app
    src/main.cpp
)
target_link_libraries(bitnulled_app PRIVATE bitnulled_core)

# 3. Suporte a Testes
enable_testing()

add_executable(test_bencode
    tests/test_bencode.cpp
)
target_link_libraries(test_bencode PRIVATE bitnulled_core)

# Registrar o teste no CTest
add_test(NAME TestBencode COMMAND test_bencode)

add_executable(test_sha1
    tests/test_sha1.cpp
)
target_link_libraries(test_sha1 PRIVATE bitnulled_core)
add_test(NAME TestSHA1 COMMAND test_sha1)

