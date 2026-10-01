cmake_minimum_required(VERSION 3.10)
project(lan_copies CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(lan_copies
        main.cpp
        app.cpp
        common.cpp
        protocol.cpp
        multicast_socket.cpp
        peer_table.cpp
        announcer.cpp
        listener.cpp
)

target_compile_options(lan_copies PRIVATE -Wall -Wextra -Wpedantic)