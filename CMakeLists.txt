cmake_minimum_required(VERSION 3.15)

project(practica1)

add_executable(iterative_server src/iterative_server.c srclib/logging/logging.c srclib/socket/socket.c)

find_package(Threads REQUIRED)
add_executable(server src/server.c src/connection_handler.c includes/connection_handler.h srclib/picohttpparser/picohttpparser.c srclib/logging/logging.c srclib/socket/socket.c src/thread_pool.c srclib/DynamicBuffer/dynamic_buffer.c srclib/DynamicBuffer/dynamic_buffer.h)
target_link_libraries(server ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(server m)