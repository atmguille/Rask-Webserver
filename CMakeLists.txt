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

#add_executable(main src/main.c srclib/url_encoded_form_parser/url_encoded_form_parser.c srclib/url_encoded_form_parser/url_encoded_form_parser.h)

find_package(Threads REQUIRED)
add_executable(rask src/server.c src/connection_handler.c include/connection_handler.h srclib/picohttpparser/picohttpparser.c srclib/logging/logging.c srclib/socket/socket.c src/thread_pool.c srclib/dynamic_buffer/dynamic_buffer.c srclib/dynamic_buffer/dynamic_buffer.h include/config_parser.h src/config_parser.c src/request.c include/request.h include/response.h src/response.c srclib/execute_scripts/execute_scripts.h srclib/execute_scripts/execute_scripts.c srclib/string/string.c srclib/string/string.h)
target_link_libraries(rask ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(rask m)
if (APPLE)
    target_link_libraries(rask ${CONFUSE_LIB})
else()
    target_link_libraries(rask confuse)
endif()
target_compile_options(rask PRIVATE -Wall -Wextra -pedantic)