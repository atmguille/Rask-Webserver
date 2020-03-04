cmake_minimum_required(VERSION 3.15)

project(practica1)

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

#add_executable(main src/main.c srclib/url_encoded_form_parser/url_encoded_form_parser.c srclib/url_encoded_form_parser/url_encoded_form_parser.h)


find_package(Threads REQUIRED)
add_executable(server src/server.c src/connection_handler.c includes/connection_handler.h srclib/picohttpparser/picohttpparser.c srclib/logging/logging.c srclib/socket/socket.c src/thread_pool.c srclib/dynamic_buffer/dynamic_buffer.c srclib/dynamic_buffer/dynamic_buffer.h includes/config_parser.h src/config_parser.c src/request.c includes/request.h includes/response.h src/response.c srclib/url_encoded_form_parser/url_encoded_form_parser.c srclib/url_encoded_form_parser/url_encoded_form_parser.h srclib/execute_scripts/execute_scripts.h srclib/execute_scripts/execute_scripts.c includes/utils.h)
target_link_libraries(server ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(server m)
if (APPLE)
    target_link_libraries(server ${CONFUSE_LIB})
else()
    target_link_libraries(server confuse)
endif()