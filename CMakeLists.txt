cmake_minimum_required(VERSION 3.15)

project(practica1)

include_directories(/usr/local/include)

find_library(
        CONFUSE_LIB
        NAMES confuse libconfuse
        HINTS /usr/local/lib
        NO_DEFAULT_PATH
)

message(STATUS "CONFUSE_LIB: [${CONFUSE_LIB}]")

add_executable(iterative_server src/iterative_server.c srclib/logging/logging.c srclib/socket/socket.c)

find_package(Threads REQUIRED)
add_executable(server src/server.c src/connection_handler.c includes/connection_handler.h srclib/picohttpparser/picohttpparser.c srclib/logging/logging.c srclib/socket/socket.c src/thread_pool.c srclib/dynamic_buffer/dynamic_buffer.c srclib/dynamic_buffer/dynamic_buffer.h includes/config_parser.h src/config_parser.c)
target_link_libraries(server ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(server m)
target_link_libraries(server ${CONFUSE_LIB})