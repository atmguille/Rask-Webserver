cmake_minimum_required(VERSION 3.15)

project(practica1)

add_executable(iterative_server src/iterative_server.c srclib/logging/logging.c srclib/socket/socket.c)

find_package(Threads REQUIRED)
add_executable(server src/server.c srclib/logging/logging.c srclib/socket/socket.c src/thread_pool.c src/config_parser.c includes/config_parser.h)
target_link_libraries(server ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(server m)
target_link_libraries(server confuse)