cmake_minimum_required(VERSION 3.10)
project(tcp_file_transfer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -pedantic)

find_package(Threads REQUIRED)

add_library(common STATIC
        src/common/encoding.cpp
        src/common/encoding.hpp
        src/common/errors.hpp
        src/common/port.hpp
        src/common/protocol.cpp
        src/common/protocol.hpp
        src/common/unique_fd.hpp
        src/common/units.cpp
        src/common/units.hpp
        src/common/utf8.hpp
)
target_include_directories(common PUBLIC src)

add_executable(server
        src/server/main.cpp
        src/server/server_app.cpp
        src/server/server_app.hpp
        src/server/server.cpp
        src/server/server.hpp
        src/server/listener.cpp
        src/server/listener.hpp
        src/server/client_session.cpp
        src/server/client_session.hpp
        src/server/connection.cpp
        src/server/connection.hpp
        src/server/speed_meter.cpp
        src/server/speed_meter.hpp
        src/server/upload_storage.cpp
        src/server/upload_storage.hpp
        src/server/file_name.cpp
        src/server/file_name.hpp
        src/server/log.cpp
        src/server/log.hpp
)
target_link_libraries(server PRIVATE common Threads::Threads)

add_executable(client
        src/client/main.cpp
        src/client/client_app.cpp
        src/client/client_app.hpp
        src/client/file_sender.cpp
        src/client/file_sender.hpp
        src/client/net.cpp
        src/client/net.hpp
)
target_link_libraries(client PRIVATE common)