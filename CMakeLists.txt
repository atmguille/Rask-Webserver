cmake_minimum_required(VERSION 3.10)

project(rask)

if (APPLE)
    include_directories(/usr/local/include)

    find_library(
        CONFUSE_LIB
        NAMES confuse libconfuse
        HINTS /usr/local/lib
        NO_DEFAULT_PATH
    )
    message(STATUS "CONFUSE_LIB: [${CONFUSE_LIB}]")
endif()

add_library(logging srclib/logging/logging.c)
add_library(string srclib/string/string.c)
add_library(socket srclib/socket/socket.c)
target_link_libraries(socket logging)
add_library(execute_scripts srclib/execute_scripts/execute_scripts.c)
target_link_libraries(execute_scripts logging)
add_library(dynamic_buffer srclib/dynamic_buffer/dynamic_buffer.c)
target_link_libraries(dynamic_buffer logging)

add_executable(rask src/server.c src/connection_handler.c srclib/picohttpparser/picohttpparser.c src/thread_pool.c src/config_parser.c src/request.c src/response.c)

target_link_libraries(rask logging)
target_link_libraries(rask string)
target_link_libraries(rask socket)
target_link_libraries(rask execute_scripts)
target_link_libraries(rask dynamic_buffer)

find_package(Threads REQUIRED)
target_link_libraries(rask ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(rask m)

if (APPLE)
    target_link_libraries(rask ${CONFUSE_LIB})
else()
    target_link_libraries(rask confuse)
endif()
target_compile_options(rask PRIVATE -Wall -Wextra -pedantic)